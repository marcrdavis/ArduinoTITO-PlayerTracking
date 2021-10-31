/*
  Arduino TITO Deluxe v2.0.20211031D
  by Marc R. Davis - Copyright (c) 2020-2021 All Rights Reserved
  https://github.com/marcrdavis/ArduinoTITO-PlayerTracking

  Portions of the Arduino SAS protocol implementation by Ian Walker - Thank you!
  Additional testing and troubleshooting by NLG member Eddiiie - Thank you!

  Hardware requirements: 
    Arduino Uno; W5100 Ethernet Shield; Serial Port;

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

#include <SPI.h>
#include <SD.h>
#include <Ethernet.h>

// ------------------------------------------------------------------------------------------------------------
// Core Variables
// ------------------------------------------------------------------------------------------------------------

long Credits = 0;
long totalIn = 0;
long totalWon = 0;
long totalGames = 0;
long gamesWon = 0;
long gamesLost = 0;

bool changeToCredits = 0;
bool sasError = false;
bool isLocked = false;

String changeCredits = "100";
String stringData = "";
String url = "";

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // Change to unique address for each board
IPAddress ip(192, 168, 1, 254);  // Default address - set according to your network

// ------------------------------------------------------------------------------------------------------------
// SAS Protocol Variables
// ------------------------------------------------------------------------------------------------------------

byte SASAdr = 0x01;
byte CRCH = 0x00;
byte CRCL = 0x00;
byte SASEvent [1];
byte returnStatus[1];

byte SVNS[2] = {SASAdr, 0x57};
byte TP[2] = {SASAdr, 0x70};
byte HPI[2] = {SASAdr, 0x1B};
byte LOCK[4] = {SASAdr, 0x01, 0x00, 0x00};
byte ULOCK[4] = {SASAdr, 0x02, 0x00, 0x00};
byte MUTE[4] = {SASAdr, 0x03, 0x00, 0x00};
byte UMUTE[4] = {SASAdr, 0x04, 0x00, 0x00};
byte EVInfo[5] = {SASAdr, 0x4D, 0x00, 0x00, 0x00};
byte transComplete[6] = {SASAdr, 0x71, 0x01, 0xFF, 0x1F, 0xD0};
byte mCredits[2] = {SASAdr, 0x1A};
byte mCoinIn[2] = {SASAdr, 0x11};
byte mTotWon[2] = {SASAdr, 0x12};
byte mTotGames[2] = {SASAdr, 0x15};
byte mgamesWon[2] = {SASAdr, 0x16};
byte mgamesLost[2] = {SASAdr, 0x17};

byte LBS [9];
byte PCR [10];
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

File sdFile;
EthernetServer server(80);

// ------------------------------------------------------------------------------------------------------------
// Setup - called once during init
// ------------------------------------------------------------------------------------------------------------

void setup()
{
  // Variable Reserves
  changeCredits.reserve(5);
  stringData.reserve(240);
  url.reserve(15);
  
  // Setup SAS/TITO Communications
  Serial.begin(19200);
  Serial.setTimeout(200);
  pinMode(LED_BUILTIN, OUTPUT);

  // Setup SD Card
  initSDCard();

  // Read Config
  readConfig();
  
  // Initialize Ethernet
  initEthernet();

  // Initialize HTTP Server
  server.begin();
  
  // Clear the game serial buffer
  if (Serial.available() > 3) while (Serial.available() > 3) Serial.read();  
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
// IO Functions
// ------------------------------------------------------------------------------------------------------------

// Read the configuration from SD card

void readConfig()
{
  sdFile = SD.open("config.txt");
  
  if (!sdFile)
  {
    Serial.println(F("Failed to read config.txt"));
    return;
  }

  while (sdFile.available()) 
  {
    stringData = sdFile.readStringUntil('\r');
    if (getValue(stringData, '=', 0) == "changeCredits") changeCredits = getValue(stringData, '=', 1);
    if (getValue(stringData, '=', 0) == "changeToCredits") changeToCredits = getValue(stringData, '=', 1).toInt();
    if (getValue(stringData, '=', 0) == "ipAddress")  ip.fromString(getValue(stringData, '=', 1));
    stringData = sdFile.readStringUntil('\n');
  }
  
  sdFile.close();
}

// Initialize the SD card

void initSDCard()
{
  if (!SD.begin(4))
  {
    Serial.println(F("SD Init Failed"));
    while (1);
  }
}

// ------------------------------------------------------------------------------------------------------------
// Network Functions
// ------------------------------------------------------------------------------------------------------------

// Initialize the Ethernet Shield

void initEthernet()
{
  // Start the Ethernet connection and the server
  Ethernet.begin(mac, ip);
  ip = Ethernet.localIP();
  delay(1000);
}

// ------------------------------------------------------------------------------------------------------------
// Game Functions
// ------------------------------------------------------------------------------------------------------------

// Add X credits to game

bool addCredits(String credits)
{
  String paddedValue = "";
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

// Read the game meters

bool readGameData()
{
  sasError = false;

  Credits = pollMeters(mCredits);
  delay(100);
  totalIn = pollMeters(mCoinIn);
  delay(100);
  totalWon = pollMeters(mTotWon);
  delay(100);
  totalGames = pollMeters(mTotGames);
  delay(100);
  gamesWon = pollMeters(mgamesWon);
  delay(100);
  gamesLost = pollMeters(mgamesLost);

  return !sasError;
}

// ------------------------------------------------------------------------------------------------------------
// Misc Functions
// ------------------------------------------------------------------------------------------------------------

// Get a value in a string of data split by a separator character

String getValue(const String &data, char separator, int index)
{
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++)
  {
    if (data.charAt(i) == separator || i == maxIndex)
    {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
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
  EthernetClient client = server.available();
  if (!client) return;
  
  if (client.connected()) // If client is present and connected
  {
    bool reqResult = false;
    stringData = "";
    
    stringData = client.readStringUntil('\n'); // Get the first line of request
    
    // Check for known GET commands
    while (client.available()) client.read(); // Get the rest of the header and discard

    url = getValue(getValue(stringData, ' ', 1), '?', 0);
    stringData = getValue(getValue(stringData, ' ', 1), '?', 1);

    // Parse querystring
    // Uses and expands on codes from BETTORSlots TITO for compatibility with remote app
    String command = stringData.substring(0, 2);
 
    if (url.equals("/") && command != "")
    {
      if (command == "ds") // Game Statistics
      {
        client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n<html>\r\n"));
        if (readGameData())
        {
          client.print(F("<head></head><body><h2>GAME STATISTICS</h2><br>"));
          client.print(F("Credits<br>"));
          client.print(Credits);
          client.print(F("<br>Total In<br>"));
          client.print(totalIn);
          client.print(F("<br>Total Won<br>"));
          client.print(totalWon);
          client.print(F("<br>Total Games<br>"));
          client.print(totalGames);
          client.print(F("<br>Games Won<br>"));
          client.print(gamesWon);
          client.print(F("<br>Games Lost<br>"));
          client.print(gamesLost);
          client.print(F("<br>"));        
        }
        else
        {
          client.print(F("<head><body><h2>GAME STATISTICS NOT AVAILABLE</h2><br>"));
        }

        client.print(F("</body>"));
        client.print(F("</html>\r\n\r\n"));
        client.stop();
        return;
      }

      // All of the remaining commands return a single status or a result code
      
      if (command == "mo") reqResult=mute();
      if (command == "mt") reqResult=unMute();
      if (command == "lk") reqResult=lockMachine();
      if (command == "uk") reqResult=unlockMachine();
      
      if (command == "ud") // Update Ticket Data
      {
        String location = urlDecode(getValue(getValue(stringData, '&', 0), '=', 1));
        String address1 = urlDecode(getValue(getValue(stringData, '&', 1), '=', 1));
        String address2 = urlDecode(getValue(getValue(stringData, '&', 2), '=', 1));

        reqResult=SetTicketData(location, address1, address2);
      }
     
      if (command == "cr")  // Add Credits to game
      {
        String credits = getValue(getValue(stringData, '&', 1), '=', 1);
        reqResult=addCredits(credits);
      }
      
      // Send status/result to client
      client.println(F("HTTP/1.1 200 OK\r\n\r\n"));
      if (reqResult) client.print(F("OK"));
      else client.print(F("ERROR"));
    }
    else if (url.equals("/"))
    {
      // Show web interface
      sdFile = SD.open("index.htm", FILE_READ);
      if (sdFile)
      {
        client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n"));

        while (sdFile.available()) {
          client.write(sdFile.read());
        }
        client.print(F("</html>\r\n\r\n"));
        sdFile.close();
      }
      else
      {
        // UI Not available (SD Card error?)
        client.print(F("HTTP/1.1 503 Service not available\r\n\r\n"));
      }
    }
    else
    {
      // Everything else is a 404
      client.print(F("HTTP/1.1 404 Not Found\r\n\r\n"));
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
  SASEvent[0] = 0x00;

  UCSR0B = 0b10011101;
  Serial.write(0x80);
  delay(20);
  Serial.write(0x81);
  UCSR0B = 0b10011100;

  delay(10);  // Found to be necessary on some machines to wait for data on the serial bus
  if (Serial.available() > 0) Serial.readBytes(SASEvent, sizeof(SASEvent));

  if (SASEvent[0] != 0x1F && SASEvent[0] != 0x00 && SASEvent[0] != 0x01 && SASEvent[0] != 0x80 && SASEvent[0] != 0x81 && SASEvent[0] != 0x7C) {

    // Process/log these events
    if (SASEvent[0] == 0x71 & changeToCredits) addCredits(changeCredits); // To enable 'Change button' credits
    if (SASEvent[0] == 0x72 & changeToCredits) addCredits(changeCredits); // To enable 'Change button' credits
    if (SASEvent[0] == 0x51) getHandpayInfo();
    if (SASEvent[0] == 0x57) SystemValidation();
    if (SASEvent[0] == 0x3D) CashOutState();
    if (SASEvent[0] == 0x67) RedeemTicket();
    if (SASEvent[0] == 0x68) ConfirmRedeem();
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

  while (Serial.read() != waitfor && wait < 3000) {
    delay(1);
    wait += 1;
  }

  if (wait >= 3000) {
    memset(ret, 0, sz);
    sasError = true;
    return;
  }

  Serial.readBytes(responseBytes, sizeof(responseBytes));
  ret[0] = {0x01};
  ret[1] = waitfor;
  memcpy(ret + 2, responseBytes, sizeof(responseBytes));

  return;
}

bool waitForACK(byte waitfor)
{
  int wait = 0;

  while (Serial.read() != waitfor && wait < 3000) {
    delay(1);
    wait += 1;
  }

  if (wait >= 3000) {
      return false;
  }
  
  return true;
}

long pollMeters(byte * meter)
{
  byte meterData[8];

  SendTypeR(meter, sizeof(meter));
  waitForResponse(meter[1], meterData, sizeof(meterData));

  String sMeter;
  for (int i = 2; i < 6; i++) {
    sMeter.concat(String(bcd2dec(meterData[i]) < 10 ? "0" : ""));
    sMeter.concat(String(bcd2dec(meterData[i])));
  }
  return sMeter.toInt();
}

bool lockMachine()
{
  SendTypeS(LOCK, sizeof(LOCK));
  isLocked=true;
  return waitForACK(SASAdr);
}

bool unlockMachine()
{
  SendTypeS(ULOCK, sizeof(LOCK));
  isLocked=false;
  return waitForACK(SASAdr);
}

bool mute()
{
  SendTypeS(MUTE, sizeof(MUTE));
  return waitForACK(SASAdr);
}

bool unMute()
{
  SendTypeS(UMUTE, sizeof(UMUTE));
  return waitForACK(SASAdr);
}

void getHandpayInfo()
{
  SendTypeR(HPI, sizeof(HPI));
  waitForResponse(HPI[1], HPS, sizeof(HPS));  
}

void SystemValidation()
{
  // Retry up to 2 times

  for (int x = 0; x < 2; x++) {
    SendTypeR(SVNS, sizeof(SVNS));
    sasError = false;
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
      SVN [2] = 0x00;
      SendTypeS(SVN, sizeof(SVN));
      waitForResponse(SVN[1], COS, sizeof(COS));
    }
    else
    {
      SendTypeS(SVN, sizeof(SVN));
      waitForResponse(SVN[1], COS, sizeof(COS));     
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
  return true;
}

void CashOutState()
{
  SendTypeS(EVInfo, sizeof(EVInfo));
  waitForResponse(EVInfo[1], TPS, sizeof(TPS));
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
  return waitForACK(SASAdr);
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
  return waitForACK(SASAdr);
}

void RedeemTicket()
{
  // Retry up to 2 times
  
  for (int x = 0; x < 2; x++) {
    SendTypeR(TP, sizeof(TP));
    sasError = false;
    waitForResponse(TP[1], TEQ, sizeof(TEQ));
    if (!sasError) break;
  }
  
  if (!sasError)
  {      
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
  
    SendTypeS(TRS, sizeof(TRS));  
    sasError = false;
    waitForResponse(TRS[1], TEQ, sizeof(TEQ));
  }
}

void ConfirmRedeem()
{
  SendTypeS(transComplete, sizeof(transComplete));
  waitForResponse(transComplete[1], TEQ, sizeof(TEQ));
}

void SendTypeR (byte temp[], int len)
{
  UCSR0B = 0b10011101;
  Serial.write(temp[0]);
  UCSR0B = 0b10011100;

  for (int i = 1; i < len; i++) Serial.write(temp[i]);
}

void SendTypeS (byte temp[], int len)
{
  CalculateCRC(temp, len - 2);
  temp [len - 2] = CRCH;
  temp [len - 1] = CRCL;
  UCSR0B = 0b10011101;
  Serial.write(temp[0]);
  UCSR0B = 0b10011100;

  for (int i = 1; i < len; i++) Serial.write(temp[i]);
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
