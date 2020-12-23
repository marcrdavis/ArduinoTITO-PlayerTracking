/*
  Arduino TITO and Player Tracking v2.0.20201222
  by Marc R. Davis - Copyright (c) 2020 All Rights Reserved

  Portions of the Arduino SAS protocol implementation by Ian Walker - Thank you!

  Hardware requirements: Arduino Mega 2560 R3; RFID RC 522; W5100 Ethernet Shield; Serial Port Shield;
  Compatible vacuum fluorescent display or LCD; if using an LCD then modifications will be required;
  Modifications will be required if using another type of ethernet shield; Wifi shields are NOT recommended

  Software requirements:
    You will need my modified version of IeeFlipNoFrills.h which fixes compatibility issues with
    newer Arduino IDE

  For TITO Setup please follow the included documentation

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

#include <IniFile.h>
#include <IeeFlipNoFrills.h>
#include <SPI.h>
#include <MFRC522.h>
#include <SD.h>
#include <Ethernet.h>
#include <EEPROM.h>

// Setup Pins
#define SS_PIN 53  //for RFID
#define RST_PIN 49  //for RFID

// ------------------------------------------------------------------------------------------------------------
// Player Tracking Variables
// ------------------------------------------------------------------------------------------------------------

int displayWidth = 20;
int displayHeight = 2;
int scrollDelay = 100;
int cardType = 0;

long Credits = 0;
long totalIn = 0;
long totalWon = 0;
long totalGames = 0;
long gamesWon = 0;
long gamesLost = 0;
long tempTotalIn = 0;
long tempTotalWon = 0;
long tempTotalGames = 0;
long tempGamesWon = 0;
long tempGamesLost = 0;
long playerTotalGames = 0;
long playerGamesWon = 0;
long playerGamesLost = 0;
long playerTotalWon = 0;

bool logToSerial = 1;
bool haveStartingStats = false;
bool localStorage = 1;
bool onlyTITO = 0;
bool changeToCredits = 0;
bool useDHCP = 1;
bool sasError = false;

String ipStr;
String cardHolder = "";
String lastCardID;
String cardID;
String creditsToAdd = "1000";
String changeCredits = "100";
String gameName = "Slot Machine";
String stringData = "";

char ipAddress[15];
char casinoName[30] = "THE CASINO";  // actual text should not exceed the display width
char scrollingText[256] = "Welcome to [CASINONAME]! Enjoy your stay!                    Please insert your Player Card";
char playerMessage[256] = "Welcome back [CARDHOLDER]! Enjoy your stay!";
char scrollBuffer[296];
char fixedBuffer[21];

const char htmlHeader[] = "HTTP/1.1 200 OK\r\n"
                          "Content-Type: text/html\r\n"
                          "Connection: close\r\n\r\n"
                          "<!DOCTYPE HTML>\r\n"
                          "<html>\r\n";

const char htmlFooter[] = "</html>\r\n\r\n";

byte mac[] = { 0x38, 0x24, 0x01, 0x00, 0x00, 0x00 }; // Will be set by init routine
IPAddress ip(192, 168, 1, 254);  // Default address in case ipAddress not populated in config and DHCP unavailable
IPAddress serverIP(192, 168, 1, 254); // The board should not point to itself as a web client

// ------------------------------------------------------------------------------------------------------------
// SAS Protocol Variables
// ------------------------------------------------------------------------------------------------------------

int SASAdr = 0x01;
int CRCH = 0x00;
int CRCL = 0x00;
int LED = 13;

byte SASEvent [1];
byte returnStatus[1];

byte SVNS[2] = {SASAdr, 0x57};
byte TP[2] = {SASAdr, 0x70};
byte RHP[2] = {SASAdr, 0x1B};
byte LOCK[4] = {SASAdr, 0x01, 0x51, 0x08};
byte ULOCK[4] = {SASAdr, 0x02, 0xCA, 0x3A};
byte MUTE[4] = {SASAdr, 0x03, 0x43, 0x2B};
byte UMUTE[4] = {SASAdr, 0x04, 0xFC, 0x5F};
byte EBILL[4] = {SASAdr, 0x06, 0xEE, 0x7C};
byte DBILL[4] = {SASAdr, 0x07, 0x67, 0x6D};
byte HPRES[4] = {SASAdr, 0x94, 0x75, 0xCB};
byte EVInfo[5] = {SASAdr, 0x4D, 0x00, CRCH, CRCL};
byte transComplete[6] = {SASAdr, 0x71, 0x01, 0xFF, 0x1F, 0xD0};
byte globalCRC[2];
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

// ------------------------------------------------------------------------------------------------------------
// Setup instances
// ------------------------------------------------------------------------------------------------------------

IeeFlipNoFrills vfd(22, 23, /*control pins */
                    31, 30, 29, 28, 27, 26, 25, 24 /*data pins */);

MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance.
File sdFile;
EthernetServer server(80);

// ------------------------------------------------------------------------------------------------------------

void setup()
{
  // Variable Reserves
  lastCardID.reserve(9);
  cardID.reserve(9);
  cardHolder.reserve(30);
  creditsToAdd.reserve(8);
  changeCredits.reserve(8);
  ipStr.reserve(15);
  gameName.reserve(30);
  stringData.reserve(256);

  // Initiate  SPI bus
  SPI.begin();
  initSDCard();

  // Read in the config and store in variables
  readConfig();

  // Setup Serial Logging
  if (logToSerial) Serial.begin(9600);

  // Setup SAS/TITO Communications
  Serial1.begin(19200);
  Serial1.setTimeout(200);
  pinMode(LED, OUTPUT);

  Serial.println(F("Arduino TITO and Player Tracking - Version 2.0 (Dec 22, 2020) By Marc R. Davis"));
  Serial.println("Initializing...");

  // Setup Scrolling Text
  String tmp = String(scrollingText);
  tmp.replace("[CASINONAME]", casinoName);
  tmp.toCharArray(scrollingText, tmp.length() + 1);
  strcpy(scrollBuffer, "                    ");
  strcat(scrollBuffer, scrollingText);
  strcat(scrollBuffer, "                    ");

  // Skip init if TITO Only Mode
  if (!onlyTITO)
  {
    // Setup VFD
    vfd.begin(displayWidth, displayHeight);

    // Setup RFID
    mfrc522.PCD_Init();
  }
  else Serial.println("TITO Only Mode Enabled");

  showMessageOnVFD("Initializing...", 0);
  delay(2000);

  // Initialize Ethernet
  initEthernet();

  // Initialize HTTP Server
  server.begin();

  Serial.println(F("Initialization complete"));
}

// ------------------------------------------------------------------------------------------------------------

void loop()
{
  // Check for DHCP renewal
  checkEthernet();
  
  if (onlyTITO)
  {
    // Check web
    htmlPoll();

    // Check game
    generalPoll();
  }
  else
  {
    showMessageOnVFD(casinoName, 0);

    // Scroll Text Loop
    for (int letter = 0; letter <= strlen(scrollBuffer) - displayWidth; letter++) //From 0 to upto n-displayWidth characters supply to below function
    {
      scrollText(0, letter);

      // Check web
      htmlPoll();

      // Check for player card events
      if (checkForPlayerCard()) return;

      // Check game
      generalPoll();
    }
  }
}

// ------------------------------------------------------------------------------------------------------------

// Read the configuration from SD card

void readConfig()
{
  char buffer[256];
  IniFile ini("config.txt");

  if (!ini.open())
  {
    Serial.println(F("Config file is missing!"));
    return;
  }

  if (ini.getValue(NULL, "scrollingText", buffer, 256)) strcpy(scrollingText, buffer);
  if (ini.getValue(NULL, "playerMessage", buffer, 256)) strcpy(playerMessage, buffer);
  if (ini.getValue(NULL, "casinoName", buffer, 256)) strcpy(casinoName, buffer);
  if (ini.getValue(NULL, "displayHeight", buffer, 256)) displayHeight = atoi(buffer);
  if (ini.getValue(NULL, "scrollDelay", buffer, 256)) scrollDelay = atoi(buffer);
  if (ini.getValue(NULL, "logToSerial", buffer, 256)) logToSerial = atoi(buffer);
  if (ini.getValue(NULL, "localStorage", buffer, 256)) localStorage = atoi(buffer);
  if (ini.getValue(NULL, "changeToCredits", buffer, 256)) changeToCredits = atoi(buffer);
  if (ini.getValue(NULL, "useDHCP", buffer, 256)) useDHCP = atoi(buffer);
  if (ini.getValue(NULL, "changeCredits", buffer, 256)) changeCredits = String(buffer);
  if (ini.getValue(NULL, "gameName", buffer, 256)) gameName = String(buffer);

  if (ini.getValue(NULL, "ipAddress", buffer, 256))
  {
    strcpy(ipAddress, buffer);
    ip.fromString((ipAddress));
  }

  if (ini.getValue(NULL, "serverIPAddress", buffer, 256))
  {
    strcpy(ipAddress, buffer);
    serverIP.fromString((ipAddress));
  }
  ini.close();
}

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

  LegacyBonus(0x01, ac[0], ac[1], ac[2], ac[3], 0x00);

  char b[10];
  credits.toCharArray(b, credits.length() + 1);

  strcpy(fixedBuffer, b);
  strcat(fixedBuffer, " credits added");
  Serial.println(credits + " credits added");
  showMessageOnVFD(fixedBuffer, 0);
  delay(2000);
  showMessageOnVFD(casinoName, 0);
  return true;
}

// Read the game meters

bool readGameData()
{
  Serial.println(F("Reading meters from game"));
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

// Read player data from the SD card

String readPlayerDataFromSD(String cid, bool remote)
{
  stringData = "";
  String CardFilename = cid + ".txt";

  IniFile ini(CardFilename.c_str());

  if (!SD.exists(CardFilename))
  {
    // Create file with defaults
    writePlayerDataToSD(CardFilename, 1, "Player", 0, 0, 0, 0);
  }

  char buffer[30];
  if (ini.open())
  {
    if (remote)
    {
      if (ini.getValue(NULL, "cardHolder", buffer, 30)) stringData = String(buffer) + "|";
      if (ini.getValue(NULL, "cardType", buffer, 30)) stringData += String(buffer) + "|";
      if (ini.getValue(NULL, "playerTotalGames", buffer, 30)) stringData += String(buffer) + "|";
      if (ini.getValue(NULL, "playerGamesWon", buffer, 30)) stringData += String(buffer) + "|";
      if (ini.getValue(NULL, "playerGamesLost", buffer, 30)) stringData += String(buffer) + "|";
      if (ini.getValue(NULL, "playerTotalWon", buffer, 30)) stringData += String(buffer) + "|";
      if (ini.getValue(NULL, "creditsToAdd", buffer, 30)) stringData += String(buffer);
    }
    else
    {
      if (ini.getValue(NULL, "cardHolder", buffer, 30)) cardHolder = String(buffer);
      if (ini.getValue(NULL, "cardType", buffer, 30)) cardType = atoi(buffer);
      if (ini.getValue(NULL, "playerTotalGames", buffer, 30)) playerTotalGames = atol(buffer);
      if (ini.getValue(NULL, "playerGamesWon", buffer, 30)) playerGamesWon = atol(buffer);
      if (ini.getValue(NULL, "playerGamesLost", buffer, 30)) playerGamesLost = atol(buffer);
      if (ini.getValue(NULL, "playerTotalWon", buffer, 30)) playerTotalWon = atol(buffer);
      if (ini.getValue(NULL, "creditsToAdd", buffer, 30)) creditsToAdd = String(buffer);
      if (cardHolder == "") cardHolder = "Player";
    }
    ini.close();
  }

  return stringData;
}

// Write player data to remote server

void writePlayerDataToServer(String cid, int ct, String cn, long pg, long pw, long pl, long ptw)
{
  EthernetClient client;
  String sIP = String(serverIP[0]) + '.' + String(serverIP[1]) + '.' + String(serverIP[2]) + '.' + String(serverIP[3]);
  cn.replace(" ", "+");
  String playerData = cn + "|" + String(ct) + "|" + String(pg) + "|" + String(pw) + "|" + String(pl) + "|" + String(ptw);

  if (client.connect(serverIP, 80)) {
    Serial.print("Connected to ");
    Serial.println(client.remoteIP());

    // Make an HTTP request
    client.println("GET /?pu=&cardID=" + cid + "&data=" + playerData + " HTTP/1.1");
    client.println("Host: " + sIP);
    client.println("Connection: close");

    Serial.println("Card data uploaded for: " + cid);
    client.stop();
    return;

  } else {
    Serial.println("Connection to server failed!");
    showMessageOnVFD("Card Update Failed", 0);
    delay(2000);
  }

}

// Read player data from remote server

void readPlayerDataFromServer(String cid)
{
  EthernetClient client;
  String sIP = String(serverIP[0]) + '.' + String(serverIP[1]) + '.' + String(serverIP[2]) + '.' + String(serverIP[3]);
  cardType = 0;
  cardHolder = "";

  if (client.connect(serverIP, 80)) {
    Serial.print("Connected to ");
    Serial.println(client.remoteIP());

    // Make an HTTP request
    client.println("GET /?pd=&cardID=" + cid + " HTTP/1.1");
    client.println("Host: " + sIP);
    client.println("Connection: close");
    client.println();

    // Set timeout for server reply
    int i = 5000;
    while (client.available() <= 0 && i--)
    {
      delay(1);
      if (i == 1)
      {
        Serial.println(F("Timeout"));
        client.stop();
        showMessageOnVFD("Network Timeout", 0);
        delay(2000);
        return;
      }
    }

    while (client.available() > 0)
    {
      stringData = client.readStringUntil('\n');
      if (stringData == "\r") break; // end of headers - this might be an endless loop if response is malformed - look into this
    }

    stringData = client.readStringUntil('\n'); // skip html and body
    stringData = client.readStringUntil('\n'); // this should contain our data

    if (stringData.indexOf("|") == -1) {
      clearStats();
      client.stop();
      Serial.println("Unable to get card data for: " + cid);
      showMessageOnVFD("Card Load Failed", 0);
      delay(2000);
      return;
    }

    cardHolder = getValue(stringData, '|', 0);
    cardType = getValue(stringData, '|', 1).toInt();
    if (cardType == 3)
    {
      creditsToAdd = getValue(stringData, '|', 2);
    }
    else
    {
      playerTotalGames = getValue(stringData, '|', 2).toInt();
      playerGamesWon = getValue(stringData, '|', 3).toInt();
      playerGamesLost = getValue(stringData, '|', 4).toInt();
      playerTotalWon = getValue(stringData, '|', 5).toInt();
    }

    Serial.println("Card data downloaded for: " + cid);
    client.stop();
    return;

  } else {
    Serial.println("Connection to server failed!");
    showMessageOnVFD("Card Load Failed", 0);
    delay(2000);
  }
}

// Write player data to SD card

void writePlayerDataToSD(String filename, int ct, String cn, long pg, long pw, long pl, long ptw)
{
  sdFile = SD.open(filename, O_WRITE | O_CREAT | O_TRUNC);

  // if the file opened okay, write to it:
  if (sdFile)
  {
    sdFile.print("cardHolder=");
    sdFile.println(cn);
    sdFile.print("cardType=");
    sdFile.println(ct);
    sdFile.print("playerTotalGames=");
    sdFile.println(pg);
    sdFile.print("playerGamesWon=");
    sdFile.println(pw);
    sdFile.print("playerGamesLost=");
    sdFile.println(pl);
    sdFile.print("playerTotalWon=");
    sdFile.println(ptw);

    // Close the file
    sdFile.close();

    Serial.println(F("Player data updated successfully"));
  }
  else
  {
    // If the file didn't open, print an error
    Serial.println(F("Error writing player data"));
    showMessageOnVFD("Card Update Failed", 0);
    delay(2000);
  }
}

// Initialize the Ethernet Shield

void initEthernet()
{
  // Generate unique MAC Address
  getMacAddress(mac);

  Ethernet.init(10);  // Most Arduino ethernet shields

  // Start the Ethernet connection and the server
  if (useDHCP) Ethernet.begin(mac);
  else Ethernet.begin(mac, ip);

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found. Remote Access will be unavailable.");

    showMessageOnVFD("No Network Found", 0);
    delay(2000);
    return;
  }

  Serial.print(F("IP address: "));
  Serial.println(Ethernet.localIP());
  ip = Ethernet.localIP();
  ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);

  showMessageOnVFD("Network Connected", 0);
  showMessageOnVFD(ipStr.c_str(), 1);
  delay(2000);

}

// Check for DHCP renewals

void checkEthernet()
{
  if (useDHCP) return;

  switch (Ethernet.maintain()) {
    case 1:
      // Renewed fail
      Serial.println("Error: DHCP renewal failed");
      break;

    case 2:
      // Renewed success
      Serial.println("DHCP Renewed successfully");
      Serial.print(F("IP address: "));
      Serial.println(Ethernet.localIP());
      ip = Ethernet.localIP();
      ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      break;

    case 3:
      // Rebind fail
      Serial.println("Error: Ethernet rebind failed");
      break;

    case 4:
      // Rebind success
      Serial.println("Ethernet rebind successful");
      Serial.print(F("IP address: "));
      Serial.println(Ethernet.localIP());
      ip = Ethernet.localIP();
      ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      break;

    default:
      // Nothing happened
      break;
  }
}

// Initialize the SD card

void initSDCard()
{
  Serial.println(F("Initializing SD card..."));
  if (!SD.begin(4))
  {
    Serial.begin(9600);
    Serial.println(F("SD card initialization failed"));
    Serial.println(F("App Halted"));
    while (1);
  }

  Serial.println(F("SD card initialization done"));
}

// Check for player card insertion

bool checkForPlayerCard()
{
  if (mfrc522.PICC_IsNewCardPresent())
  {
    if (mfrc522.PICC_ReadCardSerial())
    {
      // Card present - identify
      cardID = "";
      byte letter;

      for (byte i = 0; i < mfrc522.uid.size; i++)
      {
        cardID.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
        cardID.concat(String(mfrc522.uid.uidByte[i], HEX));
      }

      cardID.toUpperCase();
      cardID = cardID.substring(1);
      cardID.replace(" ", "");

      if (cardID != lastCardID)
      {
        showMessageOnVFD("CARD INSERTED", 0);

        if (localStorage) readPlayerDataFromSD(cardID, false);
        else   readPlayerDataFromServer(cardID);

        lastCardID = cardID;
        Serial.println(F("Card Inserted"));
        Serial.print(F("Card ID: ")); Serial.println(lastCardID);

        // Can be Player 1, Admin 2, Bonus 3, or 0 if card data cannot be read from SD/Server
        // Had this as a Switch statement and it would get skipped - so we do it this way

        if (cardType == 1)
        {
          Serial.println(F("Card Type: Player"));
          Serial.print(F("Cardholder: ")); Serial.println(cardHolder);
          Serial.print(F("Player Stats - Games played: ")); Serial.print(playerTotalGames);
          Serial.print(F(" Games Won: ")); Serial.print(playerGamesWon);
          Serial.print(F(" Games Lost: ")); Serial.print(playerGamesLost);
          Serial.print(F(" Total Won: ")); Serial.println(playerTotalWon);

          // Get current stats to figure out player session data when card is removed
          haveStartingStats = readGameData();

          if (!haveStartingStats)
          {
            Serial.println(F("Could not read current stats from game. Player's current session will not be saved."));
          }
          else
          {
            tempTotalIn = totalIn;
            tempTotalWon = totalWon;
            tempTotalGames = totalGames;
            tempGamesWon = gamesWon;
            tempGamesLost = gamesLost;
          }

          showMessageOnVFD("GOOD LUCK!", 0);
          delay(2000);

          strcpy(scrollBuffer, "                    ");
          strcat(scrollBuffer, playerMessage);
          strcat(scrollBuffer, "                    ");
          
          String tmp = String(scrollBuffer);
          tmp.replace("[CARDHOLDER]", cardHolder);
          tmp.replace("[CASINONAME]", casinoName);
          tmp.toCharArray(scrollBuffer, tmp.length() + 1);

          Serial.println("Ready for play");
          return true;
        }

        if (cardType == 2)
        {
          // Reserved for future use
          Serial.println(F("Card Type: Admin"));
          return true;
        }

        if (cardType == 3)
        {
          Serial.println(F("Card Type: System Bonus"));
          if (addCredits(creditsToAdd))
          {
            strcpy(scrollBuffer, "                    ");
            strcat(scrollBuffer, "More credits means MORE FUN! Please remove the System Bonus Card now and insert your Player Card.");
            strcat(scrollBuffer, "                    ");
          }

          return true;
        }

        showMessageOnVFD("UNKNOWN CARD", 0);
        delay(2000);
      }
    }
    else
    {
      cardHolder = "";
      lastCardID = "";
      return false;
    }
  }
  else
  {
    bool current, previous;
    if (lastCardID != "")
    {
      previous = true;
      current = !mfrc522.PICC_IsNewCardPresent();

      if (current && previous)
      {
        // Card Removed
        strcpy(scrollBuffer, "                    ");
        strcat(scrollBuffer, scrollingText);
        strcat(scrollBuffer, "                    ");

        showMessageOnVFD("CARD REMOVED", 0);
        Serial.println(F("Card Removed"));

        bool haveEndingStats = false;
        if (cardType == 1 & haveStartingStats) haveEndingStats = readGameData();

        if (haveStartingStats & haveEndingStats)
        {
          //  Update player stats
          String CardFilename = lastCardID + ".txt";

          playerTotalGames = playerTotalGames + (totalGames - tempTotalGames);
          playerGamesWon = playerGamesWon + (gamesWon - tempGamesWon);
          playerGamesLost = playerGamesLost + (gamesLost - tempGamesLost);
          playerTotalWon = playerTotalWon + (totalWon - tempTotalWon);

          // In case it overflows
          if (playerTotalGames < 0 | playerGamesWon < 0 | playerGamesLost < 0 | playerTotalWon < 0)
          {
            Serial.println(F("Player stats for session are invalid and will not be saved!"));
          }
          else
          {
            if (localStorage) writePlayerDataToSD(CardFilename, cardType, cardHolder, playerTotalGames, playerGamesWon, playerGamesLost, playerTotalWon);
            else writePlayerDataToServer(lastCardID, cardType, cardHolder, playerTotalGames, playerGamesWon, playerGamesLost, playerTotalWon);
          }
        }
        else
        {
          if (cardType == 1) Serial.println(F("Could not read current stats from game. Player's current session will not be saved."));
        }

        // Clear variables
        clearStats();
        cardHolder = "";
        lastCardID = "";
        cardType = 1;

        delay(2000);
        return true;
      }
    }
  }

  return false;
}

// Clear player data variables

void clearStats()
{
  tempTotalIn = 0;
  tempTotalWon = 0;
  tempTotalGames = 0;
  tempGamesWon = 0;
  tempGamesLost = 0;

  playerTotalGames = 0;
  playerGamesWon = 0;
  playerGamesLost = 0;
  playerTotalWon = 0;
  haveStartingStats = false;
  cardType = 0;
  stringData = "";
}

// Show static message on VFD

void showMessageOnVFD(char message[], int line)
{
  if (onlyTITO) return;

  if (line == 0) vfd.clear();
  int startPos = floor((displayWidth - strlen(message)) / 2);
  vfd.setCursor(startPos, line);
  vfd.print(message);
}

// Scrolls message on VFD

void scrollText(int printStart, int startLetter)
{
  vfd.setCursor(printStart, 1);
  for (int letter = startLetter; letter <= startLetter + displayWidth - 1; letter++)  // Print only X chars in Line #2 starting 'startLetter'
  {
    vfd.print(scrollBuffer[letter]);
  }

  vfd.print(" ");
  delay(scrollDelay);
}

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

void getMacAddress(byte* macAddr) {
  int eepromOffset = 128;
  int b = 0;
  for (int c = 0; c < 6; c++) {
    b = 0;
    if (macAddr[c] == 0) {
      b = EEPROM.read(eepromOffset + c);
      if (b == 0 || b == 255) {
        b = random(0, 255);
        EEPROM.write(eepromOffset + c, b);
      }
      macAddr[c] = b;
    }
  }
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
    showMessageOnVFD("Remote Access", 0);

    String url = "";
    String request = "";
    String querystring = "";

    stringData = client.readStringUntil('\r');   // Get the first line of request
    while (client.available()) client.read(); // Get the rest of the header and discard
    client.flush();

    request = getValue(stringData, ' ', 1); // Get the request - we only care about GETs
    url = getValue(request, '?', 0);
    querystring = getValue(request, '?', 1);

    // Parse querystring
    // Uses and expands on codes from BETTORSlots TITO for compatibility with remote app
    String command = querystring.substring(0, 2);

    if (url.equals("/") && command != "")
    {
      if (command == "ps") // Player Statistics
      {
        if (cardHolder != "")
        {
          readGameData();

          client.print(htmlHeader);
          client.print("<head><meta name='viewport' content='initial-scale=1.0'><style>body {font-family: Tahoma;}</style></head><body><h2>PLAYER STATISTICS: " + cardHolder + "</h2><br>(when card was inserted)<br><br>");
          client.print("Total Games<br>" + String(playerTotalGames + (totalGames - tempTotalGames)) + "<br>Games Won<br>" + String(playerGamesWon + (gamesWon - tempGamesWon)) + "<br>Games Lost<br>" + String(playerGamesLost + (gamesLost - tempGamesLost)) + "<br>Total Won<br>" + String(playerTotalWon + (totalWon - tempTotalWon)) + "<br>");
          client.print("</body>");
          client.print(htmlFooter);
        }
        else
        {
          client.print(htmlHeader);
          client.print("<head><style>body {font-family: Tahoma;}</style></head><body><h2>NO PLAYER CARD INSERTED</h2><br>");
          client.print("</body>");
          client.print(htmlFooter);
        }

        client.stop();
        showMessageOnVFD(casinoName, 0);
        return;
      }

      if (command == "ds") // Game Statistics
      {
        readGameData();

        client.print(htmlHeader);
        client.print("<head><meta name='viewport' content='initial-scale=1.0'><style>body {font-family: Tahoma;}</style></head><body><h2>GAME STATISTICS</h2><br>");
        client.print("Credits<br>" + String(Credits) + "<br>Total In<br>" + String(totalIn) + "<br>Total Won<br>" + String(totalWon) + "<br>Total Games<br>" + String(totalGames) + "<br>Games Won<br>" + String(gamesWon) + "<br>gamesLost<br>" + String(gamesLost) + "<br>");
        client.print("</body>");
        client.print(htmlFooter);

        client.stop();
        showMessageOnVFD(casinoName, 0);
        return;
      }

      if (command == "pd")  // Download player card data
      {
        String cardID = getValue(getValue(querystring, '&', 1), '=', 1);
        String playerData = readPlayerDataFromSD(cardID, true);

        client.print(htmlHeader);
        client.print("<head></head><body>\r\n");
        for (int i = 0; i < 6; i++) {
          client.print(getValue(playerData, '|', i) + "|");
        }
        client.print("\r\n</body>");
        client.print(htmlFooter);
        Serial.print(F("Card Data Request: ")); Serial.println(cardID);
        client.stop();
        showMessageOnVFD(casinoName, 0);
        return;
      }

      if (command == "pu")  // Upload player card data to server
      {
        String cardID = getValue(getValue(querystring, '&', 1), '=', 1);
        String playerData = getValue(getValue(querystring, '&', 2), '=', 1);

        writePlayerDataToSD(cardID + ".txt", getValue(playerData, '|', 1).toInt(), urlDecode(getValue(playerData, '|', 0)), getValue(playerData, '|', 2).toInt(), getValue(playerData, '|', 3).toInt(), getValue(playerData, '|', 4).toInt(), getValue(playerData, '|', 5).toInt());
        Serial.print(F("Card Data Updated: ")); Serial.println(cardID);
      }

      if (command == "mo") mute();
      if (command == "mt") unmute();
      if (command == "lk") lockMachine();
      if (command == "uk") unlockMachine();
      if (command == "eb") enableBV();
      if (command == "db") disableBV();
      if (command == "rh") resetHandpay();
      if (command == "ec") changeButtonToCredits(true);
      if (command == "dc") changeButtonToCredits(false);
      if (command == "pn" && cardHolder != "") cardHolder=urlDecode(getValue(getValue(querystring, '&', 0), '=', 1));

      if (command == "ud") // Update Ticket Data
      {
        String location = urlDecode(getValue(getValue(querystring, '&', 0), '=', 1));
        String address1 = urlDecode(getValue(getValue(querystring, '&', 1), '=', 1));
        String address2 = urlDecode(getValue(getValue(querystring, '&', 2), '=', 1));

        SetTicketData(location, address1, address2);
        showMessageOnVFD("Ticket Data Updated", 0);
        delay(2000);
      }

      if (command == "cr")  // Add Credits to game
      {
        String credits = getValue(getValue(querystring, '&', 1), '=', 1);
        addCredits(credits);
      }

      if (command == "cm")  // Update Scrolling Message
      {
        String customMsg = getValue(getValue(urlDecode(querystring), '&', 1), '=', 1);
        customMsg.toCharArray(scrollingText, customMsg.length() + 1);
        strcpy(scrollBuffer, "                    ");
        strcat(scrollBuffer, scrollingText);
        strcat(scrollBuffer, "                    ");
        Serial.print(F("Scrolling message updated: ")); Serial.println(customMsg);
      }

      client.print(htmlHeader);
      client.print("<head><style>body {font-family: Tahoma;}</style></head><body><h2>OK</h2><br></body>");
      client.print(htmlFooter);
      client.stop();
      showMessageOnVFD(casinoName, 0);
      return;
    }
    else if (url.equals("/"))
    {
      // Show web interface

      String  cp = "No Card Inserted";
      if (cardHolder != "") cp = cardHolder;

      sdFile = SD.open("index.htm", O_READ);
      if (sdFile)
      {
        client.print(htmlHeader);
        client.print("<head><meta name='viewport' content='initial-scale=1.0'>");
        client.print("<style>body {font-family: Tahoma;} button {font-family: inherit; font-size: 1.0em; background-color: #008CBA; color: white; border: none; text-decoration: none; border-radius: 4px; transition-duration: 0.4s;} button:hover { background-color: white; color: black; border: 2px solid #008CBA; } td { padding-left: 7.5px; padding-right: 7.5px; } </style></head><body>");
        client.print("<div style='max-width: 100%; margin: auto; text-align:center;'>");
        client.print("<h2>Arduino TITO and Player Tracking</h2>");
        client.print("Game Name: <b>" + gameName + "</b>&nbsp;&nbsp;&nbsp;");
        client.print("IP Address: <b>" + ipStr + "</b><br>");
        client.print("Current player: <b> " + cp + "</b></div>");

        while (sdFile.available()) {
          client.print(sdFile.readStringUntil('\n'));
        }

        sdFile.close();
        client.print("</div></body>");
        client.print(htmlFooter);
        Serial.println(F("Web Interface Loaded"));
      }
      else
      {
        // UI Not available (SD Card error?)

        client.print(htmlHeader);
        client.print("<head><meta name='viewport' content='initial-scale=1.0'><style>body {font-family: Tahoma;}</style></head><body>");
        client.print("<div style='max-width: 100%; margin: auto; text-align:center;'>");
        client.print("<h2>Arduino TITO and Player Tracking</h2>");
        client.print("Game Name: <b>" + gameName + "</b>&nbsp;&nbsp;&nbsp;");
        client.print("IP Address: <b>" + ipStr + "</b><br><hr>");
        client.print("Unable to load User Interface!<br>This device can still be controlled remotely with the BETTORSlots Android/IOS apps.<br>");
        client.print("</div></body>");
        client.print(htmlFooter);
        Serial.println(F("Web Interface failed to load"));
      }
    }
    else
    {
      // Everything else is a 404
      client.print("HTTP/1.1 404 Not Found\r\n\r\n");
    }

    client.stop();
    showMessageOnVFD(casinoName, 0);
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

  UCSR1B = 0b10011101;
  Serial1.write(0x80);
  delay(20);
  Serial1.write(0x81);
  UCSR1B = 0b10011100;

  if (Serial1.available() > 0) Serial1.readBytes(SASEvent, sizeof(SASEvent));

  if (SASEvent[0] != 0x00 && SASEvent[0] != 0x01 && SASEvent[0] != 0x80 && SASEvent[0] != 0x81) {
    Serial.print("SAS Event Received: "); Serial.print(SASEvent[0], HEX); Serial.println("");
  }

  // Process these events
  if (SASEvent[0] == 0x71 & changeToCredits) addCredits(changeCredits); // To enable 'Change button' credits
  if (SASEvent[0] == 0x72 & changeToCredits) addCredits(changeCredits); // To enable 'Change button' credits
  if (SASEvent[0] == 0x51) SendHandpay();
  if (SASEvent[0] == 0x57) SystemValidation();
  if (SASEvent[0] == 0x3D) CashOutState();
  if (SASEvent[0] == 0x67) RedeemTicket();
  if (SASEvent[0] == 0x68) ConfirmRedeem();
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

byte waitForResponse(byte & waitfor, byte * ret, int sz)
{
  byte responseBytes[sz - 2];
  int wait = 0;

  while (Serial1.read() != waitfor && wait < 1000) {
    delay(1);
    wait += 1;
  }

  if (wait >= 1000) {
    Serial.println(F("Unable to read data - timeout"));
    memset(ret, 0, sz);
    sasError = true;
    return ret;
  }

  Serial1.readBytes(responseBytes, sizeof(responseBytes));
  memcpy(ret, {0x01}, 1);
  memcpy(ret + 1, waitfor, 1);
  memcpy(ret + 2, responseBytes, sizeof(responseBytes));

  return ret;
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

void lockMachine()
{
  SendTypeR(LOCK, sizeof(LOCK));
  Serial1.readBytes(returnStatus, sizeof(returnStatus));
  Serial.println(F("Machine locked"));
}

void unlockMachine()
{
  SendTypeR(ULOCK, sizeof(LOCK));
  Serial1.readBytes(returnStatus, sizeof(returnStatus));
  Serial.println(F("Machine Unlocked"));
}

void mute()
{
  SendTypeR(MUTE, sizeof(MUTE));
  Serial1.readBytes(returnStatus, sizeof(returnStatus));
  Serial.println(F("Machine muted"));
}

void unmute()
{
  SendTypeR(UMUTE, sizeof(UMUTE));
  Serial1.readBytes(returnStatus, sizeof(returnStatus));
  Serial.println(F("Machine unmuted"));
}

void disableBV()
{
  SendTypeR(DBILL, sizeof(DBILL));
  Serial1.readBytes(returnStatus, sizeof(returnStatus));
  Serial.println(F("Bill Validator Disabled"));
}

void enableBV()
{
  SendTypeR(EBILL, sizeof(EBILL));
  Serial1.readBytes(returnStatus, sizeof(returnStatus));
  Serial.println(F("Bill Validator Enabled"));
}

void changeButtonToCredits(bool e)
{
  if (e) {
    changeToCredits = true;
    Serial.println(F("Change to Credits Enabled"));
  }
  else
  {
    changeToCredits = false;
    Serial.println(F("Change to Credits Disabled"));
  }
}

void resetHandpay()
{
  SendTypeR(HPRES, sizeof(HPRES));
  Serial1.readBytes(returnStatus, sizeof(returnStatus));
  Serial.println(F("Handpay reset"));
}

void SystemValidation()
{
  SendTypeR(SVNS, sizeof(SVNS));
  waitForResponse(SVNS[1], COT, sizeof(COT));

  SVN [0]  = SASAdr;                    // Adress
  SVN [1]  = 0x58;                      // Recieve Validation Number
  SVN [2]  = 0x01;                      // Validation ID
  SVN [3]  = COT [2];                   // Cashout Type
  SVN [4]  = 0x00;                      // None
  SVN [5]  = 0x00;                      // None
  SVN [6]  = COT [3];                   // Cashout Value Byte5 (MSB)
  SVN [7]  = COT [4];                   // Cashout Value Byte4
  SVN [8]  = COT [5];                   // Cashout Value Byte3
  SVN [9]  = COT [6] ;                  // Cashout Value Byte2
  SVN [10] = COT [7];                   // Cashout Value Byte1 (LSB)

  SendTypeS(SVN, sizeof(SVN));
  Serial1.readBytes(COS, sizeof(COS));
  Serial.println(F("Printing cashout ticket"));
}

void SetTicketData(String loc, String addr1, String addr2)
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
  Serial1.readBytes(TDR, sizeof(TDR));
  Serial.println(F("Updated Cashout Ticket Data"));
}

void CashOutState()
{
  SendTypeS(EVInfo, sizeof(EVInfo));
  Serial1.readBytes(TPS, sizeof(TPS));
}

void LegacyBonus (byte SASAdr, byte Amount1, byte Amount2, byte Amount3, byte Amount4, byte Type)
{
  returnStatus[0] = 0x00;

  LBS [0] = SASAdr;
  LBS [1] = 0x8A;
  LBS [2] = Amount1;
  LBS [3] = Amount2;
  LBS [4] = Amount3;
  LBS [5] = Amount4;
  LBS [6] = Type;

  SendTypeS(LBS, sizeof(LBS));
  Serial1.readBytes(returnStatus, sizeof(returnStatus));
}

void SendHandpay()
{
  SendTypeR(RHP, sizeof(RHP));
  Serial1.readBytes(HPS, sizeof(HPS));
}

void RedeemTicket()
{
  SendTypeR(TP, sizeof(TP));
  waitForResponse(TP[1], TEQ, sizeof(TEQ));

  TRS [0]  =  0x01;                              //' Addres
  TRS [1]  =  0x71;                              //' Command
  TRS [2]  =  0x10;                              //' Number of Bytes
  TRS [3]  =  0x00;                              //' Transfer Code
  TRS [4]  = TEQ [14];                           //' Ticket Amount BCD1  LSB
  TRS [5]  = TEQ [15];                           //' Ticket Amount BCD2
  TRS [6]  = TEQ [16];                           //' Ticket Amount BCD3
  TRS [7]  = TEQ [17];                           //' Ticket Amount BCD4
  TRS [8]  = TEQ [18];                           //' Ticket Amount BCD5  MSB
  TRS [9]  = 0x00;                               //' Parsing Code
  TRS [10] = TEQ [10];                           //' Validation BCD1
  TRS [11] = TEQ [11];                           //' Validation BCD2
  TRS [12] = TEQ [12];                           //' Validation BCD3
  TRS [13] = TEQ [13];                           //' Validation BCD4
  TRS [14] = TEQ [14];                           //' Validation BCD5
  TRS [15] = TEQ [15];                           //' Validation BCD6
  TRS [16] = TEQ [16];                           //' Validation BCD7
  TRS [17] = TEQ [17];                           //' Validation BCD8
  TRS [18] = TEQ [18];                           //' Validation BCD9

  SendTypeS(TRS, sizeof(TRS));
  Serial1.readBytes(TEQ, sizeof(TEQ));
  Serial.println(F("Redeeming ticket"));
}

void ConfirmRedeem()
{
  SendTypeR(transComplete, sizeof(transComplete));
  Serial1.readBytes(TEQ, sizeof(TEQ));
  Serial.println(F("Ticket redeemed successfully"));
}

void SendTypeR (byte temp[], int len)
{
  UCSR1B = 0b10011101;
  Serial1.write(temp[0]);
  UCSR1B = 0b10011100;

  for (int i = 1; i < len; i++) Serial1.write(temp[i]);
}

void SendTypeS (byte temp[], int len)
{
  CalculateCRC(temp, len - 2);
  temp [len - 2] = CRCH;
  temp [len - 1] = CRCL;
  UCSR1B = 0b10011101;
  Serial1.write(temp[0]);
  delay(10);
  UCSR1B = 0b10011100;

  for (int i = 1; i < len; i++) Serial1.write(temp[i]);
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
