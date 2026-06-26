/*
  Arduino TITO Deluxe R4 v4.0.20260623
  by Marc R. Davis - Copyright (c) 2020-2026 All Rights Reserved
  https://github.com/marcrdavis/ArduinoTITO-PlayerTracking

  Portions of the Arduino SAS protocol implementation by Ian Walker - Thank you!
  Additional testing and troubleshooting by NLG member Eddiiie - Thank you!
  9-Bit code by j.DeLutis and vashadow - Thank you!

  Hardware requirements: 
  Arduino Uno R4 WiFi; Serial Port, SD Card Reader or Shield (optional)

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
#include "WiFiS3.h"
#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"
#include <Arduino.h>

// ------------------------------------------------------------------------------------------------------------
// Core Variables
// ------------------------------------------------------------------------------------------------------------

const uint8_t SD_CS_PIN = 4;
const unsigned long WIFI_RETRY_DELAY_MS = 3000;
const size_t CONFIG_LINE_BUFFER_SIZE = 192;
const size_t WIFI_SSID_BUFFER_SIZE = 33;
const size_t WIFI_PASS_BUFFER_SIZE = 65;
const size_t CHANGE_CREDITS_BUFFER_SIZE = 16;
const size_t GAME_NAME_BUFFER_SIZE = 64;
const size_t VERSION_STRING_BUFFER_SIZE = 24;
const size_t HTTP_QUERY_VALUE_BUFFER_SIZE = 128;

// Set your values here or use the config.txt on the SD card to override these defaults

char gameName[GAME_NAME_BUFFER_SIZE] = "Slot Machine"; // Set this to the name of your game
char versionString[VERSION_STRING_BUFFER_SIZE] = "4.0.20260623"; // Do not change
char changeCredits[CHANGE_CREDITS_BUFFER_SIZE] = "500"; // Set the number of credits to add 
char ssid[WIFI_SSID_BUFFER_SIZE] = "DAVWRT1900AC"; // Set your WiFi SSID here
char pass[WIFI_PASS_BUFFER_SIZE] = "enterprise"; // Set your WiFi Password here

bool sasOnline = false; // Do not change
bool sasError = false; // Do not change
bool changeToCredits = 0; // Set to 1 to enable Change to Credits
bool useDHCP = 0; // Set to 1 to enable DHCP
bool noSDCard = false; // Set true to disable all SD card operations; automatically set true if SD is not detected

int status = WL_IDLE_STATUS;

IPAddress ip(192, 168, 1, 254); // Set the IP address of the board - change if your network addressing is different or if you have multiple machines

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

  // Setup SD Card
  initSDCard();

  // Read Config
  readConfig();

  // Initialize WiFi
  initWiFi();

  Serial.print(F("Arduino TITO Deluxe R4 WiFi - By Marc R. Davis - Version ")); Serial.println(versionString);
 
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
// SD Card
// ------------------------------------------------------------------------------------------------------------

// Initialize the SD card
void initSDCard()
{
  if (noSDCard)
  {
    Serial.println(F("SD card disabled by configuration; using default configuration"));
    return;
  }

  if (!SD.begin(SD_CS_PIN))
  {
    noSDCard = true;
    Serial.println(F("SD card not available; using default configuration"));
    return;
  }

  Serial.println(F("SD Init OK"));
}

char* trimInPlace(char *s)
{
  while (*s == ' ' || *s == '\t') s++;

  char *end = s + strlen(s);

  while (end > s && (*(end - 1) == ' ' || *(end - 1) == '\t' || *(end - 1) == '\r' || *(end - 1) == '\n'))
  {
    *(--end) = '\0';
  }

  return s;
}


static bool copyConfigValue(char *destination, size_t destinationSize, const char *value, const __FlashStringHelper *settingName)
{
  if (destination == NULL || destinationSize == 0 || value == NULL) return false;

  if (value[0] == '\0')
  {
    Serial.print(F("Ignoring blank config value for "));
    Serial.println(settingName);
    return false;
  }

  size_t valueLen = strlen(value);

  if (valueLen >= destinationSize)
  {
    Serial.print(F("Ignoring overlength config value for "));
    Serial.print(settingName);
    Serial.print(F("; max length is "));
    Serial.println(destinationSize - 1);
    return false;
  }

  strncpy(destination, value, destinationSize - 1);
  destination[destinationSize - 1] = '\0';
  return true;
}

static bool parseBoolConfigValue(const char *value)
{
  return atoi(value) != 0;
}

// Read the configuration from SD card
void readConfig()
{
  if (noSDCard)
  {
    Serial.println(F("Skipping config.txt; SD card not available"));
    return;
  }

  File configFile = SD.open("config.txt", FILE_READ);

  if (!configFile)
  {
    Serial.println(F("Failed to read config.txt; using default configuration"));
    return;
  }

  char lineBuffer[CONFIG_LINE_BUFFER_SIZE];

  while (configFile.available())
  {
    int len = configFile.readBytesUntil('\n', lineBuffer, sizeof(lineBuffer) - 1);
    lineBuffer[len] = '\0';
    // If the line exceeded the buffer, drain the rest of that line.
    if (len >= (int)(sizeof(lineBuffer) - 1))
    {
      while (configFile.available())
      {
        int c = configFile.read();
        if (c == '\n') break;
      }
    }

    char *line = trimInPlace(lineBuffer);

    if (line[0] == '\0' || line[0] == '#') continue;

    char *equals = strchr(line, '=');
    if (!equals) continue;

    *equals = '\0';

    char *key = trimInPlace(line);
    char *value = trimInPlace(equals + 1);

    if (strcmp(key, "changeCredits") == 0)
    {
      copyConfigValue(changeCredits, sizeof(changeCredits), value, F("changeCredits"));
    }
    else if (strcmp(key, "gameName") == 0)
    {
      copyConfigValue(gameName, sizeof(gameName), value, F("gameName"));
    }
    else if (strcmp(key, "changeToCredits") == 0)
    {
      changeToCredits = parseBoolConfigValue(value);
    }
    else if (strcmp(key, "useDHCP") == 0)
    {
      useDHCP = parseBoolConfigValue(value);
    }
    else if (strcmp(key, "ssid") == 0)
    {
      copyConfigValue(ssid, sizeof(ssid), value, F("ssid"));
    }
    else if (strcmp(key, "pass") == 0)
    {
      copyConfigValue(pass, sizeof(pass), value, F("pass"));
    }
    else if (strcmp(key, "ipAddress") == 0)
    {
      IPAddress parsedIp;

      if (parsedIp.fromString(value))
      {
        ip = parsedIp;
      }
      else
      {
        Serial.print(F("Invalid ipAddress in config.txt ignored: "));
        Serial.println(value);
      }
    }
  }

  configFile.close();
}

// ------------------------------------------------------------------------------------------------------------
// Network Functions
// ------------------------------------------------------------------------------------------------------------

// Initialize the WiFi
void initWiFi()
{
  if (strlen(ssid) == 0 || strlen(pass) == 0)
  {
    Serial.println(F("SSID or Password not set in configuration!"));
    // don't continue
    matrixPrint("ERR");
    while (true);
  }

  // Check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE)
  {
    Serial.println(F("Communication with WiFi module failed!"));
    // don't continue
    matrixPrint("ERR");
    while (true);
  }

  // Attempt to connect to WiFi network:
  while (status != WL_CONNECTED)
  {
    Serial.print(F("Attempting to connect to SSID: "));
    Serial.println(ssid);

    if (!useDHCP) WiFi.config(ip);
    status = WiFi.begin(ssid, pass);

    // Wait before retrying connection:
    delay(WIFI_RETRY_DELAY_MS);
  }

  // Connection Information
  Serial.print(F("SSID: "));
  Serial.println(WiFi.SSID());

  IPAddress localIp = WiFi.localIP();
  Serial.print(F("IP Address: "));
  Serial.println(localIp);

  long rssi = WiFi.RSSI();
  Serial.print(F("Signal Strength: "));
  Serial.print(rssi);
  Serial.println("dBm");
}

// ------------------------------------------------------------------------------------------------------------
// Game Functions
// ------------------------------------------------------------------------------------------------------------

// Add X credits to game

bool addCredits(const char *creditsInput)
{
  Serial.println(F("Add Credits"));

  if (creditsInput == NULL)
  {
    Serial.println(F("Invalid credit value: null"));
    return false;
  }

  while (*creditsInput == ' ' || *creditsInput == '\t' || *creditsInput == '\r' || *creditsInput == '\n')
  {
    creditsInput++;
  }

  char digits[9];
  size_t digitCount = 0;

  while (*creditsInput && *creditsInput != ' ' && *creditsInput != '\t' && *creditsInput != '\r' && *creditsInput != '\n')
  {
    if (*creditsInput < '0' || *creditsInput > '9')
    {
      Serial.println(F("Invalid credit value: non-numeric"));
      return false;
    }

    if (digitCount >= 8)
    {
      Serial.println(F("Invalid credit value: more than 8 digits"));
      return false;
    }

    digits[digitCount++] = *creditsInput++;
  }

  while (*creditsInput)
  {
    if (*creditsInput != ' ' && *creditsInput != '\t' && *creditsInput != '\r' && *creditsInput != '\n')
    {
      Serial.println(F("Invalid credit value: trailing non-whitespace data"));
      return false;
    }

    creditsInput++;
  }

  if (digitCount == 0)
  {
    Serial.println(F("Invalid credit value: blank"));
    return false;
  }

  digits[digitCount] = '\0';

  char paddedValue[9];
  size_t padCount = 8 - digitCount;

  for (size_t i = 0; i < padCount; i++)
  {
    paddedValue[i] = '0';
  }

  for (size_t i = 0; i < digitCount; i++)
  {
    paddedValue[padCount + i] = digits[i];
  }

  paddedValue[8] = '\0';

  byte ac[4];

  for (int i = 0; i < 4; i++)
  {
    byte highDigit = paddedValue[i * 2] - '0';
    byte lowDigit = paddedValue[(i * 2) + 1] - '0';
    ac[i] = dec2bcd((highDigit * 10) + lowDigit);
  }

  return LegacyBonus(0x01, ac[0], ac[1], ac[2], ac[3], 0x00);
}

bool addCredits(String credits)
{
  return addCredits(credits.c_str());
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

// Get a querystring value without copying the full querystring or using dynamic String allocation

static bool getQueryValue(const char* queryString, const char* key, char *output, size_t outputSize)
{
  if (output == NULL || outputSize == 0) return false;
  output[0] = '\0';

  if (queryString == NULL || key == NULL || key[0] == '\0') return false;

  size_t requestedKeyLength = strlen(key);
  const char *pairStart = queryString;

  while (*pairStart && *pairStart != ' ')
  {
    const char *pairEnd = pairStart;

    while (*pairEnd && *pairEnd != '&' && *pairEnd != ' ')
    {
      pairEnd++;
    }

    const char *equals = pairStart;

    while (equals < pairEnd && *equals != '=')
    {
      equals++;
    }

    if (equals < pairEnd)
    {
      size_t currentKeyLength = equals - pairStart;

      if (currentKeyLength == requestedKeyLength && strncmp(pairStart, key, requestedKeyLength) == 0)
      {
        const char *valueStart = equals + 1;
        size_t valueLength = pairEnd - valueStart;

        if (valueLength >= outputSize)
        {
          output[0] = '\0';
          return false;
        }

        memcpy(output, valueStart, valueLength);
        output[valueLength] = '\0';
        return true;
      }
    }

    pairStart = pairEnd;

    if (*pairStart == '&')
    {
      pairStart++;
    }
  }

  return false;
}

// Used by urlDecodeToBuffer

static bool isHexDigit(char c)
{
  return ((c >= '0' && c <= '9') ||
          (c >= 'a' && c <= 'f') ||
          (c >= 'A' && c <= 'F'));
}

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

// Decode querystring parameters into a fixed output buffer.
// Malformed percent-encoding is copied literally instead of reading beyond the input.

static bool urlDecodeToBuffer(const char *input, char *output, size_t outputSize)
{
  if (output == NULL || outputSize == 0) return false;
  output[0] = '\0';

  if (input == NULL) return false;

  size_t outputPos = 0;

  while (*input)
  {
    if (outputPos >= outputSize - 1)
    {
      output[0] = '\0';
      return false;
    }

    if (*input == '+')
    {
      output[outputPos++] = ' ';
      input++;
    }
    else if (*input == '%' && isHexDigit(*(input + 1)) && isHexDigit(*(input + 2)))
    {
      output[outputPos++] = (char)((h2int(*(input + 1)) << 4) | h2int(*(input + 2)));
      input += 3;
    }
    else
    {
      output[outputPos++] = *input++;
    }

    yield();
  }

  output[outputPos] = '\0';
  return true;
}

// Print a value safely for use inside a browser querystring.
// This replaces the old String-based cleanQueryValue() helper.

static void httpPrintEncodedQueryValue(WiFiClient &client, const char *value)
{
  if (value == NULL) return;

  while (*value)
  {
    switch (*value)
    {
      case '%': client.print(F("%25")); break;
      case ' ': client.print(F("%20")); break;
      case '&': client.print(F("%26")); break;
      case '#': client.print(F("%23")); break;
      case '\'': client.print(F("%27")); break;
      case '"': client.print(F("%22")); break;
      case '?': client.print(F("%3F")); break;
      case '=': client.print(F("%3D")); break;
      default: client.write((uint8_t)*value); break;
    }

    value++;
  }
}

// ------------------------------------------------------------------------------------------------------------
// HTML Server
// ------------------------------------------------------------------------------------------------------------

static bool httpReadLine(WiFiClient &client, char *buffer, size_t bufferSize, unsigned long timeoutMs)
{
  if (buffer == NULL || bufferSize == 0) return false;

  size_t pos = 0;
  unsigned long lastActivity = millis();

  while ((millis() - lastActivity) < timeoutMs)
  {
    while (client.available() > 0)
    {
      char c = client.read();
      lastActivity = millis();

      if (c == '\n')
      {
        buffer[pos] = '\0';
        return true;
      }

      if (c != '\r')
      {
        if (pos < bufferSize - 1)
        {
          buffer[pos++] = c;
        }
      }
    }

    if (!client.connected()) break;
    delay(1);
  }

  buffer[pos] = '\0';
  return (pos > 0);
}

static bool httpStartsWithIgnoreCase(const char *value, const char *prefix)
{
  while (*prefix && *value)
  {
    char a = *value;
    char b = *prefix;

    if (a >= 'A' && a <= 'Z') a += 32;
    if (b >= 'A' && b <= 'Z') b += 32;

    if (a != b) return false;

    value++;
    prefix++;
  }

  return (*prefix == '\0');
}

static int httpReadHeadersGetContentLength(WiFiClient &client)
{
  char headerLine[128];
  int contentLength = 0;

  while (httpReadLine(client, headerLine, sizeof(headerLine), 3000))
  {
    if (headerLine[0] == '\0') break;

    if (httpStartsWithIgnoreCase(headerLine, "Content-Length:"))
    {
      char *p = headerLine + 15;
      while (*p == ' ' || *p == '\t') p++;
      contentLength = atoi(p);
    }
  }

  return contentLength;
}

static void httpDiscardHeaders(WiFiClient &client)
{
  char headerLine[128];

  while (httpReadLine(client, headerLine, sizeof(headerLine), 3000))
  {
    if (headerLine[0] == '\0') break;
  }
}

static bool httpHasQueryParam(const char *requestLine, const char *key)
{
  const char *q = strchr(requestLine, '?');
  if (q == NULL) return false;

  q++;

  size_t keyLen = strlen(key);

  while (*q && *q != ' ')
  {
    if (strncmp(q, key, keyLen) == 0)
    {
      char nextChar = q[keyLen];

      // Valid matches:
      //   /?gd
      //   /?gd=1
      //   /?gd&cf
      //   /?gd HTTP/1.1
      //
      // Invalid matches:
      //   /?gdx
      //   /?gdExtra=1
      if (nextChar == '=' || nextChar == '&' || nextChar == ' ' || nextChar == '\0')
      {
        return true;
      }
    }

    q = strchr(q, '&');
    if (q == NULL) break;

    q++;
  }

  return false;
}

static void httpExtractQueryString(const char *requestLine, char *queryString, size_t queryStringSize)
{
  if (queryString == NULL || queryStringSize == 0) return;

  queryString[0] = '\0';

  const char *q = strchr(requestLine, '?');
  if (q == NULL) return;

  q++;
  size_t pos = 0;

  while (*q && *q != ' ' && pos < queryStringSize - 1)
  {
    queryString[pos++] = *q++;
  }

  queryString[pos] = '\0';
}

static void httpPrintUiQueryParams(WiFiClient &client)
{
  client.print(ip);
  client.print(F("&board=DELUXE&gn="));
  httpPrintEncodedQueryValue(client, gameName);
  client.print(F("&cp=&v="));
  httpPrintEncodedQueryValue(client, versionString);
}

static void httpSendOkText(WiFiClient &client, const __FlashStringHelper *text)
{
  client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n"));
  client.print(text);
}

static void httpPrintInCodeConfigValues(WiFiClient &client)
{
  client.print(F("changeCredits="));
  client.println(changeCredits);
  client.print(F("gameName="));
  client.println(gameName);
  client.print(F("changeToCredits="));
  client.println(changeToCredits ? F("1") : F("0"));
  client.print(F("useDHCP="));
  client.println(useDHCP ? F("1") : F("0"));
  client.print(F("ipAddress="));
  client.println(ip);
  client.print(F("ssid="));
  client.println(ssid);
  client.print(F("pass="));
  client.println(strlen(pass) > 0 ? F("[hidden]") : F(""));
}

static void httpServeLocalUi(WiFiClient &client)
{
  if (noSDCard)
  {
    client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n"));
    client.println(F("SD card is not available."));
    client.println(F("Local User Interface cannot be loaded."));
    client.println(F("The board is using these default configuration values:"));
    client.println();
    httpPrintInCodeConfigValues(client);
    client.println();
    client.println(F("This game can still be controlled remotely with the Game Manager Windows app."));
    return;
  }

  File sdFile = SD.open("index.htm", O_READ);

  if (!sdFile)
  {
    httpSendOkText(client, F("Unable to load User Interface from SD card! This game can still be controlled remotely with the Game Manager Windows app."));
    Serial.println(F("Web Interface failed to load"));
    return;
  }

  client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n"));

  uint8_t buffer[128];

  while (sdFile.available())
  {
    int bytesRead = sdFile.read(buffer, sizeof(buffer));

    if (bytesRead > 0)
    {
      client.write(buffer, bytesRead);
    }
    else
    {
      break;
    }

    yield();
  }

  sdFile.close();
  Serial.println(F("Web Interface Loaded"));
}

static void httpRedirectToLocalUi(WiFiClient &client)
{
  client.print(F("HTTP/1.1 302 Found\r\nLocation: /?ip="));
  httpPrintUiQueryParams(client);
  client.print(F("\r\nConnection: close\r\n\r\n"));
}

static bool httpWritePostBodyToFile(WiFiClient &client, File &sdFile, int contentLength)
{
  unsigned long lastActivity = millis();
  int bytesWritten = 0;

  if (contentLength > 0)
  {
    while (bytesWritten < contentLength && (millis() - lastActivity) < 5000)
    {
      if (client.available() > 0)
      {
        int c = client.read();

        if (c >= 0)
        {
          sdFile.write((uint8_t)c);
          bytesWritten++;
          lastActivity = millis();
        }
      }
      else
      {
        delay(1);
      }
    }

    return (bytesWritten == contentLength);
  }

  while ((millis() - lastActivity) < 1500)
  {
    if (client.available() > 0)
    {
      int c = client.read();

      if (c >= 0)
      {
        sdFile.write((uint8_t)c);
        bytesWritten++;
        lastActivity = millis();
      }
    }
    else
    {
      delay(1);
    }
  }

  return (bytesWritten > 0);
}

void htmlPoll()
{
  WiFiClient client = server.available();
  if (!client) return;

  char requestLine[192];
  memset(requestLine, 0, sizeof(requestLine));

  if (!httpReadLine(client, requestLine, sizeof(requestLine), 3000))
  {
    client.stop();
    return;
  }

  bool isGet = (strncmp(requestLine, "GET ", 4) == 0);
  bool isPost = (strncmp(requestLine, "POST ", 5) == 0);

  // ------------------------------------------------------------
  // Special POST handling for SD card updates.
  // This must happen before discarding the body because the body
  // contains the file content.
  // ------------------------------------------------------------
  if (isPost && strstr(requestLine, "UPDATE"))
  {
    const char *fn = "config.txt";
    if (strstr(requestLine, "UPDATEHTML")) fn = "index.htm";

    if (noSDCard)
    {
      httpDiscardHeaders(client);
      Serial.println(F("Unable to update SD file; SD card not available"));
      httpSendOkText(client, F("ERROR: SD card not available"));
      client.stop();
      return;
    }

    int contentLength = httpReadHeadersGetContentLength(client);
    File sdFile = SD.open(fn, O_WRITE | O_CREAT | O_TRUNC);

    if (sdFile)
    {
      bool writeOk = httpWritePostBodyToFile(client, sdFile, contentLength);
      sdFile.close();

      if (writeOk)
      {
        Serial.print(fn);
        Serial.println(F(" updated. Restarting..."));

        httpSendOkText(client, F("OK"));
        client.stop();
        delay(250);

        NVIC_SystemReset();
        return;
      }
      else
      {
        Serial.print(F("Incomplete POST body for file: "));
        Serial.println(fn);

        httpSendOkText(client, F("ERROR"));
        client.stop();
        return;
      }
    }
    else
    {
      Serial.print(F("Unable to write file: "));
      Serial.println(fn);

      httpSendOkText(client, F("ERROR"));
      client.stop();
      return;
    }
  }

  httpDiscardHeaders(client);

  if (!isGet)
  {
    client.print(F("HTTP/1.1 405 Method Not Allowed\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nERROR"));
    client.stop();
    return;
  }

  char queryData[192];
  httpExtractQueryString(requestLine, queryData, sizeof(queryData));

  // Parse querystring commands first. Only non-command GET requests load the UI.
  bool commandFound = false;
  bool reqResult = false;

  if (httpHasQueryParam(requestLine, "ds")) // Game Statistics
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

  if (httpHasQueryParam(requestLine, "mo")) { commandFound = true; reqResult = slotCommand(MUTE, 4, "Sound Off"); }
  if (httpHasQueryParam(requestLine, "mt")) { commandFound = true; reqResult = slotCommand(UMUTE, 4, "Sound On"); }
  if (httpHasQueryParam(requestLine, "lk")) { commandFound = true; reqResult = slotCommand(LOCK, 4, "Game Locked"); }
  if (httpHasQueryParam(requestLine, "uk")) { commandFound = true; reqResult = slotCommand(ULOCK, 4, "Game Unlocked"); }
  if (httpHasQueryParam(requestLine, "eb")) { commandFound = true; reqResult = slotCommand(EBILL, 4, "BV Enabled"); }
  if (httpHasQueryParam(requestLine, "db")) { commandFound = true; reqResult = slotCommand(DBILL, 4, "BV Disabled"); }
  if (httpHasQueryParam(requestLine, "jr")) { commandFound = true; reqResult = slotCommand(JREST, 4, "Jackpot Reset"); }
  if (httpHasQueryParam(requestLine, "ec")) { commandFound = true; changeToCredits = 1; reqResult = true; }
  if (httpHasQueryParam(requestLine, "dc")) { commandFound = true; changeToCredits = 0; reqResult = true; }

  if (httpHasQueryParam(requestLine, "dt")) // Set Date/Time
  {
    commandFound = true;

    char datetime[15];

    if (getQueryValue(queryData, "dt", datetime, sizeof(datetime)) && datetime[0] != '\0')
    {
      reqResult = SetDateTime(datetime);
    }
    else
    {
      reqResult = false;
    }
  }

  if (httpHasQueryParam(requestLine, "cf")) // Request config data
  {
    client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n"));

    if (noSDCard)
    {
      client.println(F("SD card is not available."));
      client.println(F("The board is using these default configuration values:"));
      client.println();
      httpPrintInCodeConfigValues(client);

      client.stop();
      Serial.println(F("Unable to display config data; SD card not available"));
      return;
    }

    File sdFile = SD.open("config.txt", O_READ);

    if (sdFile)
    {
      uint8_t fileBuffer[128];

      while (sdFile.available())
      {
        int bytesRead = sdFile.read(fileBuffer, sizeof(fileBuffer));

        if (bytesRead > 0)
        {
          client.write(fileBuffer, bytesRead);
        }
        else
        {
          break;
        }

        yield();
      }

      sdFile.close();

      Serial.println(F("Display config.txt"));
    }
    else
    {
      client.print(F("Unable to display config data"));
      Serial.println(F("Unable to display config data"));
    }

    client.stop();
    return;
  }

  if (httpHasQueryParam(requestLine, "bd")) // Board
  {
    client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n"));
    client.print(F("DELUXE"));
    client.stop();
    return;
  }

  if (httpHasQueryParam(requestLine, "pd")) // Player Data - not used in Deluxe sketch; here for compatibility with Game Manager
  {
    client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n"));
    client.print(F("||||||"));
    client.stop();
    return;
  }

  if (httpHasQueryParam(requestLine, "gd")) // Request game data
  {
    client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n"));
    
    if (sasOnline)
    {
      client.print(gameName);
      client.print(F("|"));
      client.print(pollMeters(mCredits));
      client.print(F("|"));
      client.print(pollMeters(mCoinIn));
      client.print(F("|"));
      client.print(pollMeters(mTotWon));
      client.print(F("|"));
      client.print(pollMeters(mTotGames));
      client.print(F("|"));
      client.print(pollMeters(mgamesWon));
      client.print(F("|"));
      client.print(pollMeters(mgamesLost));
      client.print(F("|"));
      client.print(versionString);
    }
    else
    {
      client.print(gameName);
      client.print(F("|||||||"));
      client.print(versionString);
    }

    client.stop();
    Serial.println(F("Game data requested by host"));
    return;
  }

  if (httpHasQueryParam(requestLine, "rb")) // Reboot Arduino
  {
    httpSendOkText(client, F("OK"));
    matrixPrint("   ");
    client.stop();
    delay(250);
    NVIC_SystemReset();
    return;
  }

  if (httpHasQueryParam(requestLine, "ud")) // Update Ticket Data
  {
    commandFound = true;

    char rawLocation[HTTP_QUERY_VALUE_BUFFER_SIZE];
    char rawAddress1[HTTP_QUERY_VALUE_BUFFER_SIZE];
    char rawAddress2[HTTP_QUERY_VALUE_BUFFER_SIZE];
    char location[HTTP_QUERY_VALUE_BUFFER_SIZE];
    char address1[HTTP_QUERY_VALUE_BUFFER_SIZE];
    char address2[HTTP_QUERY_VALUE_BUFFER_SIZE];

    if (getQueryValue(queryData, "ud1", rawLocation, sizeof(rawLocation)) &&
        getQueryValue(queryData, "ud2", rawAddress1, sizeof(rawAddress1)) &&
        getQueryValue(queryData, "ud3", rawAddress2, sizeof(rawAddress2)) &&
        urlDecodeToBuffer(rawLocation, location, sizeof(location)) &&
        urlDecodeToBuffer(rawAddress1, address1, sizeof(address1)) &&
        urlDecodeToBuffer(rawAddress2, address2, sizeof(address2)))
    {
      reqResult = SetTicketData(String(location), String(address1), String(address2));
    }
    else
    {
      reqResult = false;
    }
  }

  if (httpHasQueryParam(requestLine, "cr")) // Add Credits to game
  {
    commandFound = true;

    char credits[9];

    if (getQueryValue(queryData, "cr", credits, sizeof(credits)))
    {
      reqResult = addCredits(credits);
    }
    else
    {
      reqResult = false;
    }
  }

  if (commandFound)
  {
    httpSendOkText(client, reqResult ? F("OK") : F("ERROR"));
    client.stop();
    return;
  }

  // Non-command GET requests load the local UI.
  if (noSDCard)
  {
    httpServeLocalUi(client);
  }
  // Root page requests get redirected once so the browser URL has the UI state.
  // Requests that already have a querystring serve the SD-served page directly.
  else if (strstr(requestLine, "GET / ") == requestLine)
  {
    httpRedirectToLocalUi(client);
  }
  else
  {
    httpServeLocalUi(client);
  }

  client.stop();
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
    if (Serial1.available() > 0)
    {
      byte currentByte = Serial1.read();

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
    Serial.println(F("Unable to read data - response header timeout"));
    sasError = true;
    return;
  }

  int bytesNeeded = sz - 2;
  int bytesRead = 0;
  startTime = millis();

  while (bytesRead < bytesNeeded && (millis() - startTime) < timeoutMs)
  {
    if (Serial1.available() > 0)
    {
      ret[bytesRead + 2] = Serial1.read();
      bytesRead++;
    }
    else
    {
      delay(1);
    }
  }

  if (bytesRead < bytesNeeded)
  {
    Serial.println(F("Unable to read data - response body timeout"));
    memset(ret, 0, sz);
    sasError = true;
    return;
  }
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
  
  Serial.println(msg);
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

byte asciiPairToBcd(const char* p)
{
  return (byte)(((p[0] - '0') << 4) | (p[1] - '0'));
}

bool SetDateTime(const char* datetime)
{
  if (!datetime || datetime[0] == '\0') return false;

  TIM[0] = SASAdr;
  TIM[1] = 0x7F;

  for (byte i = 0; i < 7; i++)
  {
    TIM[i + 2] = asciiPairToBcd(datetime + (i * 2));
  }

  SendTypeS(TIM, sizeof(TIM));
  return waitForACK(SASAdr, "DateTime set by host");
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
