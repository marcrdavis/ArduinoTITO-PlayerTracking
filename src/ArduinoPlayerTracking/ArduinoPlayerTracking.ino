/*
  Arduino Player Tracking and Display v1.0
  by Marc Davis (10/22/2020)

  Hardware requirements: Arduino Mega 2560 R3; RFID RC 522; Cytron WiFi Shield; Compatible
  vacuum fluorescent display or LCD; if using an LCD then modifications will be required.

  Modifications will be required if using wired ethernet or another WiFi Shield

  Software requirements: You will need my modified version of IeeFlipNoFrills.h which fixes
  compatibility issues with newer Arduino IDE.

  NOTE: BETTORSLOTS File Modifications may be required on its SD card! Check your SD card
  for the Data.html file. This file should be blank; otherwise the app will have trouble
  reading the data.

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
#include <CytronWiFiClient.h>
#include <CytronWiFiShield.h>
#include <SD.h>

// Setup variables and constants
#define SS_PIN 53  //for RFID
#define RST_PIN 49  //for RFID

int DisplayWidth = 20;
int DisplayHeight = 2;
int ScrollDelay = 100;
long TotalIn = 0;
long TotalWon = 0;
long TotalGames = 0;
long GamesWon = 0;
long GamesLost = 0;

long tempTotalIn = 0;
long tempTotalWon = 0;
long tempTotalGames = 0;
long tempGamesWon = 0;
long tempGamesLost = 0;

long PlayerTotalGames = 0;
long PlayerGamesWon = 0;
long PlayerGamesLost = 0;
long PlayerTotalWon = 0;

bool logToSerial = 0;
bool haveStartingStats = false;
bool LocalStorage = 1;
String Cardholder = "";
String LastCardID;
String CardID;
int CardType = 0;
int CreditsToAdd = 1000;
char *ssid = "YOURSSIDNAMEHERE";
char *pass = "YOURWIFIPASSWORD";
String BettorIPAddress = "BETTORIPADDRESS";
char *CasinoName = "THE CASINO";

char ScrollingText[256] = "Welcome to the Casino! Enjoy your stay!                    Please insert your Player Card";
char PlayerMessage[256] = "Welcome back[CARDHOLDER]! Enjoy your stay!";
char ScrollBuffer[296];
char FixedBuffer[21];

// Setup instances
IeeFlipNoFrills vfd(22, 23, /*control pins */
                    31, 30, 29, 28, 27, 26, 25, 24 /*data pins */);

MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance.

// Create file instance
File sdFile;

void setup()
{
  // Variable Reserves
  LastCardID.reserve(9);
  CardID.reserve(9);
  Cardholder.reserve(30);
  BettorIPAddress.reserve(15);

  SPI.begin();  // Initiate  SPI bus
  initSDCard();

  // Read in the config and store in variables
  readConfig();

  // Setup Serial Logging
  if (logToSerial) Serial.begin(9600);  // Initiate a serial communication

  Serial.println(F("Arduino Player Tracking - Version 1.0 (Oct 22, 2020) By Marc R. Davis"));
  Serial.println("Initializing...");

  // Setup Scrolling Text
  strcpy(ScrollBuffer, "                    ");
  strcat(ScrollBuffer, ScrollingText);
  strcat(ScrollBuffer, "                    ");

  // Setup VFD
  vfd.begin(DisplayWidth, DisplayHeight);

  // Setup RFID
  mfrc522.PCD_Init(); // Initiate MFRC522

  showMessageOnVFD("Initializing...");

  // Initialize Wifi
  initWifi();

  // Wait for slot - Takes a moment for the game to boot
  showMessageOnVFD("Waiting on Game");
  delay(4000);

  Serial.println(F("Initialization complete"));

}

void loop()
{
  showMessageOnVFD(CasinoName);

  // Scroll text Loop
  for (int letter = 0; letter <= strlen(ScrollBuffer) - DisplayWidth; letter++) //From 0 to upto n-DisplayWidth characters supply to below function
  {
    scrollText(0, letter);
    if (checkForPlayerCard()) return;
  }
}

void readConfig()
{
  char buffer[256];
  IniFile ini("config.txt");

  if (!ini.open())
  {
    Serial.println(F("Config file is missing!"));
    return;
  }

  if (ini.getValue(NULL, "ScrollingText", buffer, 256)) strcpy(ScrollingText, buffer);
  if (ini.getValue(NULL, "PlayerMessage", buffer, 256)) strcpy(PlayerMessage, buffer);
  if (ini.getValue(NULL, "CasinoName", buffer, 256)) strcpy(CasinoName, buffer);
  if (ini.getValue(NULL, "ssid", buffer, 256)) strcpy(ssid, buffer);
  if (ini.getValue(NULL, "pass", buffer, 256)) strcpy(pass, buffer);
  if (ini.getValue(NULL, "BettorIPAddress", buffer, 15)) String(Buffer);
  if (ini.getValue(NULL, "DisplayHeight", buffer, 256)) DisplayHeight = atoi(buffer);
  if (ini.getValue(NULL, "ScrollDelay", buffer, 256)) ScrollDelay = atoi(buffer);
  if (ini.getValue(NULL, "logToSerial", buffer, 256)) logToSerial = atoi(buffer);
  if (ini.getValue(NULL, "LocalStorage", buffer, 256)) LocalStorage = atoi(buffer);

  ini.close();
}

bool addCredits(int credits)
{
  ESP8266Client client;
  if (!client.connect(BettorIPAddress, 80))
  {
    Serial.println(F("Failed to connect to BettorSlots TITO Board."));
    client.stop();
    // Check Wifi
    if (checkWifi())
    {
      bool s = false;
      s = addCredits(credits);
      return s;
    }

    return false;
  }

  String req = "GET /?cr=Add+Credits&cr=" + String(credits) + " HTTP/1.1\r\nConnection: close\r\n\r\n";

  if (!client.print(req))
  {
    Serial.println(F("HTTP Request failed"));
    showMessageOnVFD("Unable to add credits");
    client.stop();
    delay(2000);
    return false;
  }

  char b[10];
  String str;
  str = String(credits);
  str.toCharArray(b, 10);

  strcpy(FixedBuffer, b);
  strcat(FixedBuffer, " credits added");
  showMessageOnVFD(FixedBuffer);

  client.stop();
  return true;
}

bool retrySlotData(int retries)
{
  while (!readSlotData() && retries--)
  {
    if (retries = 0) return false;
  }

  return true;
}

bool readSlotData()
{
  ESP8266Client client;
  client.setTimeout(20);

  Serial.println(F("Reading counters from game"));

  if (!client.connect(BettorIPAddress, 80))
  {
    Serial.println(F("Failed to connect to BettorSlots TITO Board"));
    client.stop();
    checkWifi();
    return false;
  }

  const char *httpRequest = "GET /?ds=Display+Statistics HTTP/1.1\r\n"
                            "User-Agent: Arduino\r\n"
                            "Connection: close\r\n\r\n";

  if (!client.print(httpRequest))
  {
    Serial.println(F("HTTP Request failed"));
    client.stop();
    return false;
  }

  // set timeout for server reply
  int i = 5000;
  while (client.available() <= 0 && i--)
  {
    delay(1);
    if (i == 1)
    {
      Serial.println(F("Timeout"));
      client.stop();
      return false;
    }
  }

  String sHtml = "";
  sHtml.reserve(512); // Reserve this so we don't blow out the heap
  bool wait = 1;
  char c;

  while (client.available() > 0)
  {
    // get character from buffer
    c = client.read();
    if (c == '\n') continue;
    if (c == '\r') continue;

    // skip stupid +IPD ....  header
    // for some reason the bettorslots http server doesn't output data consistently
    // so we need to deal with small chunks of data

    if (wait)
    {
      if (c == ':') wait = 0;
      continue;
    }
    else
    {
      if (c == '+')
      {
        wait = 1;
        continue;
      }

      sHtml += String(c);
    }
  }

  client.stop();

  // Test to see if we have all of the data - sometimes the client does not return all of the bytes
  // or is not ready yet

  if (sHtml.indexOf("SinceON") == -1 | sHtml.indexOf("+IPD") != -1)
  {
    Serial.println("DEBUG: " + sHtml);
    showMessageOnVFD("Please Wait");
    return false;
  }
  else
  {
    Serial.println(F("Game data received successfully"));

    // Parse the data
    sHtml = sHtml.substring(sHtml.indexOf("Credits"));
    sHtml.replace("<br>", ":");
    TotalIn = getValue(sHtml, ':', 3).toInt();
    TotalWon = getValue(sHtml, ':', 5).toInt();
    TotalGames = getValue(sHtml, ':', 7).toInt();
    GamesWon = getValue(sHtml, ':', 9).toInt();
    GamesLost = getValue(sHtml, ':', 11).toInt();
    return true;
  }

  return false;
}

void readPlayerDataFromSD(const char *filename)
{
  char buffer[30];
  IniFile ini(filename);

  if (!ini.open())
  {
    Serial.println(F("Player file is missing!"));
    return;
  }

  if (ini.getValue(NULL, "Cardholder", buffer, 30)) Cardholder = String(buffer);
  if (ini.getValue(NULL, "CardType", buffer, 30)) CardType = atoi(buffer);
  if (ini.getValue(NULL, "PlayerTotalGames", buffer, 30)) PlayerTotalGames = atoi(buffer);
  if (ini.getValue(NULL, "PlayerGamesWon", buffer, 30)) PlayerGamesWon = atoi(buffer);
  if (ini.getValue(NULL, "PlayerGamesLost", buffer, 30)) PlayerGamesLost = atoi(buffer);
  if (ini.getValue(NULL, "PlayerTotalWon", buffer, 30)) PlayerTotalWon = atoi(buffer);
  if (ini.getValue(NULL, "CreditsToAdd", buffer, 30)) CreditsToAdd = atoi(buffer);

  ini.close();
}

void writePlayerDataToSD(const String &Filename, int ct, String cn, long pg, long pw, long pl, long ptw)
{
  sdFile = SD.open(Filename, O_WRITE | O_CREAT | O_TRUNC);

  // if the file opened okay, write to it:
  if (sdFile)
  {
    sdFile.print("Cardholder=");
    sdFile.println(cn);
    sdFile.print("CardType=");
    sdFile.println(ct);
    sdFile.print("PlayerTotalGames=");
    sdFile.println(pg);
    sdFile.print("PlayerGamesWon=");
    sdFile.println(pw);
    sdFile.print("PlayerGamesLost=");
    sdFile.println(pl);
    sdFile.print("PlayerTotalWon=");
    sdFile.println(ptw);

    // close the file
    sdFile.close();

    Serial.println(F("Player data updated successfully"));
  }
  else
  {
    // if the file didn't open, print an error
    Serial.println(F("Error writing player data"));
  }
}

void initSDCard()
{
  Serial.println(F("Initializing SD card..."));
  if (!SD.begin(4))
  {
    Serial.println(F("SD card initialization failed"));
    Serial.println(F("App Halted"));

    showMessageOnVFD("SD Card Init Failed");
    while (1);
  }

  Serial.println(F("SD card initialization done"));
}

bool checkWifi()
{
  wifi.updateStatus();

  if (wifi.status() != 2)
  {
    Serial.println("Wifi disconnected... reconnecting");
    Serial.println("Connecting to WiFi");
    showMessageOnVFD("Waiting on Network");

    if (!wifi.begin(12, 13))
    {
      Serial.println(F("Error talking to WiFi Shield"));
      delay(1000);
      return false;
    }

    if (!wifi.connectAP(ssid, pass))
    {
      Serial.println(F("Error connecting to WiFi"));
      showMessageOnVFD("WiFi Error");
      delay(1000);
      return false;
    }

    Serial.print(F("Connected to "));
    Serial.println(wifi.SSID());
    wifi.updateStatus();
    return true;
  }

  return true;
}

bool initWifi()
{
  showMessageOnVFD("Connecting to WiFi");
  if (!wifi.begin(12, 13))
  {
    Serial.println(F("Error talking to WiFi Shield"));
    delay(1000);
    return false;
  }

  if (!wifi.connectAP(ssid, pass))
  {
    Serial.println(F("Error connecting to WiFi"));
    showMessageOnVFD("WiFi Error");
    delay(1000);
    return false;
  }

  Serial.print(F("Connected to "));
  Serial.println(wifi.SSID());
  Serial.print(F("IP address is "));
  Serial.println(wifi.localIP());
  wifi.updateStatus();

  switch (wifi.status())
  {
    case 2:
      // Wifi Connected
      showMessageOnVFD("WiFi Connected");
      delay(1000);
      return true;
      break;
    case 5:
      Serial.println(F("No WiFi Available"));
      showMessageOnVFD("No WiFi Available");
      delay(1000);
      return false;
      break;
  }

  return false;
}

bool checkForPlayerCard()
{
  if (mfrc522.PICC_IsNewCardPresent())
  {
    if (mfrc522.PICC_ReadCardSerial())
    {
      // Card present - identify
      CardID = "";
      byte letter;

      for (byte i = 0; i < mfrc522.uid.size; i++)
      {
        CardID.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
        CardID.concat(String(mfrc522.uid.uidByte[i], HEX));
      }

      CardID.toUpperCase();
      CardID = CardID.substring(1);
      CardID.replace(" ", "");
      String CardFilename = CardID + ".txt";

      if (CardID != LastCardID)
      {
        showMessageOnVFD("CARD INSERTED");

        if (!SD.exists(CardFilename))
        {
          // Create file with defaults
          writePlayerDataToSD(CardFilename, 1, "Player", 0, 0, 0, 0);
        }

        // Read card data from memory card or from website
        char charFile[12];
        CardFilename.toCharArray(charFile, CardFilename.length() + 1);

        readPlayerDataFromSD(charFile);

        LastCardID = CardID;
        Serial.println(F("Card Inserted"));
        Serial.print(F("Card ID: "));
        Serial.println(LastCardID);
        Serial.print(F("Cardholder: "));
        Serial.println(Cardholder);
        Serial.print(F("Player Stats - Games played: "));
        Serial.print(PlayerTotalGames);
        Serial.print(F(" Games Won: "));
        Serial.print(PlayerGamesWon);
        Serial.print(F(" Games Lost: "));
        Serial.print(PlayerGamesLost);
        Serial.print(F(" Total Won: "));
        Serial.println(PlayerTotalWon);

        // Can be Player 1, Admin 2 or Bonus 3
        // Had this as a Switch statement and it would get skipped - odd

        if (CardType == 1)
        {
          Serial.println(F("Card Type: Player"));
          // Get current stats to figure out player session data when card is removed
          haveStartingStats = retrySlotData(3);

          if (!haveStartingStats)
          {
            Serial.println(F("Could not read current stats from game. Player's current session will not be saved."));
          }
          else
          {
            tempTotalIn = TotalIn;
            tempTotalWon = TotalWon;
            tempTotalGames = TotalGames;
            tempGamesWon = GamesWon;
            tempGamesLost = GamesLost;
          }

          showMessageOnVFD("GOOD LUCK!");
          delay(1000);
          String tmp = String(PlayerMessage);
          tmp.replace("[CARDHOLDER]", Cardholder);
          tmp.toCharArray(PlayerMessage, tmp.length() + 1);

          strcpy(ScrollBuffer, "                    ");
          strcat(ScrollBuffer, PlayerMessage);
          strcat(ScrollBuffer, "                    ");
          return true;
        }

        if (CardType == 2)
        {
          Serial.println(F("Card Type: Admin"));
          return true;
        }

        if (CardType == 3)
        {
          Serial.println(F("Card Type: System Bonus"));
          if (addCredits(CreditsToAdd))
          {
            strcpy(ScrollBuffer, "                    ");
            strcat(ScrollBuffer, "More credits means MORE FUN! Please remove the System Bonus Card now and insert your Player Card.");
            strcat(ScrollBuffer, "                    ");
          }

          return true;
        }

        showMessageOnVFD("UNKNOWN CARD");
        delay(1000);
      }
    }
    else
    {
      Cardholder = "";
      LastCardID = "";
      return false;
    }
  }
  else
  {
    if (LastCardID != "")
    {
      // Check if Card was removed
      bool current, previous;
      previous = !mfrc522.PICC_IsNewCardPresent();
      delay(75);
      current = !mfrc522.PICC_IsNewCardPresent();
      if (current && previous)
      {
        // Card Removed
        strcpy(ScrollBuffer, "                    ");
        strcat(ScrollBuffer, ScrollingText);
        strcat(ScrollBuffer, "                    ");

        showMessageOnVFD("CARD REMOVED");
        Serial.println(F("Card Removed"));

        bool haveEndingStats = false;
        if (Cardholder != "SYSTEM BONUS" &haveStartingStats) haveEndingStats = retrySlotData(3);

        if (haveStartingStats & haveEndingStats)
        {
          //  Update player stats
          String CardFilename = LastCardID + ".txt";

          PlayerTotalGames = PlayerTotalGames + (TotalGames - tempTotalGames);
          PlayerGamesWon = PlayerGamesWon + (GamesWon - tempGamesWon);
          PlayerGamesLost = PlayerGamesLost + (GamesLost - tempGamesLost);
          PlayerTotalWon = PlayerTotalWon + (TotalWon - tempTotalWon);

          // TODO - also want to track $$
          writePlayerDataToSD(CardFilename, CardType, Cardholder, PlayerTotalGames, PlayerGamesWon, PlayerGamesLost, PlayerTotalWon);
        }
        else
        {
          if (Cardholder != "SYSTEM BONUS") Serial.println(F("Could not read current stats from game. Player's current session will not be saved."));
        }

        // Clear variables
        clearStats();
        Cardholder = "";
        LastCardID = "";
        CardType = 1;

        delay(2000);
        return true;
      }
    }
  }

  return false;

}

void clearStats()
{
  tempTotalIn = 0;
  tempTotalWon = 0;
  tempTotalGames = 0;
  tempGamesWon = 0;
  tempGamesLost = 0;

  PlayerTotalGames = 0;
  PlayerGamesWon = 0;
  PlayerGamesLost = 0;
  PlayerTotalWon = 0;
  haveStartingStats = false;
  CardType = 0;
}

void showMessageOnVFD(char message[])
{
  vfd.clear();
  int startPos = floor((DisplayWidth - strlen(message)) / 2);
  vfd.setCursor(startPos, 0);
  vfd.print(message);
}

void scrollText(int printStart, int startLetter)
{
  vfd.setCursor(printStart, 1);
  for (int letter = startLetter; letter <= startLetter + DisplayWidth - 1; letter++)  // Print only X chars in Line #2 starting 'startLetter'
  {
    vfd.print(ScrollBuffer[letter]);
  }

  vfd.print(" ");
  delay(ScrollDelay);
}

String getValue(const String &data, char separator, int index)
{
  int found = 0;
  int strIndex[] = { 0,
                     -1
                   };
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

int freeRam()
{
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
