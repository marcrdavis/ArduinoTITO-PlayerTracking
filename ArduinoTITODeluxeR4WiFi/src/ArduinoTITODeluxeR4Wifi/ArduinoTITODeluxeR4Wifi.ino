/*
  Arduino TITO Deluxe v3.0.20241013DR4
  by Marc R. Davis - Copyright (c) 2020-2024 All Rights Reserved
  https://github.com/marcrdavis/ArduinoTITO-PlayerTracking

  Portions of the Arduino SAS protocol implementation by Ian Walker - Thank you!
  Additional testing and troubleshooting by NLG member Eddiiie - Thank you!
  9-Bit code by j.DeLutis and vashadow - Thank you!

  Hardware requirements: 
  Arduino Uno R4 WiFi; Serial Port

  For TITO setup please follow the included documentation

  Note: Remote access has been made compatible with BETTORSlots TITO apps for IOS/Android; BETTORSlots is
  not affiliated with this project and does not support or endorse using their apps for this purpose;
  This project does not use BETTORSlots code.

  This software is licensed to you under The Creative Commons Attribution-NonCommercial-NoDerivatives 4.0
  International license (https://creativecommons.org/licenses/by-nc-nd/4.0/legalcode).

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
  AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// ------------------------------------------------------------------------------------------------------------
// Required Libraries
// ------------------------------------------------------------------------------------------------------------

#include "WiFiS3.h"
#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"
#include <Arduino.h>

// ------------------------------------------------------------------------------------------------------------
// Core Variables
// ------------------------------------------------------------------------------------------------------------

bool changeToCredits = 0; // Set to 1 to enable Change to Credits
bool useDHCP = 0; // Set to 1 to enable DHCP
bool sasOnline = false;
bool sasError = false;

char* changeCredits = "500"; // Credits to add 
char* webUI = "http://arduinotito.infinityfreeapp.com/?ip="; // The url of the site hosting the webUI; change if you want to host it locally; must append /?ip= to url

char* ssid = ""; // Set your WiFi SSID here
char* pass = ""; // Set your WiFi Password here
int status = WL_IDLE_STATUS;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // UPDATE BEFORE COMPILING IF YOU HAVE MULTIPLE MACHINES - Change to unique address for each board 
IPAddress ip(192, 168, 1, 254); // Your default IP address - change if your network addressing is different or if you have multiple machines
String versionString = "3.0.20241013DR4";

// ------------------------------------------------------------------------------------------------------------
// SAS Protocol Variables
// ------------------------------------------------------------------------------------------------------------

byte SASAdr = 0x01;
byte CRCH = 0x00;
byte CRCL = 0x00;

uint16_t gPoll1[] = {0xFF80};
uint16_t gPoll2[] = {0xFF81};

byte SVNS[2] = {SASAdr, 0x57};
byte TP[2] = {SASAdr, 0x70};
byte HPI[2] = {SASAdr, 0x1B};
byte JREST[4] = {SASAdr, 0x94, 0x00, 0x00};
byte LOCK[4] = {SASAdr, 0x01, 0x00, 0x00};
byte ULOCK[4] = {SASAdr, 0x02, 0x00, 0x00};
byte MUTE[4] = {SASAdr, 0x03, 0x00, 0x00};
byte UMUTE[4] = {SASAdr, 0x04, 0x00, 0x00};
byte EBILL[4] = {SASAdr, 0x06, 0x00, 0x00};
byte DBILL[4] = {SASAdr, 0x07, 0x00, 0x00};
byte EVInfo[5] = {SASAdr, 0x4D, 0x00, 0x00, 0x00};
byte transComplete[6] = {SASAdr, 0x71, 0x01, 0xFF, 0x1F, 0xD0};
byte mCredits[2] = {SASAdr, 0x1A};
byte mCoinIn[2] = {SASAdr, 0x11};
byte mTotWon[2] = {SASAdr, 0x12};
byte mTotGames[2] = {SASAdr, 0x15};
byte mgamesWon[2] = {SASAdr, 0x16};
byte mgamesLost[2] = {SASAdr, 0x17};

byte LBS [9];
byte SVN [13];
byte TPS [35];
byte HPS [24];
byte TEQ [21];
byte TRS [21];
byte COT [10];
byte COS [5];
byte TDR [5];
byte TIM [11];

// ------------------------------------------------------------------------------------------------------------
// Setup instances
// ------------------------------------------------------------------------------------------------------------

WiFiServer server(80);
ArduinoLEDMatrix matrix;

// ------------------------------------------------------------------------------------------------------------
// Setup - called once during init
// ------------------------------------------------------------------------------------------------------------

void setup()
{
  // Serial Logging
  Serial.begin(9600);
  
  matrix.begin();

  // Setup SAS/TITO Communications - 9-Bit support required via updated HardwareSerial.h and Serial.cpp
  Serial1.begin(19200,SERIAL_9N1);
  Serial1.setTimeout(200);
  pinMode(LED_BUILTIN, OUTPUT);

  matrixPrint("ATD");

  // Initialize WiFi
  initWiFi();

  Serial.print(F("Arduino TITO Deluxe WiFi R4 - By Marc R. Davis - Version ")); Serial.println(versionString);
  
  // Initialize HTTP Server
  server.begin();
  
  // Clear the game serial buffer
  if (Serial1.available() > 3) while (Serial1.available() > 3) Serial1.read();  

  Serial.println(F("Initialization complete"));  
  
  matrixPrint(" OK"); 
}

// ------------------------------------------------------------------------------------------------------------
// Main processing loop
// ------------------------------------------------------------------------------------------------------------

void loop()
{
  // Flash the LED
  digitalWrite(LED_BUILTIN, HIGH);

  // Check web
  htmlPoll();

  // Flash the LED
  digitalWrite(LED_BUILTIN, LOW);
  
  // Check game
  generalPoll();
}

// ------------------------------------------------------------------------------------------------------------
// Network Functions
// ------------------------------------------------------------------------------------------------------------

// Initialize the WiFi
void initWiFi()
{
    if (strlen(ssid) == 0 | strlen(pass) == 0) {
    Serial.println(F("SSID or Password not set in configuration!"));
    // don't continue
    matrixPrint("ERR"); 
    while (true);
  }

    // Check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println(F("Communication with WiFi module failed!"));
    // don't continue
    matrixPrint("ERR"); 
    while (true);
  }

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print(F("Attempting to connect to SSID: "));
    Serial.println(ssid);

    if (!useDHCP) WiFi.config(ip);
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(3000);
  }

  // Connection Information
  Serial.print(F("SSID: "));
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print(F("IP Address: "));
  Serial.println(ip);

  long rssi = WiFi.RSSI();
  Serial.print(F("Signal Strength: "));
  Serial.print(rssi);
  Serial.println("dBm");
}

// ------------------------------------------------------------------------------------------------------------
// Game Functions
// ------------------------------------------------------------------------------------------------------------

// Add X credits to game

bool addCredits(String credits)
{
  Serial.println("Add Credits");
  String paddedValue = "";
  paddedValue.reserve(8);

  byte ac[4];
  credits.trim();
  for (int i = 0; i < 8 - credits.length(); i++) paddedValue += "0";
  paddedValue += credits;
  ac[0] = dec2bcd(paddedValue.substring(0, 2).toInt());
  ac[1] = dec2bcd(paddedValue.substring(2, 4).toInt());
  ac[2] = dec2bcd(paddedValue.substring(4, 6).toInt());
  ac[3] = dec2bcd(paddedValue.substring(6, 8).toInt());

  if (LegacyBonus(0x01, ac[0], ac[1], ac[2], ac[3], 0x00))
  {
    return true;
  }
  else return false;
}

// ------------------------------------------------------------------------------------------------------------
// Misc Functions
// ------------------------------------------------------------------------------------------------------------

// Matrix display text message

void matrixPrint(const char text[])
{
    matrix.beginDraw();
    matrix.stroke(0xFFFFFFFF);
    matrix.textFont(Font_4x6);
    matrix.beginText(0, 1, 0xFFFFFF);
    matrix.println(text);
    matrix.endText();
    matrix.endDraw();
}

// Get a value in a string of data split by a separator character

String getQueryValue(const char* queryString, const char* key) {
  const char* delimiter = "&";
  const char* valueDelimiter = "=";

  char queryStringCopy[strlen(queryString) + 1];
  strcpy(queryStringCopy, queryString);

  char* pair = strtok(queryStringCopy, delimiter);

  while (pair != NULL) {
    char* keyValueSeparator = strstr(pair, valueDelimiter);

    if (keyValueSeparator != NULL) {
      char* currentKey = pair;
      size_t keyLength = keyValueSeparator - pair;

      if ((keyLength == strlen(key) && strncmp(currentKey, key, keyLength) == 0) ||
          (keyLength == strlen(key) - 1 && strncmp(currentKey, key, keyLength) == 0 && currentKey[keyLength] == '=')) {
        char* value = keyValueSeparator + strlen(valueDelimiter);

        // Find the first occurrence of a blank space
        char* spacePosition = strchr(value, ' ');
        if (spacePosition != NULL) {
          *spacePosition = '\0';  // Set the blank space to null terminator
        }

        return String(value);
      }
    }

    pair = strtok(NULL, delimiter);
  }

  return String();  // Return an empty string if key not found
}

// Used by urlDecode

unsigned char h2int(char c)
{
  if (c >= '0' && c <= '9') {
    return ((unsigned char)c - '0');
  }
  if (c >= 'a' && c <= 'f') {
    return ((unsigned char)c - 'a' + 10);
  }
  if (c >= 'A' && c <= 'F') {
    return ((unsigned char)c - 'A' + 10);
  }
  return (0);
}

// Decode querystring parameters

String urlDecode(String str)
{
  String decodedString = "";
  char c;
  char code0;
  char code1;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == '+') {
      decodedString += ' ';
    } else if (c == '%') {
      i++;
      code0 = str.charAt(i);
      i++;
      code1 = str.charAt(i);
      c = (h2int(code0) << 4) | h2int(code1);
      decodedString += c;
    } else {

      decodedString += c;
    }

    yield();
  }

  return decodedString;
}

// ------------------------------------------------------------------------------------------------------------
// HTML Server
// ------------------------------------------------------------------------------------------------------------

void htmlPoll()
{ 
  WiFiClient client = server.available();
  if (!client) return;

  if (client.connected()) // If client is present and connected
  {
    bool reqResult = false;
    char stringData[130]; 
    memset(stringData, 0, sizeof(stringData));
    
    int size = client.readBytesUntil('\n', stringData, sizeof(stringData)); // Get the first line of request
    while (client.available()) client.read(); // Get the rest of the header and discard
   
    // Parse querystring
    if (strstr(stringData,"ds=")) // Game Statistics
    {
      client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n<html>\r\n"));
      client.print(F("<head></head><body><h2>GAME STATISTICS</h2><br>"));
      client.print(F("Credits<br>"));
      client.print(pollMeters(mCredits));
      client.print(F("<br>Total In<br>"));
      client.print(pollMeters(mCoinIn));
      client.print(F("<br>Total Won<br>"));
      client.print(pollMeters(mTotWon));
      client.print(F("<br>Total Games<br>"));
      client.print(pollMeters(mTotGames));
      client.print(F("<br>Games Won<br>"));
      client.print(pollMeters(mgamesWon));
      client.print(F("<br>Games Lost<br>"));
      client.print(pollMeters(mgamesLost));
      client.print(F("<br>"));      
      client.print(F("</body>"));
      client.print(F("</html>\r\n\r\n"));
      client.stop();
      return;
    }

    // All of the remaining commands return a single status or a result code

    if (strstr(stringData,"mo=")) reqResult=slotCommand(MUTE,4,"Sound Off");
    if (strstr(stringData,"mt=")) reqResult=slotCommand(UMUTE,4,"Sound On");
    if (strstr(stringData,"lk=")) reqResult=slotCommand(LOCK,4,"Game Locked");
    if (strstr(stringData,"uk=")) reqResult=slotCommand(ULOCK,4,"Game Unlocked");
    if (strstr(stringData,"eb=")) reqResult=slotCommand(EBILL,4,"BV Enabled");
    if (strstr(stringData,"db=")) reqResult=slotCommand(DBILL,4,"BV Disabled");
    if (strstr(stringData,"jr=")) reqResult=slotCommand(JREST,4,"Jackpot Reset");
    if (strstr(stringData,"ec=")) changeToCredits=1;
    if (strstr(stringData,"dc=")) changeToCredits=0;

    if (strstr(stringData,"rb=")) // Reboot Arduino
    {
      client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n<!DOCTYPE HTML>\r\n<html><head><meta http-equiv='Refresh' content='8; url=\""));
      client.print(webUI);
      client.print(ip);
      client.println(F("\"' /></head></html>"));
      client.print(F("Rebooting..."));
      matrixPrint("   "); 
      client.stop();
      NVIC_SystemReset();
      return;
    }
    
    if (strstr(stringData,"ud=")) // Update Ticket Data
    {
      memmove(stringData, stringData + 6, size - 6 + 1);  // +1 to include the null terminator
      String location = urlDecode(getQueryValue(stringData,"ud1"));      
      String address1 = urlDecode(getQueryValue(stringData,"ud2"));    
      String address2 = urlDecode(getQueryValue(stringData,"ud3"));    
      reqResult=SetTicketData(location, address1, address2);
    }
   
    if (strstr(stringData,"cr=")) // Add Credits to game
    {
      String credits = getQueryValue(stringData,"cr");
      reqResult=addCredits(credits);
    }

    if (strstr(stringData, "GET / HTTP/1.1"))
    {
      // Show web interface
      client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n<!DOCTYPE HTML>\r\n<html><head><meta http-equiv='Refresh' content='0; url=\""));
      client.print(webUI);
      client.print(ip);
      client.println(F("\"' /></head></html>"));
    }
    else
    {        
      // Send status/result to client
      client.println(F("HTTP/1.1 200 OK\r\n\r\n"));
      if (reqResult) client.print(F("OK"));
      else client.print(F("ERROR"));
    }

    client.stop();
    delay(1000);
    return;
  }
}

// ------------------------------------------------------------------------------------------------------------
// SAS Protocol
// NOTE: This does not implement the full protocol - just enough for TITO, Meters, some controls and
//       System Bonusing
// ------------------------------------------------------------------------------------------------------------

// The General poll must be running in order to send data to game and receive responses

void generalPoll()
{
  byte eventCode = 0;
  Serial1.write((uint8_t *)gPoll1, sizeof(gPoll1));
  delay(20);
  Serial1.write((uint8_t *)gPoll2, sizeof(gPoll2));
  delay(10);  // Wait for data on the serial bus

  if (Serial1.available() > 0) {
    eventCode = Serial1.read();
     if (sasOnline==false) sasOnline=true;
  }

  if (eventCode != 0x1F && eventCode != 0x00 && eventCode != 0x01 && eventCode != 0x80 && eventCode != 0x81 && eventCode != 0x7C) {
    Serial.print(F("SAS Event Received: ")); Serial.print(eventCode, HEX); Serial.print(F(" "));

    switch (eventCode) {
      case 0x11:
        Serial.println(F("Game door opened"));
        break;
      case 0x12:
        Serial.println(F("Game door closed"));
        break;
      case 0x17:
        Serial.println(F("AC power was applied to gaming machine"));
        break;
      case 0x18:
        Serial.println(F("AC power was lost from gaming machine"));
        break;
      case 0x19:
        Serial.println(F("Cashbox door was opened"));
        break;
      case 0x1A:
        Serial.println(F("Cashbox door was closed"));
        break;
      case 0x66:
        Serial.println(F("Cash out button pressed"));
        break;
      case 0x51:
        Serial.println(F("Handpay is pending"));
        getHandpayInfo();
        break;
      case 0x52:
        Serial.println(F("Handpay was reset"));
        break;
      case 0x2B:
        Serial.println(F("Bill rejected"));
        break;
      case 0x7E:
        Serial.println(F("Game started"));
        break;
      case 0x7F:
        Serial.println(F("Game ended"));
        break;
      case 0x71:
      case 0x72:
        if (changeToCredits) {
          addCredits(changeCredits);  // To enable 'Change button' credits
        }
        break;
      case 0x57:
        SystemValidation();
        break;
      case 0x3D:
        CashOutState();
        break;
      case 0x67:
        RedeemTicket();
        break;
      case 0x68:
        ConfirmRedeem();
        break;
      default:
        break;
    }
    matrixPrint(" OK"); 
    Serial.println(F(""));
  }
}

int dec2bcd(byte val)
{
  return ( ((val / 10) << 4) | (val % 10) );
}

int bcd2dec(byte bcd)
{
  return ( ((bcd >> 4) * 10) + (bcd & 0xF) );
}

// Data from game may be delayed due to other events or the state of the board

void waitForResponse(byte & waitfor, byte * ret, int sz)
{
  byte responseBytes[sz - 2];
  int wait = 0;
  sasError = false;

  while (Serial1.read() != waitfor && wait < 3000) {
    delay(1);
    wait += 1;
  }
 
  if (wait >= 3000) {
    Serial.println(F("Timeout waiting for Response!"));
    memset(ret, 0, sz);
    matrixPrint("ERR"); 
    sasError = true;
    return;
  }
 
  Serial1.readBytes(responseBytes, sizeof(responseBytes));
  ret[0] = {0x01};
  ret[1] = waitfor;
  memcpy(ret + 2, responseBytes, sizeof(responseBytes));

  return;
}

bool waitForACK(byte waitfor, const char msg[])
{
  int wait = 0;

  while (Serial1.read() != waitfor && wait < 3000) {
    delay(1);
    wait += 1;
  }

  if (wait >= 3000) {
      Serial.println(F("Timeout waiting for ACK!"));
      matrixPrint("ERR"); 
      return false;
  }
  
  return true;
}

long pollMeters(byte *meter)
{
  byte meterData[8];

  SendTypeR(meter, 2); // meter size will always be 2; sizeof was returning 4 due to the 32 bit pointers
  waitForResponse(meter[1], meterData, sizeof(meterData));
 
  String sMeter;
  for (int i = 2; i < 6; i++) {
    sMeter.concat(String(bcd2dec(meterData[i]) < 10 ? "0" : ""));
    sMeter.concat(String(bcd2dec(meterData[i])));
  }

  return sMeter.toInt();
}

bool slotCommand(byte cmd[], int len, const char msg[])
{
  SendTypeS(cmd, len);
  return waitForACK(SASAdr, msg);
}

void getHandpayInfo()
{
  Serial.println(F("Getting handpay information"));
  SendTypeR(HPI, sizeof(HPI));
  waitForResponse(HPI[1], HPS, sizeof(HPS));  
}

void SystemValidation()
{
  // Retry up to 2 times
  Serial.println(F("Getting cashout information"));

  for (int x = 0; x < 2; x++) {
    SendTypeR(SVNS, sizeof(SVNS));
    waitForResponse(SVNS[1], COT, sizeof(COT));
    if (!sasError) break;
  }

  if (!sasError)
  { 
    SVN [0] = SASAdr;                    // Address
    SVN [1] = 0x58;                      // Receive Validation Number
    SVN [2] = 0x01;                      // Validation ID
    SVN [3] = COT [2];                   // Cashout Type
    SVN [4] = 0x00;                      // None
    SVN [5] = 0x00;                      // None
    SVN [6] = COT [3];                   // Cashout Value Byte5 (MSB)
    SVN [7] = COT [4];                   // Cashout Value Byte4
    SVN [8] = COT [5];                   // Cashout Value Byte3
    SVN [9] = COT [6] ;                  // Cashout Value Byte2
    SVN [10] = COT [7];                  // Cashout Value Byte1 (LSB)

    CalculateCRC(COT, 8);
    if (CRCH != COT[8] && CRCL != COT[9])
    {
      // Bad Validation Data
      Serial.println(F("Unable to validate - CRC does not match"));
      SVN [2] = 0x00;
      SendTypeS(SVN, sizeof(SVN));
      waitForResponse(SVN[1], COS, sizeof(COS));
      Serial.println(F("Cashout aborted"));
    }
    else
    {
      Serial.println(F("Validating cashout request"));
      SendTypeS(SVN, sizeof(SVN));
      waitForResponse(SVN[1], COS, sizeof(COS));     
      Serial.println(F("Printing cashout ticket"));
    }
  }
}

bool SetTicketData(String loc, String addr1, String addr2)
{
  byte bLoc[loc.length() + 1];
  byte bAddr1[addr1.length() + 1];
  byte bAddr2[addr2.length() + 1];
  byte ticketData[11 + loc.length() + addr1.length() + addr2.length()];
  byte bSize = 6 + loc.length() + addr1.length() + addr2.length();

  loc.getBytes(bLoc, loc.length() + 1);
  addr1.getBytes(bAddr1, addr1.length() + 1);
  addr2.getBytes(bAddr2, addr2.length() + 1);

  ticketData[0] = SASAdr;
  ticketData[1] = 0x7D;
  ticketData[2] = bSize;
  ticketData[3] = 0x00;
  ticketData[4] = 0x00;
  ticketData[5] = 0x00;
  ticketData[6] = loc.length();

  memcpy(ticketData + 7, bLoc, loc.length());
  ticketData[7 + loc.length()] = addr1.length();
  memcpy(ticketData + 8 + loc.length(), bAddr1, addr1.length());
  ticketData[8 + loc.length() + addr1.length()] = addr2.length();
  memcpy(ticketData + 9 + loc.length() + addr1.length(), bAddr2, addr2.length());

  SendTypeS(ticketData, sizeof(ticketData));
  waitForResponse(ticketData[1], TDR, sizeof(TDR));
  Serial.println(F("Updated Cashout Ticket Data"));
  return true;
}

void CashOutState()
{
  SendTypeS(EVInfo, sizeof(EVInfo));
  waitForResponse(EVInfo[1], TPS, sizeof(TPS));
  Serial.println(F("Cashout ticket has been printed"));
}

bool SetDateTime(String datetime)
{
  datetime.trim();
  
  TIM [0] = SASAdr;
  TIM [1] = 0x7F;
  TIM [2] = dec2bcd(datetime.substring(0, 2).toInt());
  TIM [3] = dec2bcd(datetime.substring(2, 4).toInt());
  TIM [4] = dec2bcd(datetime.substring(4, 6).toInt());
  TIM [5] = dec2bcd(datetime.substring(6, 8).toInt());
  TIM [6] = dec2bcd(datetime.substring(8, 10).toInt());
  TIM [7] = dec2bcd(datetime.substring(10, 12).toInt());
  TIM [8] = dec2bcd(datetime.substring(12, 14).toInt());

  SendTypeS(TIM, sizeof(TIM));
  return waitForACK(SASAdr,"DateTime set by host");
}

bool LegacyBonus (byte SASAdr, byte Amount1, byte Amount2, byte Amount3, byte Amount4, byte Type)
{
  LBS [0] = SASAdr;
  LBS [1] = 0x8A;
  LBS [2] = Amount1;
  LBS [3] = Amount2;
  LBS [4] = Amount3;
  LBS [5] = Amount4;
  LBS [6] = Type;

  SendTypeS(LBS, sizeof(LBS));
  return waitForACK(SASAdr, "");
}

void RedeemTicket()
{
  // Retry up to 2 times
  Serial.println(F("Ticket inserted"));
  
  for (int x = 0; x < 2; x++) {
    SendTypeR(TP, sizeof(TP));
    Serial.println(F("Waiting for ticket data"));
    waitForResponse(TP[1], TEQ, sizeof(TEQ));
    if (!sasError) break;
  }
  
  if (!sasError)
  {  
    Serial.println(F("Received ticket data"));
    
    TRS [0] = 0x01;                              // Address
    TRS [1] = 0x71;                              // Command
    TRS [2] = 0x10;                              // Number of Bytes
    TRS [3] = 0x00;                              // Transfer Code
    TRS [4] = TEQ [14];                          // Ticket Amount BCD1  LSB
    TRS [5] = TEQ [15];                          // Ticket Amount BCD2
    TRS [6] = TEQ [16];                          // Ticket Amount BCD3
    TRS [7] = TEQ [17];                          // Ticket Amount BCD4
    TRS [8] = TEQ [18];                          // Ticket Amount BCD5  MSB
    TRS [9] = 0x00;                              // Parsing Code
    TRS [10] = TEQ [10];                         // Validation BCD1
    TRS [11] = TEQ [11];                         // Validation BCD2
    TRS [12] = TEQ [12];                         // Validation BCD3
    TRS [13] = TEQ [13];                         // Validation BCD4
    TRS [14] = TEQ [14];                         // Validation BCD5
    TRS [15] = TEQ [15];                         // Validation BCD6
    TRS [16] = TEQ [16];                         // Validation BCD7
    TRS [17] = TEQ [17];                         // Validation BCD8
    TRS [18] = TEQ [18];                         // Validation BCD9
  
    Serial.println(F("Authorizing ticket"));
    SendTypeS(TRS, sizeof(TRS));  
    waitForResponse(TRS[1], TEQ, sizeof(TEQ));
  
    // Report on common responses
    if (!sasError)
    {
      if (TEQ[3] == 0x00) Serial.println(F("Redeeming ticket"));
      if (TEQ[3] == 0x40) Serial.println(F("Ticket redemption pending"));
      if (TEQ[3] == 0x80) Serial.println(F("Ticket rejected"));
      if (TEQ[3] == 0x81) Serial.println(F("Ticket validation number does not match"));
      if (TEQ[3] == 0x84) Serial.println(F("Transfer amount exceeded the gaming machine credit limit"));  
      if (TEQ[3] == 0x85) Serial.println(F("Transfer amount not an even multiple of gaming machine denomination"));  
      if (TEQ[3] == 0x87) Serial.println(F("Gaming machine unable to accept transfer at this time"));  
      if (TEQ[3] == 0x88) Serial.println(F("Ticket rejected due to timeout")); 
      if (TEQ[3] == 0x8B) Serial.println(F("Ticket rejected due to validator failure")); 
      if (TEQ[3] == 0xFF) Serial.println(F("No validation information available")); 
    } 
  }
  else
  {
    Serial.println(F("Ticket data was not received"));
  } 
}

void ConfirmRedeem()
{
  SendTypeS(transComplete, sizeof(transComplete));
  waitForResponse(transComplete[1], TEQ, sizeof(TEQ));

  // Report on common responses
  if (TEQ[3] == 0x00) Serial.println(F("Ticket redeemed successfully"));
  if (TEQ[3] == 0x80) Serial.println(F("Ticket rejected"));
  if (TEQ[3] == 0x81) Serial.println(F("Ticket validation number does not match"));
  if (TEQ[3] == 0x84) Serial.println(F("Transfer amount exceeded the gaming machine credit limit"));  
  if (TEQ[3] == 0x85) Serial.println(F("Transfer amount not an even multiple of gaming machine denomination"));  
  if (TEQ[3] == 0x87) Serial.println(F("Gaming machine unable to accept transfer at this time"));  
  if (TEQ[3] == 0x88) Serial.println(F("Ticket rejected due to timeout")); 
  if (TEQ[3] == 0x8B) Serial.println(F("Ticket rejected due to validator failure")); 
  if (TEQ[3] == 0xFF) Serial.println(F("No validation information available"));  
}

void SendTypeR (byte temp[], int len)
{
  uint16_t d[len];
  for (int i = 0; i < len; i++) {
      d[i] = temp[i]; // Move data to a 16 bit array
      if(i == 0) {
        d[i] = d[i] | 0xFF00; // For the address byte turn on the wake bit
      } else {
        d[i] = d[i] | 0xFC00; // For all the data turn off the wake bit
      }
  }
  Serial1.write((uint8_t * )d, sizeof(d));
}

void SendTypeS (byte temp[], int len)
{
  CalculateCRC(temp, len - 2);
  temp [len - 2] = CRCH;
  temp [len - 1] = CRCL;

  uint16_t d[len];
  for (int i = 0; i < len; i++) {
      d[i] = temp[i]; // Move data to a 16 bit array
      if(i == 0) {
        d[i] = d[i] | 0xFF00; // For the address byte turn on the wake bit
      } else {
        d[i] = d[i] | 0xFC00; // For all the data turn off the wake bit
      }
  }
  Serial1.write((uint8_t * )d, sizeof(d));
}

void CalculateCRC(byte val[], int len)
{
  CRCH = 0x00;
  CRCL = 0x00;

  long crc;
  long q;
  byte c;
  crc = 0;
  for (int i = 0; i < len; i++)
  {
    c = val[i];
    q = (crc ^ c) & 0x0f;
    crc = (crc >> 4) ^ (q * 0x1081);
    q = (crc ^ (c >> 4)) & 0xf;
    crc = (crc >> 4) ^ (q * 0x1081);
  }
  CRCH = crc & 0xff;
  CRCL = crc >> 8;
}

