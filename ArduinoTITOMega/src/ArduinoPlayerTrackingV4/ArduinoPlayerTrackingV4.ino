/*
  Arduino TITO and Player Tracking v4.0.20260623 Ethernet
  by Marc R. Davis - Copyright (c) 2020-2026 All Rights Reserved
  https://github.com/marcrdavis/ArduinoTITO-PlayerTracking

  Portions of the Arduino SAS protocol implementation by Ian Walker - Thank you!
  Additional testing and troubleshooting by NLG member Eddiiie - Thank you!

  IMPORTANT: This single sketch supports multiple configurations. You must enable the correct
  code blocks for your hardware. See inline comments. The sketch defaults to: Generic LCD display,
  Generic RFID Reader and the ACT 8x2 Keyboard

  Hardware requirements: 
  Arduino Mega 2560 R3; Compatible RFID or magnetic reader; W5100 Ethernet Shield; Serial Port;
  Compatible vacuum fluorescent display or LCD; Compatible keypad; Modifications will 
  be required if using another type of ethernet shield; Wifi shields are NOT recommended

  Software requirements:
  If using an IEE or Noritake VFD You will need my modified version of the libraries included in the zip file

  Upgrading from earlier versions:
  Be sure to check the sample config.txt file in the zip file for new or changed parameters that may be required
  for the new version
    
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

#include <IniFile.h>
#include <SPI.h>
#include <SD.h>
#include <Ethernet.h>
#include <Keypad.h>

// ------------------------------------------------------------------------------------------------------------
// CONFIGURATION INSTRUCTIONS
//
// In each of the following 3 sections enable only the groupings for your respective hardware
// There are also sections in the setup() function that may need to be enabled or disabled depending on your
// display and reader types.
//
// You will also need to enable the correct checkForPlayerCard() function depending on your reader type
// ------------------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------------------
// Displays - Enable ONLY ONE Group
// ------------------------------------------------------------------------------------------------------------

// IEE VFDs
//#include <IeeFlipNoFrills.h> // Enable for IEE VFDs
//IeeFlipNoFrills display(22, 23, 31, 30, 29, 28, 27, 26, 25, 24); // IEE VFDs; Pins 22, 23 = Control Pins; Pins 31-24 = data pins

// Futaba VFDs
//#include <FutabaVFD.h> // Enable for Futaba VFDs
//FutabaVFD display(22, 23, 31, 30, 29, 28, 27, 26, 25, 24); // FutabaVFD VFDs; Pins 22 = Reset, 23 = Write; Pins 31-24 = data pins DB7-DB0

// Noritake GU-7000s
//#include <GU7000_Interface.h>     // Enable this for GU-7000 Series VFDs
//#include <GU7000_Serial_Async.h>  // Enable this for GU-7000 Series VFDs
//#include <Noritake_VFD_GU7000.h>  // Enable this for GU-7000 Series VFDs
//GU7000_Serial_Async interface(38400, 3, 5, 7); //  Pins 3 = SIN, 5 = BUSY, 7 = RESET; There is also code to enable in setup()
//Noritake_VFD_GU7000 display;

// LCDs
#include <LiquidCrystal.h> // Enable this for LCDs
LiquidCrystal display(22, 23, 27, 26, 25, 24); // (4 bit mode); Pins 22 = RS, 23 = Enable, 24 = DB7, 25 = DB6, 26 = DB5, 27 = DB4
//LiquidCrystal display(22, 23, 31, 30, 29, 28, 27, 26, 25, 24); // (8 bit mode); Pins 22 = RS, 23 = Enable, 24 = DB7, 25 = DB6, 26 = DB5, 27 = DB4, 28 = DB3, 29 = DB2, 30 = DB1, 31 = DB0

// ------------------------------------------------------------------------------------------------------------
// Card Readers - Enable ONLY ONE Group
// ------------------------------------------------------------------------------------------------------------

// UIC MSR240-02TMRNWWBR or compatible data/clock interface
//#include <MagStripe.h>
//MagStripe card;

// XS Technologies PI70-120-TLA-DFR or compatible Serial/TTL interface
//#include <MagStripeSerial.h>
//MagStripeSerial card;

// Generic RFID Reader
#include <MFRC522.h>
#define SS_PIN 53  
#define RST_PIN 49 
MFRC522 card(SS_PIN, RST_PIN);

// ------------------------------------------------------------------------------------------------------------
// Keypads - Enable ONLY ONE Group
// ------------------------------------------------------------------------------------------------------------

// Bally 6x2 Keypad P/N 105123F & 3x4 PCDSKEY1
/*const int ROW_NUM = 3; //  6x2 Keypad presents as a 3x4 matrix
const int COLUMN_NUM = 4; 

// Key Matrix
char keys[ROW_NUM][COLUMN_NUM] = {
  {'1','2','3','C'},
  {'4','5','6','0'},
  {'7','8','9','E'}
};

// Pin definition
byte pin_rows[ROW_NUM] = {39, 38, 41}; // Keypad Pins 1,2,3
byte pin_column[COLUMN_NUM] = {45, 42, 43, 40}; // Keypad Pins 7,6,5,4 */

// ACT 8x2 Keypad
const int ROW_NUM = 4; // Keypad is physically 8x2 but presents as a 4x4 matrix
const int COLUMN_NUM = 4; 

// Key Matrix
char keys[ROW_NUM][COLUMN_NUM] = {
  {'1','2','3','U'},
  {'4','5','6','D'},
  {'7','8','9','-'},
  {'C','0','H','E'}
};

// Pin definition
byte pin_rows[ROW_NUM] = {38, 39, 40, 41}; // Keypad Pins 1,2,3,4
byte pin_column[COLUMN_NUM] = {42, 43, 44, 45}; // Keypad Pins 5,6,7,8 

// ------------------------------------------------------------------------------------------------------------
// Player Tracking Variables
// ------------------------------------------------------------------------------------------------------------

int displayWidth = 20;
int displayHeight = 2;
int displayCols = 20;
int displayRows = 2;
int scrollDelay = 110;
int cardType = 0;
int adminPin = 1234;

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
long tournamentScore = 0;
long winMeterStart = 0;
long creditFloor = 0;
float playerComps = 0;
float compPercentage = 0.01; // Set to zero to disable comps
unsigned long lastUpdate;

bool logToSerial = 1;
bool haveStartingStats = false;
bool localStorage = 1;
bool onlyTITO = 0;
bool changeToCredits = 0;
bool useDHCP = 1;
bool autoAddCredits = 0;
bool sasOnline = false;
bool sasError = false;
bool inAdminMenu = false;
bool inPlayerMenu = false;
bool resetScroll = false;

char ipStr[16] = "";
char cardHolder[32] = "No Card Inserted";
char lastCardID[32] = "";
char cardID[32] = "";
char creditsToAdd[12] = "1000";
char changeCredits[12] = "500";
char gameName[32] = "Slot Machine";
char versionString[] = "4.0.20260623";
const char Board[] = "MEGA";

char ipAddress[15];
char casinoName[30] = "THE CASINO";  // actual text should not exceed the display width
char attractMessage[256] = "Welcome to [CASINONAME]! Enjoy your stay! Please insert your Player Card";
char playerMessage[256] = "Welcome back [CARDHOLDER]! Enjoy your stay in [CASINONAME]!";
char bonusMessage[100] = "More credits means MORE FUN! Please remove the System Bonus Card now and insert your Player Card.";
char adminMenu[150] = "1=Add Credits  2=Sound On  3=Sound Off  4=Unlock Game  5=Lock Game  6=Enable BV  7=Disable BV  8=Change-Credits On  9=Change-Credits Off  0=Exit Menu";
char playerMenu[55] = "1=Show Comp Balance  2=Use Comp Credits  0=Exit Menu";
char scrollBuffer[296];
char fixedBuffer[21];
const size_t HTTP_QUERY_VALUE_BUFFER_SIZE = 128;
char ssid[32];  // your Wifi network SSID 
char pass[32];  // your Wifi network password 

static const byte DATA_BUFFER_LEN = 108;
static char cardData[DATA_BUFFER_LEN];

const char htmlHeader[] = "HTTP/1.1 200 OK\r\n"
                          "Content-Type: text/html\r\n"
                          "Connection: close\r\n\r\n"
                          "<!DOCTYPE HTML>\r\n"
                          "<html>\r\n";
                          
const char textHeader[] = "HTTP/1.1 200 OK\r\n"
                          "Content-Type: text/plain\r\n"
                          "Connection: close\r\n\r\n";
 
const char htmlFooter[] = "</html>\r\n\r\n";

byte mac[] = { 0x38, 0x24, 0x01, 0x00, 0x00, 0x00 }; // If you have more than one board change this to be unique per board
IPAddress ip(192, 168, 1, 254);  // Default address in case ipAddress not populated in config and DHCP unavailable
IPAddress serverIP(192, 168, 1, 254); // The board should not point to itself as a web client

// ------------------------------------------------------------------------------------------------------------
// SAS Protocol Variables
// ------------------------------------------------------------------------------------------------------------

int LED = 13;

byte SASAdr = 0x01;
byte CRCH = 0x00;
byte CRCL = 0x00;

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
Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );

// ------------------------------------------------------------------------------------------------------------
// Reset function
// ------------------------------------------------------------------------------------------------------------

void(* resetFunc) (void) = 0;

// ------------------------------------------------------------------------------------------------------------
// Setup - called once during init
// ------------------------------------------------------------------------------------------------------------

void setup()
{
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
    
  Serial.print(F("Arduino TITO and Player Tracking - By Marc R. Davis - Version ")); Serial.println(versionString);
  Serial.println(F("Initializing..."));

  // Setup Attract Scroll
  setupAttractMessage();

  // Skip init if TITO Only Mode
  if (!onlyTITO)
  {
    // Setup Display
    display.begin(displayWidth, displayHeight);

    // Enable this section for Noritake GU-7000 Series Displays Only
    /*delay(500);
    display.interface(interface);
    display.isModelClass(7003); // Set based on model of display
    display.GU7000_reset();
    display.GU7000_init();*/  

    // Setup Magnetic Reader - disable if using RFID card reader
    //card.begin(2);
    
    // Setup Generic RFID Reader - disable if using magnetic card reader
    card.PCD_Init(); 
  }
  else Serial.println(F("TITO Only Mode Enabled"));
  
  showMessageOnDisplay("Initializing...", 0);
  delay(1000);

  // Initialize Ethernet
  initEthernet();

  // Initialize HTTP Server
  server.begin();
  
  // Clear the game serial buffer
  if (Serial1.available() > 3) while (Serial1.available() > 3) Serial1.read();
  
  Serial.println(F("Initialization complete"));   
}

// ------------------------------------------------------------------------------------------------------------
// Main processing loop
// ------------------------------------------------------------------------------------------------------------

void loop()
{
  // Check for DHCP renewal
  checkEthernet();

  // Update player stats every 2 min if card is inserted
  if (millis() - lastUpdate >= 2*60*1000UL)
  {
   lastUpdate = millis();  //reset 
   updatePlayerStats();
  }

  resetScroll=false;

  if (onlyTITO)
  {
     // Check web
    htmlPoll();

    // Check game
    generalPoll();
  }
  else
  {
    // Display Messages
    if (!inAdminMenu && !inPlayerMenu) showMessageOnDisplay(casinoName, 0);
    
    // Scroll Text Loop
    for (int letter = 0; letter <= strlen(scrollBuffer) - displayCols; letter++)
    {
      if (resetScroll) return;
      scrollText(0, letter);
      
      // Check web
      htmlPoll();

      // Check game
      generalPoll();

      // Check for player card events
      if (sasOnline) if (checkForPlayerCard()) return;

      // Keypad input
      char key = keypad.getKey();
      if (inAdminMenu)
      {
        // Admin Menu Options
        if (key == '1') adminAddCredits();
        if (key == '2') slotCommand(UMUTE,4,"Sound On");
        if (key == '3') slotCommand(MUTE,4,"Sound Off");
        if (key == '4') slotCommand(ULOCK,4,"Game Unlocked");
        if (key == '5') slotCommand(LOCK,4,"Game Locked");
        if (key == '6') slotCommand(EBILL,4,"BV Enabled");
        if (key == '7') slotCommand(DBILL,4,"BV Disabled");
        if (key == '8') changeButtonToCredits(true);
        if (key == '9') changeButtonToCredits(false);
        if (key == '0') { 
          exitMenu();
          return;                
        }
      }

      if (inPlayerMenu)
      {
        // Player Menu Options
        if (key == '1') showAvailComps();
        if (key == '2') useCompCredits();
        if (key == '0') { 
          exitMenu();
          return;                
        }
      }

      // Access admin menu using pin
      if (key == 'C' && !inAdminMenu && !inPlayerMenu) {
        char pin[8];
        showMessageOnDisplay("ENTER ADMIN PIN", 0); 
        if (readKeypad(pin, sizeof(pin), true) && atoi(pin) == adminPin) enterAdminMenu();
        return;
      }

      // Access player menu
      if (cardType == 1 && key == 'E' && !inAdminMenu && !inPlayerMenu) {
        enterPlayerMenu();
        return;
      }      

      // Display IP and Version Info
      if (key == '0' && !inAdminMenu && !inPlayerMenu) {
          strcpy(fixedBuffer, "VER ");
          strncat(fixedBuffer, versionString, sizeof(fixedBuffer) - strlen(fixedBuffer) - 1);
          showMessageOnDisplay(fixedBuffer, 0);
          showMessageOnDisplay(ipStr, 1);
          delay(3000);
          exitMenu();
          return;
      }

    }
  }

  // If autoAddCredits is enabled then check the credit meter and add credits if necessary
  if (autoAddCredits)
  {
    Credits = pollMeters(mCredits);
    if (Credits != 0 && Credits < creditFloor) 
    {
      Serial.println(F("Low credit threshold reached!"));
      addCredits(changeCredits);      
    }
  }

}

// ------------------------------------------------------------------------------------------------------------
// Player & Admin Functions
// ------------------------------------------------------------------------------------------------------------

void showAvailComps()
{
  char b[20];
  int creds = 0;
  
  if (haveStartingStats && readGameData()) {
    creds = playerComps + abs((totalIn - tempTotalIn) * compPercentage);
  }
  else creds = playerComps;

  ltoa(creds, b, 10);
  strncat(b, " credits", sizeof(b) - strlen(b) - 1);
  showMessageOnDisplay("PLAYER COMPS",0);
  showMessageOnDisplay(b,1);
  delay(2000);
  
  showMessageOnDisplay("PLAYER MENU",0);
}

void useCompCredits()
{
  int creds = 0;
  
  if (haveStartingStats && readGameData()) playerComps += abs((totalIn - tempTotalIn) * compPercentage);
  creds = playerComps;
    
  if (creds < 1){
    showMessageOnDisplay("NO COMPS AVAIL",0);
    delay(2000);
  }
  else {
   char compCredits[12];
   ltoa(creds, compCredits, 10);
   if (addCredits(compCredits))
   {
      // Reset comps to zero
      playerComps=0;    
  
      // Call this now to write back the changed playerComps; the other values will be updated when the card is removed
      if (localStorage) writePlayerDataToSD(lastCardID, cardType, cardHolder, playerTotalGames, playerGamesWon, playerGamesLost, playerTotalWon, playerComps);
      else writePlayerDataToServer(lastCardID, cardType, cardHolder, playerTotalGames, playerGamesWon, playerGamesLost, playerTotalWon, playerComps);
   }
   else
   {
      showMessageOnDisplay("ERROR - RETRY",0);
      delay(2000);
   }
  }
  showMessageOnDisplay("PLAYER MENU",0);
}

void adminAddCredits()
{
  char credits[12];
  showMessageOnDisplay("AMOUNT TO ADD",0);
  
  if (readKeypad(credits, sizeof(credits), false) && credits[0]) addCredits(credits);
  showMessageOnDisplay("ADMIN MENU",0);
}

void exitMenu()
{
  // Reset display attract mode to last state
  Serial.println(F("Leaving Menu"));
  if (cardType == 1) setupPlayerMessage(false);
  else setupAttractMessage();
  
  inAdminMenu=false;
  inPlayerMenu=false;
}

void enterPlayerMenu()
{
  showMessageOnDisplay("PLAYER MENU",0);
  inPlayerMenu=true;
  Serial.println(F("Player Menu Active"));
  strcpy(scrollBuffer, "                    ");
  strcat(scrollBuffer, playerMenu);
  strcat(scrollBuffer, "                    ");
}

void enterAdminMenu()
{
  showMessageOnDisplay("ADMIN MENU",0);
  inAdminMenu=true;
  Serial.println(F("Admin Menu Active"));
  strcpy(scrollBuffer, "                    ");
  strcat(scrollBuffer, adminMenu);
  strcat(scrollBuffer, "                    ");
}

// Get keypad input
bool readKeypad(char *output, byte outputSize, bool mask)
{
  if (!output || outputSize == 0) return false;

  output[0] = '\0';
  byte len = 0;
  char key = keypad.getKey();

  while (key != 'E')
  {
    if (key == 'C')
    { 
      output[0] = '\0';
      len = 0;
      showMessageOnDisplay("                    ",1); 
    }
    else if (key && len < outputSize - 1)
    { 
      output[len++] = key;
      output[len] = '\0';

      char b[20];
      if (mask)
      {
        byte m = min(len, (byte)(sizeof(b) - 1));
        memset(b, '*', m);
        b[m] = '\0';
      }
      else
      {          
        strncpy(b, output, sizeof(b) - 1);
        b[sizeof(b) - 1] = '\0';
      }

      strcpy(fixedBuffer, b);
      showMessageOnDisplay(b,1); 
    }
    
    key = keypad.getKey();
    delay(1);

    // Must continue checking game
    generalPoll();
  }

  return true;
}

// ------------------------------------------------------------------------------------------------------------
// IO Functions
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

  if (ini.getValue(NULL, "attractMessage", buffer, 256)) strcpy(attractMessage, buffer);
  if (ini.getValue(NULL, "playerMessage", buffer, 256)) strcpy(playerMessage, buffer);
  if (ini.getValue(NULL, "casinoName", buffer, 256)) strcpy(casinoName, buffer);
  if (ini.getValue(NULL, "displayHeight", buffer, 256)) displayHeight = atoi(buffer);
  if (ini.getValue(NULL, "displayWidth", buffer, 256)) displayWidth = atoi(buffer);
  if (ini.getValue(NULL, "displayRows", buffer, 256)) displayRows = atoi(buffer);
  if (ini.getValue(NULL, "displayCols", buffer, 256)) displayCols = atoi(buffer);
  if (ini.getValue(NULL, "adminPin", buffer, 256)) adminPin = atoi(buffer);
  if (ini.getValue(NULL, "scrollDelay", buffer, 256)) scrollDelay = atoi(buffer);
  if (ini.getValue(NULL, "logToSerial", buffer, 256)) logToSerial = atoi(buffer);
  if (ini.getValue(NULL, "localStorage", buffer, 256)) localStorage = atoi(buffer);
  if (ini.getValue(NULL, "onlyTITO", buffer, 256)) onlyTITO = atoi(buffer);
  if (ini.getValue(NULL, "changeToCredits", buffer, 256)) changeToCredits = atoi(buffer);
  if (ini.getValue(NULL, "useDHCP", buffer, 256)) useDHCP = atoi(buffer);
  if (ini.getValue(NULL, "changeCredits", buffer, 256)) { strncpy(changeCredits, buffer, sizeof(changeCredits) - 1); changeCredits[sizeof(changeCredits) - 1] = '\0'; }
  if (ini.getValue(NULL, "gameName", buffer, 256)) { strncpy(gameName, buffer, sizeof(gameName) - 1); gameName[sizeof(gameName) - 1] = '\0'; }
  if (ini.getValue(NULL, "compPercentage", buffer, 256)) compPercentage = atof(buffer);
  if (ini.getValue(NULL, "autoAddCredits", buffer, 256)) autoAddCredits =  atoi(buffer);
  if (ini.getValue(NULL, "creditFloor", buffer, 256)) creditFloor = atol(buffer);
  if (ini.getValue(NULL, "wifiSSID", buffer, 256)) strcpy(ssid, buffer);
  if (ini.getValue(NULL, "wifiPassword", buffer, 256)) strcpy(pass, buffer);

  if (ini.getValue(NULL, "macAddress", buffer, 256))
  {
    byte b = 0;
    byte n = 0;

    for (int i = 0; buffer[i] && b < 6; i++)
    {
      char c = buffer[i];

      if (c == ':' || c == '-' || c == ' ') continue;
      if (c >= 'a' && c <= 'f') c -= 32;

      byte v = (c <= '9') ? c - '0' : c - 'A' + 10;

      if (n & 1) mac[b++] |= v;
      else mac[b] = v << 4;

      n++;
    }
  }

  if (creditFloor == 0 && autoAddCredits == 1)
  {
    // creditFloor cannot be zero or ticket cashout will not work properly
    autoAddCredits = 0;
    Serial.println(F("Unable to enable autoAddCredits because creditFloor is set to 0"));
  }

    if (atoi(changeCredits) == 0 && autoAddCredits == 1)
  {
    // changeCredits cannot be zero or ticket cashout will not work properly
    autoAddCredits = 0;
    Serial.println(F("Unable to enable autoAddCredits because changeCredits is set to 0"));
  }
  
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

static bool httpReadLine(EthernetClient &client, char *buffer, size_t bufferSize, unsigned long timeoutMs);
static void httpDiscardHeaders(EthernetClient &client);
static bool getPipeValue(const char *data, byte index, char *output, size_t outputSize);

// Append a value to a fixed char buffer

static void appendPlayerField(char *out, size_t outSize, const char *value)
{
  if (!out || outSize == 0 || !value) return;

  size_t len = strlen(out);
  if (len < outSize - 1) strncat(out, value, outSize - len - 1);
}

// Read player data from the SD card

bool readPlayerDataFromSD(const char *cid, bool remote, char *out, size_t outSize)
{
  if (remote && out && outSize) out[0] = '\0';
  if (!cid || cid[0] == '\0') return false;

  char CardFilename[40];
  snprintf(CardFilename, sizeof(CardFilename), "%s.txt", cid);
  IniFile ini(CardFilename);

  if (!SD.exists(CardFilename))
  {
    // Create file with defaults
    writePlayerDataToSD(cid, 1, "Player", 0, 0, 0, 0, 0);
  }

  char buffer[30];
  bool ok = false;

  if (ini.open())
  {
    if (remote)
    {
      if (out && outSize)
      {
        if (ini.getValue(NULL, "cardHolder", buffer, sizeof(buffer))) appendPlayerField(out, outSize, buffer);
        appendPlayerField(out, outSize, "|");
        if (ini.getValue(NULL, "cardType", buffer, sizeof(buffer))) appendPlayerField(out, outSize, buffer);
        appendPlayerField(out, outSize, "|");
        if (ini.getValue(NULL, "playerTotalGames", buffer, sizeof(buffer))) appendPlayerField(out, outSize, buffer);
        appendPlayerField(out, outSize, "|");
        if (ini.getValue(NULL, "playerGamesWon", buffer, sizeof(buffer))) appendPlayerField(out, outSize, buffer);
        appendPlayerField(out, outSize, "|");
        if (ini.getValue(NULL, "playerGamesLost", buffer, sizeof(buffer))) appendPlayerField(out, outSize, buffer);
        appendPlayerField(out, outSize, "|");
        if (ini.getValue(NULL, "playerTotalWon", buffer, sizeof(buffer))) appendPlayerField(out, outSize, buffer);
        appendPlayerField(out, outSize, "|");
        if (ini.getValue(NULL, "creditsToAdd", buffer, sizeof(buffer))) appendPlayerField(out, outSize, buffer);
        appendPlayerField(out, outSize, "|");
        if (ini.getValue(NULL, "playerComps", buffer, sizeof(buffer))) appendPlayerField(out, outSize, buffer);
      }
    }
    else
    {
      if (ini.getValue(NULL, "cardHolder", buffer, sizeof(buffer))) { strncpy(cardHolder, buffer, sizeof(cardHolder) - 1); cardHolder[sizeof(cardHolder) - 1] = '\0'; }
      if (ini.getValue(NULL, "cardType", buffer, sizeof(buffer))) cardType = atoi(buffer);
      if (ini.getValue(NULL, "playerTotalGames", buffer, sizeof(buffer))) playerTotalGames = atol(buffer);
      if (ini.getValue(NULL, "playerGamesWon", buffer, sizeof(buffer))) playerGamesWon = atol(buffer);
      if (ini.getValue(NULL, "playerGamesLost", buffer, sizeof(buffer))) playerGamesLost = atol(buffer);
      if (ini.getValue(NULL, "playerTotalWon", buffer, sizeof(buffer))) playerTotalWon = atol(buffer);
      if (ini.getValue(NULL, "creditsToAdd", buffer, sizeof(buffer))) { strncpy(creditsToAdd, buffer, sizeof(creditsToAdd) - 1); creditsToAdd[sizeof(creditsToAdd) - 1] = '\0'; }
      if (ini.getValue(NULL, "playerComps", buffer, sizeof(buffer))) playerComps = atof(buffer);

      if (cardHolder[0] == '\0') strcpy(cardHolder, "Player");
      if (playerComps < 0) playerComps = 0;
    }

    ini.close();
    ok = true;
  }

  return ok;
}

// Write player data to remote server

void writePlayerDataToServer(const char *cid, int ct, const char *cn, long pg, long pw, long pl, long ptw, float pc)
{
  if (!cid || cid[0] == '\0') return;

  EthernetClient client;

  if (client.connect(serverIP, 80))
  {
    Serial.print(F("Connected to "));
    Serial.println(client.remoteIP());

    // Make an HTTP request
    client.print(F("GET /?pu&cardID="));
    client.print(cid);
    client.print(F("&data="));

    if (cn)
    {
      while (*cn)
      {
        if (*cn == ' ') client.print('+');
        else client.print(*cn);
        cn++;
      }
    }

    client.print('|');
    client.print(ct);
    client.print('|');
    client.print(pg);
    client.print('|');
    client.print(pw);
    client.print('|');
    client.print(pl);
    client.print('|');
    client.print(ptw);
    client.print('|');
    client.print(pc);
    client.println(F(" HTTP/1.1"));
    client.print(F("Host: "));
    client.println(serverIP);
    client.println(F("Connection: close"));
    client.println();

    Serial.print(F("Card data uploaded for: "));
    Serial.println(cid);
    client.stop();
    return;
  }

  Serial.println(F("Connection to server failed!"));
  showMessageOnDisplay("Card Update Fail", 0);
  delay(2000);
}

// Read player data from remote server

void readPlayerDataFromServer(const char *cid)
{
  if (!cid || cid[0] == '\0') return;

  EthernetClient client;
  cardType = 0;
  cardHolder[0] = '\0';

  if (client.connect(serverIP, 80))
  {
    Serial.print(F("Connected to "));
    Serial.println(client.remoteIP());

    // Make an HTTP request
    client.print(F("GET /?pd&cardID="));
    client.print(cid);
    client.println(F(" HTTP/1.1"));
    client.print(F("Host: "));
    client.println(serverIP);
    client.println(F("Connection: close"));
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
        showMessageOnDisplay("Network Timeout", 0);
        delay(2000);
        return;
      }
    }

    httpDiscardHeaders(client);

    char playerData[180];

    if (!httpReadLine(client, playerData, sizeof(playerData), 3000) || !strchr(playerData, '|'))
    {
      clearStats();
      client.stop();
      Serial.print(F("Unable to get card data for: "));
      Serial.print(cid);
      showMessageOnDisplay("Card Load Fail", 0);
      delay(2000);
      return;
    }

    char field[48];

    if (getPipeValue(playerData, 0, field, sizeof(field))) { strncpy(cardHolder, field, sizeof(cardHolder) - 1); cardHolder[sizeof(cardHolder) - 1] = '\0'; }
    if (getPipeValue(playerData, 1, field, sizeof(field))) cardType = atoi(field);

    if (cardType == 3)
    {
      if (getPipeValue(playerData, 2, field, sizeof(field))) { strncpy(creditsToAdd, field, sizeof(creditsToAdd) - 1); creditsToAdd[sizeof(creditsToAdd) - 1] = '\0'; }
    }
    else if (cardType == 1)
    {
      if (getPipeValue(playerData, 2, field, sizeof(field))) playerTotalGames = atol(field);
      if (getPipeValue(playerData, 3, field, sizeof(field))) playerGamesWon = atol(field);
      if (getPipeValue(playerData, 4, field, sizeof(field))) playerGamesLost = atol(field);
      if (getPipeValue(playerData, 5, field, sizeof(field))) playerTotalWon = atol(field);
      if (getPipeValue(playerData, 6, field, sizeof(field))) playerComps = atof(field);
    }

    Serial.print(F("Card data downloaded for: "));
    Serial.println(cid);
    client.stop();
    return;
  }

  Serial.println(F("Connection to server failed!"));
  showMessageOnDisplay("Card Load Fail", 0);
  delay(2000);
}

// Write player data to SD card

bool writePlayerDataToSD(const char *cid, int ct, const char *cn, long pg, long pw, long pl, long ptw, float pc)
{
  if (!cid || cid[0] == '\0') return false;

  char CardFilename[40];
  snprintf(CardFilename, sizeof(CardFilename), "%s.txt", cid);

  sdFile = SD.open(CardFilename, O_WRITE | O_CREAT | O_TRUNC);

  // if the file opened okay, write to it:
  if (sdFile)
  {
    sdFile.print(F("cardHolder=")); sdFile.println(cn);
    sdFile.print(F("cardType=")); sdFile.println(ct);
    sdFile.print(F("playerTotalGames=")); sdFile.println(pg);
    sdFile.print(F("playerGamesWon=")); sdFile.println(pw);
    sdFile.print(F("playerGamesLost=")); sdFile.println(pl);
    sdFile.print(F("playerTotalWon=")); sdFile.println(ptw);
    sdFile.print(F("playerComps=")); sdFile.println(pc);

    // Close the file
    sdFile.close();

    Serial.println(F("Player data updated successfully"));
    return true;
  }
  else
  {
    // If the file didn't open, print an error
    Serial.println(F("Error writing player data"));
    showMessageOnDisplay("Card Update Fail", 0);
    delay(2000);
  }
  return false;
}

// Initialize the SD card

void initSDCard()
{
  if (!SD.begin(4))
  {
    Serial.begin(9600);
    Serial.println(F("SD card initialization failed"));
    Serial.println(F("App Halted"));
    while (1);
  }
}

// ------------------------------------------------------------------------------------------------------------
// Network Functions
// ------------------------------------------------------------------------------------------------------------

// Helper function

void setIpStrFromIp()
{
  snprintf(ipStr, sizeof(ipStr), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
}

// Initialize the Ethernet Shield

void initEthernet()
{
  Ethernet.init(10);  
  
  // Start the Ethernet connection and the server
  if (useDHCP) Ethernet.begin(mac);
  else Ethernet.begin(mac, ip);

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println(F("Ethernet shield was not found. Remote Access will not be available."));

    showMessageOnDisplay("No Network", 0);
    delay(2000);
    return;
  }
  
  Serial.print(F("IP address: "));
  Serial.println(Ethernet.localIP());
  ip = Ethernet.localIP();
  setIpStrFromIp();

  showMessageOnDisplay("Net Connected", 0);
  showMessageOnDisplay(ipStr, 1);
  delay(2000);
}

// Check for DHCP renewals

void checkEthernet()
{
  if (!useDHCP) return;

  switch (Ethernet.maintain()) {
    case 1:
      // Renewed fail
      Serial.println(F("Error: DHCP renewal failed"));
      break;

    case 2:
      // Renewed success
      Serial.println(F("DHCP Renewed successfully"));
      Serial.print(F("IP address: ")); Serial.println(Ethernet.localIP());
      ip = Ethernet.localIP();
      setIpStrFromIp();
      break;

    case 3:
      // Rebind fail
      Serial.println(F("Error: Ethernet rebind failed"));
      break;

    case 4:
      // Rebind success
      Serial.println(F("Ethernet rebind successful"));
      Serial.print(F("IP address: ")); Serial.println(Ethernet.localIP());
      ip = Ethernet.localIP();
      setIpStrFromIp();
      break;

    default:
      // Nothing happened
      break;
  }
}

// ------------------------------------------------------------------------------------------------------------
// Card and player functions
// Enable only one checkForPlayerCard() function based on your card reader hardware
// ------------------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------------------
// Check for player card insertion - Magnetic Card Reader
// ------------------------------------------------------------------------------------------------------------

/*
bool checkForPlayerCard()
{
  if (cardID[0] == '\0' && card.available())
  {
    // Card insertion begun
    Serial.println(F("Reading card"));
    short chars = card.read(cardData, DATA_BUFFER_LEN);
    
    if (chars > 0)
    {
      // Card present - identify
      if (chars >= DATA_BUFFER_LEN) chars = DATA_BUFFER_LEN - 1;
      cardData[chars] = '\0';

      char *start = cardData;
      while (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n') start++;

      char *end = start + strlen(start);
      while (end > start && (*(end - 1) == ' ' || *(end - 1) == '\t' || *(end - 1) == '\r' || *(end - 1) == '\n')) *(--end) = '\0';

      byte len = strlen(start);
      if (len > 8) start += len - 8;

      byte out = 0;
      cardID[0] = '\0';

      while (*start && out < sizeof(cardID) - 1)
      {
        char c = *start++;
        if (c == ' ') continue;
        if (c >= 'a' && c <= 'z') c -= 32;
        cardID[out++] = c;
      }

      cardID[out] = '\0';

      if (strcmp(cardID, lastCardID) != 0)
      {
        showMessageOnDisplay("CARD INSERTED", 0);

        if (localStorage) readPlayerDataFromSD(cardID, false, NULL, 0);
        else readPlayerDataFromServer(cardID);

        strcpy(lastCardID, cardID);
        Serial.println(F("Card Inserted"));
        Serial.print(F("Card ID: ")); Serial.println(lastCardID);

        // Can be Player 1, Admin 2, Bonus 3, or 0 if card data cannot be read from SD/Server

        if (cardType == 1)
        {
          Serial.println(F("Card Type: Player"));
          Serial.print(F("Cardholder: ")); Serial.println(cardHolder);
          Serial.print(F("Player Stats - Games played: ")); Serial.print(playerTotalGames);
          Serial.print(F(" Games Won: ")); Serial.print(playerGamesWon);
          Serial.print(F(" Games Lost: ")); Serial.print(playerGamesLost);
          Serial.print(F(" Total Won: ")); Serial.print(playerTotalWon);
          Serial.print(F(" Comps: ")); Serial.println(playerComps);
          
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

          showMessageOnDisplay("GOOD LUCK!", 0);
          setupPlayerMessage(false);
          Serial.println(F("Ready for play"));
          delay(2000);
          return true;
        }

        if (cardType == 2)
        {
          // Admin Menu
          Serial.println(F("Card Type: Admin"));
          enterAdminMenu();
            
          return true;
        }

        if (cardType == 3)
        {
          Serial.println(F("Card Type: System Bonus"));
          if (addCredits(creditsToAdd))
          {
            strcpy(scrollBuffer, "                    ");
            strcat(scrollBuffer, bonusMessage);
            strcat(scrollBuffer, "                    ");
          }
          return true;
        }

        showMessageOnDisplay("UNKNOWN CARD", 0);
        delay(2000);
      }
    }
    else
    {
      // Bad Card Swipe
      Serial.println(F("Unable to read card"));
      showMessageOnDisplay("CARD READ ERROR", 0);
      delay(2000);
      showMessageOnDisplay(casinoName, 0);
      return false;
    }
  }
  else
  {
    if (lastCardID[0] != '\0' && !card.available2())
    {
      // Card Removed
      setupAttractMessage();
      showMessageOnDisplay("CARD REMOVED", 0);
      Serial.println(F("Card Removed"));

      if (cardType == 2) {
        exitMenu();
        clearStats();
        return true;         
      }

      updatePlayerStats();
     
      // Clear variables
      clearStats();

      delay(2000);

      // Flush the buffer
      card.flush();
      
      return true;
    }
  }

  return false;
}
*/

// ------------------------------------------------------------------------------------------------------------
// Check for player card insertion - Generic RFID Reader
// ------------------------------------------------------------------------------------------------------------

bool checkForPlayerCard()
{
  if (card.PICC_IsNewCardPresent())
  {
    if (card.PICC_ReadCardSerial())
    {
      // Card present - identify
      cardID[0] = '\0';

      for (byte i = 0; i < card.uid.size; i++)
      {
        char hx[3];
        byte v = card.uid.uidByte[i];
        hx[0] = (v >> 4) < 10 ? '0' + (v >> 4) : 'A' + ((v >> 4) - 10);
        hx[1] = (v & 0x0F) < 10 ? '0' + (v & 0x0F) : 'A' + ((v & 0x0F) - 10);
        hx[2] = '\0';
        strncat(cardID, hx, sizeof(cardID) - strlen(cardID) - 1);
      }

      if (strcmp(cardID, lastCardID) != 0)
      {
        showMessageOnDisplay("CARD INSERTED", 0);

        if (localStorage) readPlayerDataFromSD(cardID, false, NULL, 0);
        else readPlayerDataFromServer(cardID);

        strcpy(lastCardID, cardID);
        Serial.println(F("Card Inserted"));
        Serial.print(F("Card ID: ")); Serial.println(lastCardID);

        // Can be Player 1, Admin 2, Bonus 3, or 0 if card data cannot be read from SD/Server

        if (cardType == 1)
        {
          Serial.println(F("Card Type: Player"));
          Serial.print(F("Cardholder: ")); Serial.println(cardHolder);
          Serial.print(F("Player Stats - Games played: ")); Serial.print(playerTotalGames);
          Serial.print(F(" Games Won: ")); Serial.print(playerGamesWon);
          Serial.print(F(" Games Lost: ")); Serial.print(playerGamesLost);
          Serial.print(F(" Total Won: ")); Serial.print(playerTotalWon);
          Serial.print(F(" Comps: ")); Serial.println(playerComps);
          
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

          showMessageOnDisplay("GOOD LUCK!", 0);
          setupPlayerMessage(false);
          Serial.println(F("Ready for play"));
          delay(2000);
          return true;
        }

        if (cardType == 2)
        {
          // Admin Menu
          Serial.println(F("Card Type: Admin"));
          enterAdminMenu();
            
          return true;
        }

        if (cardType == 3)
        {
          Serial.println(F("Card Type: System Bonus"));
          if (addCredits(creditsToAdd))
          {
            strcpy(scrollBuffer, "                    ");
            strcat(scrollBuffer, bonusMessage);
            strcat(scrollBuffer, "                    ");
          }
          return true;
        }

        showMessageOnDisplay("UNKNOWN CARD", 0);
        delay(2000);
      }
    }
    else
    {
      return false;
    }
  }
  else
  {
    bool current, previous;
    if (lastCardID[0] != '\0')
    {
      previous = true;
      current = !card.PICC_IsNewCardPresent();

      if (current && previous)
      {
        // Card Removed
        setupAttractMessage();
        showMessageOnDisplay("CARD REMOVED", 0);
        Serial.println(F("Card Removed"));

        if (cardType == 2) {
          exitMenu();
          clearStats();
          return true;         
        }

        updatePlayerStats();
      
        // Clear variables
        clearStats();

        delay(2000);
        return true;
      }
    }
  }

  return false;
}

// Update player stats

void updatePlayerStats()
{
  if (cardType != 1) return;
  
  if (haveStartingStats)
  {
    if (readGameData())
    {  
      //  Update player stats      
      playerTotalGames += (totalGames - tempTotalGames);
      playerGamesWon += (gamesWon - tempGamesWon);
      playerGamesLost += (gamesLost - tempGamesLost);
      playerTotalWon += (totalWon - tempTotalWon);
      playerComps += abs((totalIn - tempTotalIn) * compPercentage);
  
      // Update temp values
      tempTotalIn = totalIn;
      tempTotalWon = totalWon;
      tempTotalGames = totalGames;
      tempGamesWon = gamesWon;
      tempGamesLost = gamesLost;
    
      // In case it overflows
      if (playerTotalGames < 0 | playerGamesWon < 0 | playerGamesLost < 0 | playerTotalWon < 0)
      {
        Serial.println(F("Player stats for session are invalid and will not be saved!"));
      }
      else
      {
        Serial.println(F("Updating player stats"));
        if (playerComps < 0) playerComps = 0;
        if (localStorage) writePlayerDataToSD(lastCardID, cardType, cardHolder, playerTotalGames, playerGamesWon, playerGamesLost, playerTotalWon, playerComps);
        else writePlayerDataToServer(lastCardID, cardType, cardHolder, playerTotalGames, playerGamesWon, playerGamesLost, playerTotalWon, playerComps);
      }
    }
    else
    {
      Serial.println(F("Could not read current stats from game. Player's current session will not be saved."));
    } 
  } 
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
  playerComps = 0;
  haveStartingStats = false;
  strcpy(cardHolder, "No Card Inserted");
  cardID[0] = '\0';
  lastCardID[0] = '\0';
  cardType = 0;
  inAdminMenu=false;
  inPlayerMenu=false;
}

// ------------------------------------------------------------------------------------------------------------
// Game Functions
// ------------------------------------------------------------------------------------------------------------

// Add X credits to game

bool addCredits(const char *credits)
{
  if (!credits) return false;

  char amount[9];
  byte pos = 0;

  while (*credits == ' ' || *credits == '	') credits++;

  while (*credits >= '0' && *credits <= '9' && pos < sizeof(amount) - 1)
  {
    amount[pos++] = *credits++;
  }
  amount[pos] = '\0';

  unsigned long value = atol(amount);

  byte ac[4];
  ac[3] = dec2bcd(value % 100); value /= 100;
  ac[2] = dec2bcd(value % 100); value /= 100;
  ac[1] = dec2bcd(value % 100); value /= 100;
  ac[0] = dec2bcd(value % 100);

  if (LegacyBonus(0x01, ac[0], ac[1], ac[2], ac[3], 0x00))
  {
    strncpy(fixedBuffer, amount, sizeof(fixedBuffer) - 9);
    fixedBuffer[sizeof(fixedBuffer) - 9] = '\0';
    strcat(fixedBuffer, " credits");

    Serial.print(amount); Serial.println(F(" credits added"));
    showMessageOnDisplay(fixedBuffer, 0);
    showMessageOnDisplay("added", 1);
    delay(2000);
    showMessageOnDisplay(casinoName, 0);
    return true;
  }
  else return false;
}

// Read the game meters

bool readGameData()
{
  Serial.println(F("Reading meters from game"));

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
// Display Functions
// ------------------------------------------------------------------------------------------------------------

// Helper functions

static void appendToBuffer(char *dest, size_t destSize, const char *src)
{
  if (!dest || !src || destSize == 0) return;
  size_t len = strlen(dest);
  if (len < destSize - 1) strncat(dest, src, destSize - len - 1);
}

static void renderTokensToScroll()
{
  char src[sizeof(scrollBuffer)];
  strncpy(src, scrollBuffer, sizeof(src) - 1);
  src[sizeof(src) - 1] = '\0';

  scrollBuffer[0] = '\0';

  const char *p = src;
  while (*p && strlen(scrollBuffer) < sizeof(scrollBuffer) - 1)
  {
    if (!strncmp(p, "[CARDHOLDER]", 12))
    {
      appendToBuffer(scrollBuffer, sizeof(scrollBuffer), cardHolder);
      p += 12;
    }
    else if (!strncmp(p, "[CASINONAME]", 12))
    {
      appendToBuffer(scrollBuffer, sizeof(scrollBuffer), casinoName);
      p += 12;
    }
    else
    {
      char c[2] = {*p++, '\0'};
      appendToBuffer(scrollBuffer, sizeof(scrollBuffer), c);
    }
  }
}

// Show static message on Display

void showMessageOnDisplay(const char message[], int line)
{
  if (onlyTITO) return;

  if (line == 0) display.clear();
  int startPos = floor((displayWidth - strlen(message)*(displayWidth/displayCols))/2);
  display.setCursor(startPos, line * floor(displayHeight/displayRows));
  display.print(message);
}

// Scrolls message on Display

void scrollText(int printStart, int startLetter)
{
  display.setCursor(printStart, floor(displayHeight/displayRows));
  for (int letter = startLetter; letter <= startLetter + displayCols - 1; letter++)  // Print only X chars in Line #2 starting 'startLetter'
  {
    display.print(scrollBuffer[letter]);
  }

  display.print(" ");
  delay(scrollDelay);
}

bool setupAttractMessage()
{ 
  strcpy(scrollBuffer, "                    ");
  appendToBuffer(scrollBuffer, sizeof(scrollBuffer), attractMessage);
  appendToBuffer(scrollBuffer, sizeof(scrollBuffer), "                    ");
  renderTokensToScroll();
  resetScroll=true;
  return true;
}

bool setupPlayerMessage(bool skipBuffer)
{
  if (!skipBuffer)
  {
    strcpy(scrollBuffer, "                    ");
    appendToBuffer(scrollBuffer, sizeof(scrollBuffer), playerMessage);
  }

  appendToBuffer(scrollBuffer, sizeof(scrollBuffer), "                    ");
  renderTokensToScroll();

  if (playerComps > 1 && !skipBuffer)
  {
    appendToBuffer(scrollBuffer, sizeof(scrollBuffer), "You have Comp Credits available! Press [ENT] to access Player Menu.                    ");
  }

  resetScroll=true;
  return true;
}

// ------------------------------------------------------------------------------------------------------------
// Misc Functions
// ------------------------------------------------------------------------------------------------------------

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


// Get a querystring value without copying the full request line

static bool getQueryValue(const char* queryString, const char* key, char *output, size_t outputSize)
{
  if (output == NULL || outputSize == 0) return false;
  output[0] = '\0';

  if (queryString == NULL || key == NULL || key[0] == '\0') return false;

  size_t keyLen = strlen(key);
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
      size_t currentKeyLen = equals - pairStart;

      if (currentKeyLen == keyLen && strncmp(pairStart, key, keyLen) == 0)
      {
        const char *valueStart = equals + 1;
        size_t valueLen = pairEnd - valueStart;

        if (valueLen >= outputSize)
        {
          output[0] = '\0';
          return false;
        }

        memcpy(output, valueStart, valueLen);
        output[valueLen] = '\0';
        return true;
      }
    }

    pairStart = pairEnd;

    if (*pairStart == '&') pairStart++;
  }

  return false;
}


static bool getQueryValueByIndex(const char* queryString, byte index, char *output, size_t outputSize)
{
  if (output == NULL || outputSize == 0) return false;
  output[0] = '\0';

  if (queryString == NULL) return false;

  byte currentIndex = 0;
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

    if (currentIndex == index && equals < pairEnd)
    {
      const char *valueStart = equals + 1;
      size_t valueLen = pairEnd - valueStart;

      if (valueLen >= outputSize) valueLen = outputSize - 1;

      memcpy(output, valueStart, valueLen);
      output[valueLen] = '\0';
      return true;
    }

    pairStart = pairEnd;
    if (*pairStart == '&') pairStart++;
    currentIndex++;
  }

  return false;
}

static bool getPipeValue(const char *data, byte index, char *output, size_t outputSize)
{
  if (!data || !output || outputSize == 0) return false;
  output[0] = '\0';

  byte currentIndex = 0;
  const char *fieldStart = data;

  while (true)
  {
    const char *fieldEnd = fieldStart;
    while (*fieldEnd && *fieldEnd != '|') fieldEnd++;

    if (currentIndex == index)
    {
      size_t fieldLen = fieldEnd - fieldStart;
      if (fieldLen >= outputSize) fieldLen = outputSize - 1;
      memcpy(output, fieldStart, fieldLen);
      output[fieldLen] = '\0';
      return true;
    }

    if (*fieldEnd == '\0') break;
    fieldStart = fieldEnd + 1;
    currentIndex++;
  }

  return false;
}

static bool isHexDigit(char c)
{
  return ((c >= '0' && c <= '9') ||
          (c >= 'a' && c <= 'f') ||
          (c >= 'A' && c <= 'F'));
}

// Decode querystring parameters into a fixed output buffer.

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

static void httpPrintEncodedQueryValue(EthernetClient &client, const char *value)
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

static bool httpReadLine(EthernetClient &client, char *buffer, size_t bufferSize, unsigned long timeoutMs)
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

static unsigned long httpReadHeadersGetContentLength(EthernetClient &client)
{
  char headerLine[128];
  unsigned long contentLength = 0;

  while (httpReadLine(client, headerLine, sizeof(headerLine), 3000))
  {
    if (headerLine[0] == '\0') break;

    if (httpStartsWithIgnoreCase(headerLine, "Content-Length:"))
    {
      char *p = headerLine + 15;
      while (*p == ' ' || *p == '\t') p++;
      contentLength = strtoul(p, NULL, 10);
    }
  }

  return contentLength;
}

static void httpDiscardHeaders(EthernetClient &client)
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

  if (q == NULL) q = requestLine;
  else q++;

  size_t keyLen = strlen(key);

  while (*q && *q != ' ')
  {
    if (strncmp(q, key, keyLen) == 0)
    {
      char nextChar = q[keyLen];

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

static void httpSendOkText(EthernetClient &client, const __FlashStringHelper *text)
{
  client.print(textHeader);
  client.print(text);
}

static void httpPrintUiQueryParams(EthernetClient &client)
{
  client.print(ipStr);
  client.print(F("&board="));
  client.print(Board);
  client.print(F("&gn="));
  httpPrintEncodedQueryValue(client, gameName);
  client.print(F("&cp="));
  httpPrintEncodedQueryValue(client, cardHolder);
  client.print(F("&v="));
  httpPrintEncodedQueryValue(client, versionString);
}

static bool httpWritePostBodyToFile(EthernetClient &client, File &sdFile, unsigned long contentLength)
{
  unsigned long lastActivity = millis();
  unsigned long bytesWritten = 0;

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

static void httpRedirectToLocalUi(EthernetClient &client)
{
  client.print(F("HTTP/1.1 302 Found\r\nLocation: /?ip="));
  httpPrintUiQueryParams(client);
  client.print(F("\r\nConnection: close\r\n\r\n"));
}

static void httpServeLocalUi(EthernetClient &client)
{
  sdFile = SD.open("index.htm", O_READ);

  if (sdFile)
  {
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
  else
  {
    client.print(htmlHeader);
    client.print(F("<head><meta name='viewport' content='initial-scale=1.0'><style>body {font-family: Tahoma;}</style></head><body>"));
    client.print(F("<div style='max-width: 100%; margin: auto; text-align:center;'>"));
    client.print(F("<h2>Arduino TITO and Player Tracking</h2>"));
    client.print(F("Game Name: <b>"));
    client.print(gameName);
    client.print(F("</b>&nbsp;&nbsp;&nbsp;Current player: <b> "));
    client.print(cardHolder);
    client.print(F("</b><br>IP Address: <b>"));
    client.print(ipStr);
    client.print(F("</b>&nbsp;&nbsp;&nbsp;Version: <b> "));
    client.print(versionString);
    client.print(F("</b><br><hr>Unable to load User Interface from SD card!<br>This game can still be controlled remotely with the Game Manager Windows app."));
    client.print(F("</div></body>"));
    client.print(htmlFooter);

    Serial.println(F("Web Interface failed to load"));
  }
}

static bool uiStateIsStale(const char *queryString)
{
  if (queryString == NULL || queryString[0] == '\0') return true;

  char rawCp[100];
  char decodedCp[sizeof(cardHolder)];

  if (!getQueryValue(queryString, "cp", rawCp, sizeof(rawCp))) return true;
  if (!urlDecodeToBuffer(rawCp, decodedCp, sizeof(decodedCp))) return true;

  return strcmp(decodedCp, cardHolder) != 0;
}

// ------------------------------------------------------------------------------------------------------------
// HTML Server
// ------------------------------------------------------------------------------------------------------------

void htmlPoll()
{
  EthernetClient client = server.available();
  if (!client) return;

  client.setConnectionTimeout(1000);

  char requestLine[512];
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

    unsigned long contentLength = httpReadHeadersGetContentLength(client);

    sdFile = SD.open(fn, O_WRITE | O_CREAT | O_TRUNC);

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

        showMessageOnDisplay("REBOOTING", 0);
        delay(2000);
        resetFunc();
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

  char queryData[512];
  httpExtractQueryString(requestLine, queryData, sizeof(queryData));

  bool reqResult = false;
  bool commandFound = false;

  // ------------------------------------------------------------
  // Parse querystring - commands that return their own response
  // ------------------------------------------------------------

  if (httpHasQueryParam(requestLine, "bd")) // Board
  {
    client.print(textHeader);
    client.print(Board);
    client.stop();
    return;
  }

  if (httpHasQueryParam(requestLine, "ps")) // Player Statistics
  {
    client.print(htmlHeader);

    if (cardType == 1)
    {
      client.print(F("<head><meta name='viewport' content='initial-scale=1.0'><style>body {font-family: Tahoma;}</style></head><body><h2>PLAYER STATISTICS: "));

      if (readGameData())
      {
        client.print(cardHolder);
        client.print(F("</h2> (Live)<br><br>Total Games<br>"));
        client.print(playerTotalGames + (totalGames - tempTotalGames));
        client.print(F("<br>Games Won<br>"));
        client.print(playerGamesWon + (gamesWon - tempGamesWon));
        client.print(F("<br>Games Lost<br>"));
        client.print(playerGamesLost + (gamesLost - tempGamesLost));
        client.print(F("<br>Total Won<br>"));
        client.print(playerTotalWon + (totalWon - tempTotalWon));
        client.print(F("<br>Comps Earned<br>"));
        client.print(playerComps + ((totalIn - tempTotalIn) * compPercentage));
        client.print(F("<br>"));
      }
      else
      {
        client.print(cardHolder);
        client.print(F("</h2> (Saved)<br><br>Total Games<br>"));
        client.print(playerTotalGames);
        client.print(F("<br>Games Won<br>"));
        client.print(playerGamesWon);
        client.print(F("<br>Games Lost<br>"));
        client.print(playerGamesLost);
        client.print(F("<br>Total Won<br>"));
        client.print(playerTotalWon);
        client.print(F("<br>Comps Earned<br>"));
        client.print(playerComps);
        client.print(F("<br>"));
      }
    }
    else
    {
      client.print(F("<head><style>body {font-family: Tahoma;}</style></head><body><h2>NO PLAYER CARD INSERTED</h2><br>"));
    }

    client.print(F("</body>"));
    client.print(htmlFooter);
    client.stop();
    return;
  }

  if (httpHasQueryParam(requestLine, "ds")) // Game Statistics
  {
    client.print(htmlHeader);

    if (readGameData())
    {
      client.print(F("<head><meta name='viewport' content='initial-scale=1.0'><style>body {font-family: Tahoma;}</style></head><body><h2>GAME STATISTICS</h2><br>Credits<br>"));
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
      client.print(F("<head><style>body {font-family: Tahoma;}</style></head><body><h2>GAME STATISTICS NOT AVAILABLE</h2><br>"));
    }

    client.print(F("</body>"));
    client.print(htmlFooter);
    client.stop();
    return;
  }

  if (httpHasQueryParam(requestLine, "pd")) // Request player card data
  {
    char cid[32];

    if ((!getQueryValue(queryData, "cardID", cid, sizeof(cid)) || cid[0] == '\0') &&
        !getQueryValueByIndex(queryData, 1, cid, sizeof(cid)))
    {
      cid[0] = '\0';
    }

    client.print(textHeader);

    if (cardType == 1 && !strcmp(cid, "0"))
    {
      if (readGameData())
      {
        client.print(cardHolder);
        client.print('|');
        client.print(playerTotalGames + (totalGames - tempTotalGames));
        client.print('|');
        client.print(playerGamesWon + (gamesWon - tempGamesWon));
        client.print('|');
        client.print(playerGamesLost + (gamesLost - tempGamesLost));
        client.print('|');
        client.print(playerTotalWon + (totalWon - tempTotalWon));
        client.print('|');
        client.print(playerComps + ((totalIn - tempTotalIn) * compPercentage));
      }
      else
      {
        client.print(cardHolder);
        client.print('|');
        client.print(playerTotalGames);
        client.print('|');
        client.print(playerGamesWon);
        client.print('|');
        client.print(playerGamesLost);
        client.print('|');
        client.print(playerTotalWon);
        client.print('|');
        client.print(playerComps);
      }

      Serial.println(F("Current player data requested by host"));
    }
    else
    {
      if (strcmp(cid, "0") && cid[0] != '\0')
      {
        char playerData[180];
        char field[48];

        if (readPlayerDataFromSD(cid, true, playerData, sizeof(playerData)))
        {
          for (byte i = 0; i < 7; i++)
          {
            if (getPipeValue(playerData, i, field, sizeof(field))) client.print(field);
            client.print('|');
          }
        }
        else
        {
          client.print(F("||||||"));
        }

        Serial.print(F("Card Data Request: "));
        Serial.println(cid);
      }
      else
      {
        client.print(F("||||||"));
      }
    }

    client.stop();
    return;
  }

  if (httpHasQueryParam(requestLine, "gd")) // Request game data
  {
    client.print(textHeader);

    if (sasOnline && readGameData())
    {
      client.print(gameName);
      client.print('|');
      client.print(Credits);
      client.print('|');
      client.print(totalIn);
      client.print('|');
      client.print(totalWon);
      client.print('|');
      client.print(totalGames);
      client.print('|');
      client.print(gamesWon);
      client.print('|');
      client.print(gamesLost);
      client.print('|');
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

  if (httpHasQueryParam(requestLine, "cf")) // Request config data
  {
    client.print(textHeader);

    sdFile = SD.open("config.txt", O_READ);

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
      if (sdFile) sdFile.close();
      Serial.println(F("Unable to display config data"));
    }

    client.stop();
    return;
  }

  if (httpHasQueryParam(requestLine, "rb")) // Reboot Arduino
  {
    httpSendOkText(client, F("OK"));
    client.stop();

    showMessageOnDisplay("REBOOTING", 0);
    delay(2000);
    resetFunc();
    return;
  }

  // ------------------------------------------------------------
  // Parse querystring - commands that return simple OK / ERROR
  // ------------------------------------------------------------

  if (httpHasQueryParam(requestLine, "pu")) // Upload player card data to server
  {
    commandFound = true;

    char cid[32];
    char rawPlayerData[192];
    char playerData[192];
    char playerName[48];
    char field[16];
    bool ok = true;

    if ((!getQueryValue(queryData, "cardID", cid, sizeof(cid)) || cid[0] == '\0') &&
        !getQueryValueByIndex(queryData, 1, cid, sizeof(cid)))
    {
      cid[0] = '\0';
    }

    if ((!getQueryValue(queryData, "data", rawPlayerData, sizeof(rawPlayerData)) || rawPlayerData[0] == '\0') &&
        !getQueryValueByIndex(queryData, 2, rawPlayerData, sizeof(rawPlayerData)))
    {
      rawPlayerData[0] = '\0';
    }

    if (cid[0] == '\0' || rawPlayerData[0] == '\0' || !urlDecodeToBuffer(rawPlayerData, playerData, sizeof(playerData)))
    {
      ok = false;
    }

    int ct = 0;
    long pg = 0;
    long pw = 0;
    long pl = 0;
    long ptw = 0;
    float pc = 0;

    if (ok && !getPipeValue(playerData, 0, playerName, sizeof(playerName))) ok = false;
    if (ok && getPipeValue(playerData, 1, field, sizeof(field))) ct = atoi(field); else ok = false;
    if (ok && getPipeValue(playerData, 2, field, sizeof(field))) pg = atol(field); else ok = false;
    if (ok && getPipeValue(playerData, 3, field, sizeof(field))) pw = atol(field); else ok = false;
    if (ok && getPipeValue(playerData, 4, field, sizeof(field))) pl = atol(field); else ok = false;
    if (ok && getPipeValue(playerData, 5, field, sizeof(field))) ptw = atol(field); else ok = false;
    if (ok && getPipeValue(playerData, 6, field, sizeof(field))) pc = atof(field); else ok = false;

    reqResult = ok && writePlayerDataToSD(cid, ct, playerName, pg, pw, pl, ptw, pc);
  }

  if (httpHasQueryParam(requestLine, "mo")) { commandFound = true; reqResult = slotCommand(MUTE, 4, "Sound Off"); }
  if (httpHasQueryParam(requestLine, "mt")) { commandFound = true; reqResult = slotCommand(UMUTE, 4, "Sound On"); }
  if (httpHasQueryParam(requestLine, "lk")) { commandFound = true; reqResult = slotCommand(LOCK, 4, "Game Locked"); }
  if (httpHasQueryParam(requestLine, "uk")) { commandFound = true; reqResult = slotCommand(ULOCK, 4, "Game Unlocked"); }
  if (httpHasQueryParam(requestLine, "eb")) { commandFound = true; reqResult = slotCommand(EBILL, 4, "BV Enabled"); }
  if (httpHasQueryParam(requestLine, "db")) { commandFound = true; reqResult = slotCommand(DBILL, 4, "BV Disabled"); }
  if (httpHasQueryParam(requestLine, "jr")) { commandFound = true; reqResult = slotCommand(JREST, 4, "Jackpot Reset"); }

  if (httpHasQueryParam(requestLine, "ec"))
  {
    commandFound = true;
    reqResult = changeButtonToCredits(true);
  }

  if (httpHasQueryParam(requestLine, "dc"))
  {
    commandFound = true;
    reqResult = changeButtonToCredits(false);
  }

  if (httpHasQueryParam(requestLine, "pn")) // Rename player card
  {
    commandFound = true;

    if (cardType == 1)
    {
      char rawName[48];
      char decodedName[48];

      if ((!getQueryValue(queryData, "pn", rawName, sizeof(rawName)) || rawName[0] == '\0') &&
          !getQueryValueByIndex(queryData, 1, rawName, sizeof(rawName)))
      {
        rawName[0] = '\0';
      }

      if (rawName[0] != '\0' && urlDecodeToBuffer(rawName, decodedName, sizeof(decodedName)))
      {
        strncpy(cardHolder, decodedName, sizeof(cardHolder) - 1);
        cardHolder[sizeof(cardHolder) - 1] = '\0';
        reqResult = true;
      }
      else
      {
        reqResult = false;
      }

      showMessageOnDisplay(reqResult ? "Card Renamed" : "Rename Failed", 0);
      delay(2000);
    }
    else
    {
      reqResult = false;

      showMessageOnDisplay("No Card", 0);
      delay(2000);
    }
  }

  if (httpHasQueryParam(requestLine, "pc")) // Set comps to card
  {
    commandFound = true;

    if (cardType == 1)
    {
      char compsText[12];

      if ((!getQueryValue(queryData, "pc", compsText, sizeof(compsText)) || compsText[0] == '\0') &&
          !getQueryValueByIndex(queryData, 1, compsText, sizeof(compsText)))
      {
        compsText[0] = '\0';
      }

      if (compsText[0] != '\0')
      {
        playerComps = atoi(compsText);
        reqResult = true;

        // Call this now to write back the changed playerComps.
        // The other values will be updated when the card is removed.
        if (localStorage)
        {
          writePlayerDataToSD(lastCardID, cardType, cardHolder, playerTotalGames, playerGamesWon, playerGamesLost, playerTotalWon, playerComps);
        }
        else
        {
          writePlayerDataToServer(lastCardID, cardType, cardHolder, playerTotalGames, playerGamesWon, playerGamesLost, playerTotalWon, playerComps);
        }
      }
      else
      {
        reqResult = false;
      }

      showMessageOnDisplay(reqResult ? "Comps Applied" : "Comps Failed", 0);
      delay(2000);
    }
    else
    {
      reqResult = false;

      showMessageOnDisplay("No Card", 0);
      delay(2000);
    }
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
      reqResult = SetTicketData(location, address1, address2);
    }
    else if (getQueryValueByIndex(queryData, 0, rawLocation, sizeof(rawLocation)) &&
             getQueryValueByIndex(queryData, 1, rawAddress1, sizeof(rawAddress1)) &&
             getQueryValueByIndex(queryData, 2, rawAddress2, sizeof(rawAddress2)) &&
             urlDecodeToBuffer(rawLocation, location, sizeof(location)) &&
             urlDecodeToBuffer(rawAddress1, address1, sizeof(address1)) &&
             urlDecodeToBuffer(rawAddress2, address2, sizeof(address2)))
    {
      reqResult = SetTicketData(location, address1, address2);
    }
    else
    {
      reqResult = false;
    }

    showMessageOnDisplay("Ticket Updated", 0);
    delay(2000);
  }

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

  if (httpHasQueryParam(requestLine, "cm")) // Update Scrolling Message
  {
    commandFound = true;

    char msgID[8];
    char rawMsg[256];
    char decodedMsg[256];

    if ((!getQueryValue(queryData, "cm", msgID, sizeof(msgID)) || msgID[0] == '\0') &&
        !getQueryValueByIndex(queryData, 0, msgID, sizeof(msgID)))
    {
      msgID[0] = '\0';
    }

    if ((!getQueryValue(queryData, "msg", rawMsg, sizeof(rawMsg)) || rawMsg[0] == '\0') &&
        (!getQueryValue(queryData, "message", rawMsg, sizeof(rawMsg)) || rawMsg[0] == '\0') &&
        !getQueryValueByIndex(queryData, 1, rawMsg, sizeof(rawMsg)))
    {
      rawMsg[0] = '\0';
    }

    if (rawMsg[0] != '\0' && urlDecodeToBuffer(rawMsg, decodedMsg, sizeof(decodedMsg)))
    {
      if (!strcmp(msgID, "1"))
      {
        strcpy(scrollBuffer, "                    ");
        strncat(scrollBuffer, decodedMsg, sizeof(scrollBuffer) - strlen(scrollBuffer) - 1);
        reqResult = setupPlayerMessage(true);
      }
      else
      {
        strcpy(attractMessage, "                    ");
        strncat(attractMessage, decodedMsg, sizeof(attractMessage) - strlen(attractMessage) - 1);
        reqResult = setupAttractMessage();
      }

      Serial.print(F("Scrolling message updated: "));
      Serial.println(decodedMsg);
    }
    else
    {
      reqResult = false;
    }
  }

  // ------------------------------------------------------------
  // If a command was found and it did not already return its own
  // response, return simple OK / ERROR.
  // ------------------------------------------------------------

  if (commandFound)
  {
    httpSendOkText(client, reqResult ? F("OK") : F("ERROR"));
    client.stop();

    showMessageOnDisplay(casinoName, 0);
    return;
  }

  // ------------------------------------------------------------
  // No command found. Non-command GET requests load the UI.
  // ------------------------------------------------------------

  // Root page requests get redirected once so the browser URL has the UI state.
  // Requests that already have a querystring serve the SD-served page directly.
  if (strstr(requestLine, "GET / ") == requestLine || uiStateIsStale(queryData))
  {
    httpRedirectToLocalUi(client);
  }
  else
  {
    httpServeLocalUi(client);
  }

  client.stop();

  showMessageOnDisplay(casinoName, 0);
  return;
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

  UCSR1B = 0b10011101;
  Serial1.write(0x80);
  delay(20);
  Serial1.write(0x81);
  UCSR1B = 0b10011100;

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
      default:
        break;
    }
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
      showMessageOnDisplay("GAME TIMEOUT", 0); 
      Serial.println(F("Timeout waiting for ACK"));
      delay(2000);
      return false;
  }
  
  Serial.println(msg);
  showMessageOnDisplay(msg, 0);
  delay(2000);
  return true;
}

long pollMeters(byte * meter)
{
  byte meterData[8];

  SendTypeR(meter, sizeof(meter));
  waitForResponse(meter[1], meterData, sizeof(meterData));

  long value = 0;
  for (byte i = 2; i < 6; i++)
  {
    value = (value * 100) + bcd2dec(meterData[i]);
  }

  return value;
}

bool slotCommand(byte cmd[], int len, const char msg[])
{
  SendTypeS(cmd, len);
  return waitForACK(SASAdr, msg);
}

bool changeButtonToCredits(bool e)
{
  if (e) {
    changeToCredits = true;
    Serial.println(F("Change to Credits Enabled"));
    showMessageOnDisplay("C-to-C Enabled", 0);
  }
  else
  {
    changeToCredits = false;
    Serial.println(F("Change to Credits Disabled"));
    showMessageOnDisplay("C-to-C Disabled", 0);
  }
  return true;
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

bool SetTicketData(const char *loc, const char *addr1, const char *addr2)
{
  if (!loc) loc = "";
  if (!addr1) addr1 = "";
  if (!addr2) addr2 = "";

  byte locLen = strlen(loc);
  byte addr1Len = strlen(addr1);
  byte addr2Len = strlen(addr2);
  byte ticketData[11 + locLen + addr1Len + addr2Len];
  byte bSize = 6 + locLen + addr1Len + addr2Len;

  ticketData[0] = SASAdr;
  ticketData[1] = 0x7D;
  ticketData[2] = bSize;
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
