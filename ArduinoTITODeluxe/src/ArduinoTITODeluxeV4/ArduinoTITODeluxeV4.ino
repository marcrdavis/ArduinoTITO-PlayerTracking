/*
  Arduino TITO Deluxe R3 Ethernet v4.0.20260623
  by Marc R. Davis - Copyright (c) 2020-2026 All Rights Reserved
  https://github.com/marcrdavis/ArduinoTITO-PlayerTracking

  Portions of the Arduino SAS protocol implementation by Ian Walker - Thank you!
  Additional testing and troubleshooting by NLG member Eddiiie - Thank you!

  Hardware requirements: 
    Arduino Uno R3; W5100 Ethernet Shield; Serial Port;

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
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT INCLUDING NEGLIGENCE OR OTHERWISE ARISING IN ANY
  WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

// Set your values here or use the config.txt on the SD card to override these defaults

bool sasError = false; // Do not change
bool changeToCredits = 0; // Set to 1 to enable Change to Credits; config file overrides this default
bool noSDCard = false; // Set true to disable SD card; will use default values; also no local UI

char changeCredits[9] = "500"; // Set the number of credits to add; config file overrides this default
char gameName[32] = "Slot Machine"; // Set this to the name of your game; config file overrides this default
const char versionString[] = "4.0.20260623"; // Do not change
const char Board[] = "DELUXE"; // Do not change

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // Change to unique address for each board; config file overrides this default
IPAddress ip(192, 168, 1, 254);  // Default address - set according to your network; Note: DHCP is not supported in this sketch due to memory limitations; config file overrides this default

#define MAX_HTTP_LINE          160
#define MAX_CONFIG_LINE         32

// ------------------------------------------------------------------------------------------------------------
// SAS Protocol Variables
// ------------------------------------------------------------------------------------------------------------

byte SASAdr = 0x01;
byte CRCH = 0x00;
byte CRCL = 0x00;

// SAS commands and response buffers moved locally to save Uno R3 SRAM.

// ------------------------------------------------------------------------------------------------------------
// Setup instances
// ------------------------------------------------------------------------------------------------------------

EthernetServer server(80);

Sd2Card sdCard;
SdVolume sdVolume;
SdFile sdRoot;

// ------------------------------------------------------------------------------------------------------------
// Reset function
// ------------------------------------------------------------------------------------------------------------

void(* resetFunc) (void) = 0;

// ------------------------------------------------------------------------------------------------------------
// Setup - called once during init
// ------------------------------------------------------------------------------------------------------------

void setup()
{
  // Setup SAS/TITO Communications
  Serial.begin(19200);
  Serial.setTimeout(200);
  pinMode(LED_BUILTIN, OUTPUT);

  // Setup SD Card / Read Config
  if (!noSDCard)
  {
    initSDCard();
    readConfig();
  }

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
// Expected format: key=value, one setting per line.
// This is intentionally minimal for Uno R3 size. No trimming or validation.

void readConfig()
{
  SdFile configFile;
  if (!configFile.open(sdRoot, "config.txt", O_READ)) return;

  char lineBuffer[MAX_CONFIG_LINE];
  byte len = 0;
  int c;

  while (1)
  {
    c = configFile.read();

    if (c >= 0 && c != '\n' && c != '\r')
    {
      if (len < sizeof(lineBuffer) - 1) lineBuffer[len++] = c;
      continue;
    }

    if (len)
    {
      lineBuffer[len] = '\0';
      len = 0;

      char *value = strchr(lineBuffer, '=');
      if (value)
      {
        *value++ = '\0';
        if (!strcmp(lineBuffer, "changeCredits")) strcpy(changeCredits, value);
        else if (!strcmp(lineBuffer, "changeToCredits")) changeToCredits = atoi(value);
        else if (!strcmp(lineBuffer, "gameName")) strcpy(gameName, value);
        else if (!strcmp(lineBuffer, "ipAddress")) ip.fromString(value);
        else if (!strcmp(lineBuffer, "macAddress"))
        {
          byte b = 0;
          byte nib = 0;

          while (*value && b < 6)
          {
            byte v;
            char c = *value++;

            if (c >= '0' && c <= '9') v = c - '0';
            else if (c >= 'A' && c <= 'F') v = c - 'A' + 10;
            else if (c >= 'a' && c <= 'f') v = c - 'a' + 10;
            else continue;

            if (nib & 1) mac[b++] |= v;
            else mac[b] = v << 4;

            nib++;
          }
        }
      }
    }

    if (c < 0) break;
  }

  configFile.close();
}

// Initialize the SD card

void initSDCard()
{
  if (!sdCard.init(SPI_HALF_SPEED, 4) || !sdVolume.init(sdCard) || !sdRoot.openRoot(sdVolume))
  {
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
  // Need version 1.0.4 of Ethernet Library due to memory constraints
  
  Ethernet.begin(mac, ip);
  delay(1000);
}

// ------------------------------------------------------------------------------------------------------------
// Game Functions
// ------------------------------------------------------------------------------------------------------------

// Add X credits to game

bool addCredits(const char *credits)
{
  unsigned long value = 0;

  while (*credits == ' ' || *credits == '\t') credits++;

  while (*credits >= '0' && *credits <= '9')
  {
    value = (value * 10) + (*credits - '0');
    credits++;
  }

  byte ac[4];
  ac[3] = dec2bcd(value % 100); value /= 100;
  ac[2] = dec2bcd(value % 100); value /= 100;
  ac[1] = dec2bcd(value % 100); value /= 100;
  ac[0] = dec2bcd(value % 100);

  return LegacyBonus(0x01, ac[0], ac[1], ac[2], ac[3], 0x00);
}

// ------------------------------------------------------------------------------------------------------------
// Misc Functions
// ------------------------------------------------------------------------------------------------------------

bool httpHasQueryParam(const char *query, const char *param)
{
  byte len = strlen(param);
  const char *p = query;

  while (p && *p)
  {
    if (!strncmp(p, param, len) && (p[len] == '=' || p[len] == '&' || p[len] == '\0')) return true;

    p = strchr(p, '&');
    if (p) p++;
  }

  return false;
}

char emptyQueryValue[] = "";

char *getQueryValue(char *query, const char *key)
{
  if (!query || !key) return emptyQueryValue;

  byte keyLen = strlen(key);
  char *p = query;

  while (p && *p)
  {
    if (strncmp(p, key, keyLen) == 0 && p[keyLen] == '=') return p + keyLen + 1;

    p = strchr(p, '&');
    if (p) p++;
  }

  return emptyQueryValue;
}

void urlDecode(char *value)
{
  char *src = value;
  char *dst = value;

  while (*src && *src != '&' && *src != ' ')
  {
    if (*src == '+')
    {
      *dst++ = ' ';
      src++;
    }
    else if (*src == '%' && src[1] && src[2])
    {
      byte h1 = src[1];
      byte h2 = src[2];

      if (h1 > '9') h1 = (h1 & 0xDF) - 'A' + 10;
      else h1 -= '0';

      if (h2 > '9') h2 = (h2 & 0xDF) - 'A' + 10;
      else h2 -= '0';

      *dst++ = (h1 << 4) | h2;
      src += 3;
    }
    else
    {
      *dst++ = *src++;
    }
  }

  *dst = '\0';
}

// ------------------------------------------------------------------------------------------------------------
// HTML Server
// ------------------------------------------------------------------------------------------------------------

void htmlPoll()
{
  EthernetClient client = server.available();
  if (!client) return;

  client.setTimeout(500);

  if (client.connected()) // If client is present and connected
  {
    bool rebootArduino = false;
    bool reqResult = false;
    bool commandFound = false;
    char requestLine[130];  // Anything greater and the app becomes unstable on the Uno R3
    memset(requestLine, 0, sizeof(requestLine));
    
    int size = client.readBytesUntil('\n', requestLine, sizeof(requestLine) - 1); // Get the first line of request
    requestLine[size] = '\0';

  // ------------------------------------------------------------
  // Special POST handling for SD card updates.
  // ------------------------------------------------------------
  if (!noSDCard && strstr(requestLine, "POST") && strstr(requestLine, "UPDATE"))
  {
    const char *fn = strstr(requestLine, "UPDATEHTML") ? "index.htm" : "config.txt";

    unsigned long contentLength = 0;
    unsigned long written = 0;
    bool headersDone = false;

    char line[48];
    byte lineLen = 0;
    unsigned long t = millis();

    // Read headers and get Content-Length
    while (client.connected() && !headersDone && millis() - t < 5000UL)
    {
      if (client.available())
      {
        char c = client.read();
        t = millis();

        if (c == '\r') continue;

        if (c == '\n')
        {
          line[lineLen] = '\0';

          if (lineLen == 0)
          {
            headersDone = true;
          }
          else if (strncmp(line, "Content-Length:", 15) == 0)
          {
            contentLength = atol(line + 15);
          }

          lineLen = 0;
        }
        else if (lineLen < sizeof(line) - 1)
        {
          line[lineLen++] = c;
        }
      }
    }

    SdFile sdFile;
    bool ok = headersDone && contentLength > 0 &&
              sdFile.open(sdRoot, fn, O_WRITE | O_CREAT | O_TRUNC);

    if (ok)
    {
      t = millis();

      while (written < contentLength && millis() - t < 5000UL)
      {
        if (client.available())
        {
          sdFile.write(client.read());
          written++;
          t = millis();
        }
      }

      sdFile.close();

      ok = (written == contentLength);
    }

    client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n"));
    client.print(ok ? F("OK") : F("ERROR"));
    client.stop();

    delay(1000);

    if (ok) resetFunc();
    return;
  }

    while (client.available()) client.read(); // Get the rest of the header and discard

    char *query = strchr(requestLine, '?');

    if (query)
    {
      char *endQuery = strchr(query, ' ');
      if (endQuery) *endQuery = '\0';
      query++;
    }

    // Parse querystring
    bool gameStatsHtml = query && httpHasQueryParam(query, "ds");
    bool gameDataText  = query && httpHasQueryParam(query, "gd");

    if (gameStatsHtml || gameDataText)
    {
      if (gameStatsHtml)
      {
        client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n<html>\r\n"));
        client.print(F("<head></head><body><h2>GAME STATISTICS</h2><br>Credits<br>"));
      }
      else
      {
        client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n"));
        client.print(gameName);
        client.print(F("|"));
      }

      client.print(pollMeters(0x1A));

      if (gameStatsHtml) client.print(F("<br>Total In<br>"));
      else client.print(F("|"));
      client.print(pollMeters(0x11));

      if (gameStatsHtml) client.print(F("<br>Total Won<br>"));
      else client.print(F("|"));
      client.print(pollMeters(0x12));

      if (gameStatsHtml) client.print(F("<br>Total Games<br>"));
      else client.print(F("|"));
      client.print(pollMeters(0x15));

      if (gameStatsHtml) client.print(F("<br>Games Won<br>"));
      else client.print(F("|"));
      client.print(pollMeters(0x16));

      if (gameStatsHtml) client.print(F("<br>Games Lost<br>"));
      else client.print(F("|"));
      client.print(pollMeters(0x17));

      if (gameStatsHtml)
      {
        client.print(F("<br></body></html>\r\n\r\n"));
      }
      else
      {
        client.print(F("|"));
        client.print(versionString);
      }

      client.stop();
      return;
    }

    if (query && httpHasQueryParam(query, "bd")) // Board
    {
      client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n"));
      client.print(Board);
      client.stop();
      return;
    }

    if (query && httpHasQueryParam(query, "pd")) // Player Data compatibility response
    {
      client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n||||||"));
      client.stop();
      return;
    }

    // All of the remaining commands return a single status or a result code

    if (query && httpHasQueryParam(query, "mo")) { reqResult = slotCommand(0x03); commandFound = true; }
    if (query && httpHasQueryParam(query, "mt")) { reqResult = slotCommand(0x04); commandFound = true; }
    if (query && httpHasQueryParam(query, "lk")) { reqResult = slotCommand(0x01); commandFound = true; }
    if (query && httpHasQueryParam(query, "uk")) { reqResult = slotCommand(0x02); commandFound = true; }
    if (query && httpHasQueryParam(query, "eb")) { reqResult = slotCommand(0x06); commandFound = true; }
    if (query && httpHasQueryParam(query, "db")) { reqResult = slotCommand(0x07); commandFound = true; }
    if (query && httpHasQueryParam(query, "jr")) { reqResult = slotCommand(0x94); commandFound = true; }
    if (query && httpHasQueryParam(query, "ec")) { changeToCredits = 1; reqResult = true; commandFound = true; }
    if (query && httpHasQueryParam(query, "dc")) { changeToCredits = 0; reqResult = true; commandFound = true; }
    if (query && httpHasQueryParam(query, "rb")) { rebootArduino = true; reqResult = true; commandFound = true; } // Reboot Arduino

  if (query && httpHasQueryParam(query, "dt")) // Set DateTime on Game
  {
    commandFound = true;

    char* datetime = getQueryValue(query, "dt");
    char datetimeValue[15];
    byte dtLen = 0;

    if (datetime)
    {
      while (dtLen < 14 &&
            datetime[dtLen] &&
            datetime[dtLen] != '&' &&
            datetime[dtLen] != ' ')
      {
        datetimeValue[dtLen] = datetime[dtLen];
        dtLen++;
      }
    }

    datetimeValue[dtLen] = '\0';

    if (dtLen == 14)
    {
      reqResult = SetDateTime(datetimeValue);
    }
    else
    {
      reqResult = false;
    }
  }

    if (!noSDCard && query && httpHasQueryParam(query, "cf")) // Request config data
    {
      client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n"));

      SdFile sdFile;

      if (sdFile.open(sdRoot, "config.txt", O_READ))
      {
        int c;

        while ((c = sdFile.read()) >= 0)
        {
          client.write((byte)c);
        }

        sdFile.close();
      }
      else
      {
        client.print(F("ERROR"));
      }

      client.stop();
      return;
    }

    if (query && httpHasQueryParam(query, "ud")) // Update Ticket Data
    {
      char *location = getQueryValue(query, "ud1");
      char *address1 = getQueryValue(query, "ud2");
      char *address2 = getQueryValue(query, "ud3");

      urlDecode(location);
      urlDecode(address1);
      urlDecode(address2);

      reqResult = SetTicketData(location, address1, address2);
      commandFound = true;
    }
   
    if (query && httpHasQueryParam(query, "cr")) // Add Credits to game
    {
      char *credits = strstr(query, "cr=");
      if (credits) reqResult = addCredits(credits + 3);
      commandFound = true;
    }

    if (commandFound)
    {        
      // Send status/result to client
      client.println(F("HTTP/1.1 200 OK\r\nConnection: close\r\n\r\n"));
      if (reqResult) client.print(F("OK"));
      else client.print(F("ERROR"));
      client.stop();
      delay(1000);
      if (rebootArduino) resetFunc();
      return;
    }

    // No command found

    if (strstr(requestLine, "GET / ") || strstr(requestLine, "GET /?"))
    {
      if (noSDCard)
      {
        client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n"));
        client.print(F("UI is not available."));
      }
      else
      {
        // Load Web UI from local SD card
        SdFile sdFile;

        if (sdFile.open(sdRoot, "index.htm", O_READ))
        {
          client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n"));
          client.print(F("<script>history.replaceState(null,'','/?ip="));
          client.print(ip);
          client.print(F("&board="));
          client.print(Board);
          client.print(F("&gn="));
          client.print(gameName);
          client.print(F("&cp=&v="));
          client.print(versionString);
          client.print(F("');</script>"));

          int c;

          while ((c = sdFile.read()) >= 0)
          {
            client.write((byte)c);
          }

          sdFile.close();
        }
        else
        {
          // UI not available from SD card
          client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n"));
          client.print(F("Unable to load UI."));
        }
      }

      client.stop();
      delay(1000);
      return;
    }
    else
    {
      client.print(F("HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n"));
      client.stop();
      delay(1000);
      return;
    }
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

  UCSR0B = 0b10011101;
  Serial.write(0x80);
  delay(20);
  Serial.write(0x81);
  UCSR0B = 0b10011100;

  delay(10);  // Wait for data on the serial bus
  if (Serial.available() > 0) {
    eventCode = Serial.read();
  }

  switch (eventCode) {
    case 0x71:
    case 0x72:
      if (changeToCredits) {
        addCredits(changeCredits);
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

void waitForResponse(byte &waitfor, byte *ret, int sz)
{
  sasError = false;
  memset(ret, 0, sz);

  if (sz < 2)
  {
    sasError = true;
    return;
  }

  const unsigned long timeoutMs = 3000;
  unsigned long startTime = millis();

  bool headerFound = false;
  byte previousByte = 0x00;

  while ((millis() - startTime) < timeoutMs)
  {
    if (Serial.available() > 0)
    {
      byte currentByte = Serial.read();

      if (previousByte == SASAdr && currentByte == waitfor)
      {
        ret[0] = SASAdr;
        ret[1] = waitfor;
        headerFound = true;
        break;
      }

      previousByte = currentByte;
    }
    else
    {
      delay(1);
    }
  }

  if (!headerFound)
  {
    sasError = true;
    return;
  }

  int bytesNeeded = sz - 2;
  int bytesRead = 0;
  startTime = millis();

  while (bytesRead < bytesNeeded && (millis() - startTime) < timeoutMs)
  {
    if (Serial.available() > 0)
    {
      ret[bytesRead + 2] = Serial.read();
      bytesRead++;
    }
    else
    {
      delay(1);
    }
  }

  if (bytesRead < bytesNeeded)
  {
    memset(ret, 0, sz);
    sasError = true;
    return;
  }
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

long pollMeters(byte meterCmd)
{
  byte meter[2] = {SASAdr, meterCmd};
  byte meterData[8];

  SendTypeR(meter, sizeof(meter));
  waitForResponse(meterCmd, meterData, sizeof(meterData));

  long value = 0;

  for (byte i = 2; i < 6; i++)
  {
    value = (value * 100) + bcd2dec(meterData[i]);
  }

  return value;
}

bool slotCommand(byte cmd)
{
  byte data[4] = {SASAdr, cmd, 0x00, 0x00};
  SendTypeS(data, sizeof(data));
  return waitForACK(SASAdr);
}

void SystemValidation()
{
  byte SVNS[2] = {SASAdr, 0x57};
  byte COT[10];
  byte SVN[13];
  byte COS[5];

  // Retry up to 2 times

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

bool SetTicketData(const char *loc, const char *addr1, const char *addr2)
{
  byte locLen = strlen(loc);
  byte addr1Len = strlen(addr1);
  byte addr2Len = strlen(addr2);
  byte ticketData[11 + locLen + addr1Len + addr2Len];
  byte TDR[5];

  ticketData[0] = SASAdr;
  ticketData[1] = 0x7D;
  ticketData[2] = 6 + locLen + addr1Len + addr2Len;
  ticketData[3] = 0x00;
  ticketData[4] = 0x00;
  ticketData[5] = 0x00;
  ticketData[6] = locLen;

  memcpy(ticketData + 7, loc, locLen);
  ticketData[7 + locLen] = addr1Len;
  memcpy(ticketData + 8 + locLen, addr1, addr1Len);
  ticketData[8 + locLen + addr1Len] = addr2Len;
  memcpy(ticketData + 9 + locLen + addr1Len, addr2, addr2Len);

  SendTypeS(ticketData, sizeof(ticketData));
  waitForResponse(ticketData[1], TDR, sizeof(TDR));
  return true;
}

void CashOutState()
{
  byte EVInfo[5] = {SASAdr, 0x4D, 0x00, 0x00, 0x00};
  byte TPS[35];

  SendTypeS(EVInfo, sizeof(EVInfo));
  waitForResponse(EVInfo[1], TPS, sizeof(TPS));
}

byte asciiPairToBcd(const char* p)
{
  return (byte)(((p[0] - '0') << 4) | (p[1] - '0'));
}

bool SetDateTime(const char* datetime)
{
  byte TIM[11];
  if (!datetime || datetime[0] == '\0') return false;

  TIM[0] = SASAdr;
  TIM[1] = 0x7F;

  for (byte i = 0; i < 7; i++)
  {
    TIM[i + 2] = asciiPairToBcd(datetime + (i * 2));
  }

  SendTypeS(TIM, sizeof(TIM));
  return waitForACK(SASAdr);
}

bool LegacyBonus (byte SASAdr, byte Amount1, byte Amount2, byte Amount3, byte Amount4, byte Type)
{
  byte LBS[9];

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
  byte TP[2] = {SASAdr, 0x70};
  byte TEQ[21];
  byte TRS[21];

  // Retry up to 2 times

  for (int x = 0; x < 2; x++) {
    SendTypeR(TP, sizeof(TP));
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
    waitForResponse(TRS[1], TEQ, sizeof(TEQ));
  }
}

void ConfirmRedeem()
{
  byte transComplete[6] = {SASAdr, 0x71, 0x01, 0xFF, 0x1F, 0xD0};
  byte TEQ[21];

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
