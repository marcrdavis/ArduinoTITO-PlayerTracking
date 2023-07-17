/*
  Arduino TITO and Player Tracking v2.0.20230706 Ethernet + RFID
  by Marc R. Davis - Copyright (c) 2020-2022 All Rights Reserved
  https://github.com/marcrdavis/ArduinoTITO-PlayerTracking

  Portions of the Arduino SAS protocol implementation by Ian Walker - Thank you!
  Additional testing and troubleshooting by NLG member Eddiiie - Thank you!

  Hardware requirements: 
    Arduino Mega 2560 R3; RFID RC 522; W5100 Ethernet Shield; Serial Port;
    Compatible vacuum fluorescent display or LCD; if using a display other than the default LCD then
    modifications will be required - see inline comments; Compatible keypad; if using a keypad other than
    the default Bally 6x2/3x4 then modifications will be required - see inline comments; Modifications will 
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
// Display Includes - Enable ONLY ONE Group
// ------------------------------------------------------------------------------------------------------------

// IEE VFDs
//#include <IeeFlipNoFrills.h>      // Enable for IEE VFDs; Disable other display includes

// Futaba VFDs
//#include <FutabaVFD.h>            // Enable for Futaba VFDs; Disable other display includes

// LCDs
#include <LiquidCrystal.h>          // Enable this for LCDs; Disable other display includes

// Noritake GU-7000s
//#include <GU7000_Interface.h>     // Enable this for GU-7000 Series VFDs; Disable other display includes
//#include <GU7000_Serial_Async.h>  // Enable this for GU-7000 Series VFDs; Disable other display includes
//#include <Noritake_VFD_GU7000.h>  // Enable this for GU-7000 Series VFDs; Disable other display includes

// ------------------------------------------------------------------------------------------------------------
// Required Libraries
// ------------------------------------------------------------------------------------------------------------

#include <IniFile.h>
#include <SPI.h>
#include <MFRC522.h>
#include <SD.h>
#include <Ethernet.h>
#include <EEPROM.h>
#include <Keypad.h>
#include <TimeLib.h>

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
unsigned long startTime = 0;
unsigned long endTime = 0;
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
bool inTournament = false;
bool timeExpired = false;
bool waitingForStart = false;
bool isLocked = false;
bool resetScroll = false;

String ipStr;
String cardHolder = "No Card Inserted";
String lastCardID;
String cardID;
String creditsToAdd = "1000";
String changeCredits = "100";
String gameName = "Slot Machine";
String stringData = "";
String versionString = "2.0.20230706";

char ipAddress[15];
char casinoName[30] = "THE CASINO";  // actual text should not exceed the display width
char attractMessage[256] = "Welcome to [CASINONAME]! Enjoy your stay! Please insert your Player Card";
char playerMessage[256] = "Welcome back [CARDHOLDER]! Enjoy your stay in [CASINONAME]!";
char bonusMessage[100] = "More credits means MORE FUN! Please remove the System Bonus Card now and insert your Player Card.";
char tournamentMessage[30] = "TOURNAMENT PLAY IN PROGRESS";
char adminMenu[150] = "1=Add Credits  2=Sound On  3=Sound Off  4=Unlock Game  5=Lock Game  6=Enable BV  7=Disable BV  8=Change-Credits On  9=Change-Credits Off  0=Exit Menu";
char playerMenu[55] = "1=Show Comp Balance  2=Use Comp Credits  0=Exit Menu";
char scrollBuffer[296];
char fixedBuffer[21];

const char htmlHeader[] = "HTTP/1.1 200 OK\r\n"
                          "Content-Type: text/html\r\n"
                          "Connection: close\r\n\r\n"
                          "<!DOCTYPE HTML>\r\n"
                          "<html>\r\n";
                          
const char textHeader[] = "HTTP/1.1 200 OK\r\n"
                          "Content-Type: text/plain\r\n"
                          "Connection: close\r\n\r\n";
 
const char htmlFooter[] = "</html>\r\n\r\n";

byte mac[] = { 0x38, 0x24, 0x01, 0x00, 0x00, 0x00 }; // Will be set by init routine
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
byte RHP[4] = {SASAdr, 0x94, 0x00, 0x00};
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
// Displays - Enable ONLY ONE Group
// ------------------------------------------------------------------------------------------------------------

// IEE VFDs; Pins 22, 23 = Control Pins; Pins 31-24 = data pins
//IeeFlipNoFrills vfd(22, 23, 31, 30, 29, 28, 27, 26, 25, 24);

// FutabaVFD VFDs; Pins 22 = Reset, 23 = Write; Pins 31-24 = data pins DB7-DB0
//FutabaVFD vfd(22, 23, 31, 30, 29, 28, 27, 26, 25, 24);

// LCDs (4 bit mode); Pins 22 = RS, 23 = Enable, 24 = DB7, 25 = DB6, 26 = DB5, 27 = DB4
LiquidCrystal vfd(22, 23, 27, 26, 25, 24);

// LCDs (8 bit mode); Pins 22 = RS, 23 = Enable, 24 = DB7, 25 = DB6, 26 = DB5, 27 = DB4, 28 = DB3, 29 = DB2, 30 = DB1, 31 = DB0
//LiquidCrystal vfd(22, 23, 31, 30, 29, 28, 27, 26, 25, 24);

// Noritake GU-7000 Series VFD; Pins 3 = SIN, 5 = BUSY, 7 = RESET; There is also code to enable in setup()
// GU7000_Serial_Async interface(38400, 3, 5, 7); 
// Noritake_VFD_GU7000 vfd;

// ------------------------------------------------------------------------------------------------------------
// Keypads - Enable ONLY ONE Group
// ------------------------------------------------------------------------------------------------------------

// Bally 6x2 Keypad P/N 105123F & 3x4 PCDSKEY1
const int ROW_NUM = 3; //  6x2 Keypad presents as a 3x4 matrix
const int COLUMN_NUM = 4; 

// Key Matrix
char keys[ROW_NUM][COLUMN_NUM] = {
  {'1','2','3','C'},
  {'4','5','6','0'},
  {'7','8','9','E'}
};

// Pin definition
byte pin_rows[ROW_NUM] = {39, 38, 41}; // Keypad Pins 1,2,3
byte pin_column[COLUMN_NUM] = {45, 42, 43, 40}; // Keypad Pins 7,6,5,4

// ACT 8x2 Keypad
/*const int ROW_NUM = 4; // Keypad is physically 8x2 but presents as a 4x4 matrix
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
byte pin_column[COLUMN_NUM] = {42, 43, 44, 45}; // Keypad Pins 5,6,7,8*/

// ------------------------------------------------------------------------------------------------------------
// Setup instances
// ------------------------------------------------------------------------------------------------------------

// Pins for RFID
#define SS_PIN 53  
#define RST_PIN 49 

MFRC522 mfrc522(SS_PIN, RST_PIN);
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
  // Variable Reserves
  lastCardID.reserve(9);
  cardID.reserve(9);
  cardHolder.reserve(30);
  creditsToAdd.reserve(8);
  changeCredits.reserve(8);
  ipStr.reserve(15);
  gameName.reserve(30);
  versionString.reserve(15);
  stringData.reserve(296);

  // Setup RTC
  setSyncProvider(requestSync);
  
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
    // Setup VFD
    vfd.begin(displayWidth, displayHeight);

    // Enable for Noritake GU-7000 Series
    /*delay(500);
    vfd.interface(interface);
    vfd.isModelClass(7003); // Set based on model of display
    vfd.GU7000_reset();
    vfd.GU7000_init(); */ 

    // Setup RFID
    mfrc522.PCD_Init();    
  }
  else Serial.println(F("TITO Only Mode Enabled"));
  
  showMessageOnVFD("Initializing...", 0);
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
    if (!inAdminMenu && !inPlayerMenu && !inTournament && !isLocked) showMessageOnVFD(casinoName, 0);
    
    // Scroll Text Loop
    for (int letter = 0; letter <= strlen(scrollBuffer) - displayCols; letter++) //From 0 to up to n-displayWidth characters supply to below function
    {
      if (resetScroll) return;
      if (!isLocked) scrollText(0, letter);
      
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
        showMessageOnVFD("ENTER ADMIN PIN", 0); 
        if (readKeypad(true) == String(adminPin)) enterAdminMenu();
        return;
      }

      // Access player menu
      if (cardType == 1 && key == 'E' && !inAdminMenu && !inPlayerMenu && !inTournament && !isLocked) {
        enterPlayerMenu();
        return;
      }      

      // Display IP and Version Info
      if (key == '0' && !inAdminMenu && !inPlayerMenu) {
          showMessageOnVFD(("VER " + versionString).c_str(), 0);
          showMessageOnVFD(ipStr.c_str(), 1);
          delay(3000);
          exitMenu();
          return;
      }

      // Tournament timing
      if (inTournament && waitingForStart) {
        if (startTime - now() <=0) {
          // Start Tournament
          waitingForStart=false;
          timeExpired=false;
          winMeterStart = pollMeters(mTotWon);
          slotCommand(ULOCK,4,"Game Unlocked");
          vfd.clear();
          showMessageOnVFD("Score: 0",0);
        }
        else 
        {
          showMessageOnVFD("GAME STARTS IN", 0);
          char b[20];
          stringData = " " + String(startTime - now()) + " second(s) "; 
          stringData.toCharArray(b, stringData.length() + 1); 
          showMessageOnVFD(b, 1);
        }
      }

      // Tournament progress
      if (inTournament && !waitingForStart  && !timeExpired) {
        if (endTime - now() <= 0) {
          // End Tournament play
          timeExpired=true;
          slotCommand(LOCK,4,"Game Locked");
          vfd.clear();
          showMessageOnVFD("TOURNAMENT OVER", 0);
          showMessageOnVFD("PLEASE WAIT", 1);
          delay(2000);                    
        }
        else if (inTournament && !timeExpired) {
          totalWon = pollMeters(mTotWon);
          if (totalWon>0) tournamentScore = totalWon - winMeterStart;
          char b[20];
          stringData = "Score: " + String(tournamentScore);
          stringData.toCharArray(b, stringData.length() + 1); 
          showMessageOnVFD(b,0);        
        }           
      }      
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

  stringData = String(creds) + " credits";
  stringData.toCharArray(b, stringData.length() + 1);  
  showMessageOnVFD("PLAYER COMPS",0);
  showMessageOnVFD(b,1);
  delay(2000);
  
  showMessageOnVFD("PLAYER MENU",0);
}

void useCompCredits()
{
  int creds = 0;
  
  if (haveStartingStats && readGameData()) playerComps += abs((totalIn - tempTotalIn) * compPercentage);
  creds = playerComps;
    
  if (creds <1){
    showMessageOnVFD("NO COMPS AVAIL",0);
    delay(2000);
  }
  else {
   if (addCredits(String(creds)))
   {
      // Reset comps to zero
      playerComps=0;    
  
      // Call this now to write back the changed playerComps; the other values will be updated when the card is removed
      if (localStorage) writePlayerDataToSD(lastCardID, cardType, cardHolder, playerTotalGames, playerGamesWon, playerGamesLost, playerTotalWon, playerComps);
      else writePlayerDataToServer(lastCardID, cardType, cardHolder, playerTotalGames, playerGamesWon, playerGamesLost, playerTotalWon, playerComps);
   }
   else
   {
      showMessageOnVFD("ERROR - RETRY",0);
      delay(2000);
   }
  }
  showMessageOnVFD("PLAYER MENU",0);
}

void adminAddCredits()
{
  showMessageOnVFD("AMOUNT TO ADD",0);
  
  String credits = readKeypad(false);
  if (credits != "") addCredits(credits);
  showMessageOnVFD("ADMIN MENU",0);
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
  showMessageOnVFD("PLAYER MENU",0);
  inPlayerMenu=true;
  Serial.println(F("Player Menu Active"));
  strcpy(scrollBuffer, "                    ");
  strcat(scrollBuffer, playerMenu);
  strcat(scrollBuffer, "                    ");
}

void enterAdminMenu()
{
  showMessageOnVFD("ADMIN MENU",0);
  inAdminMenu=true;
  Serial.println(F("Admin Menu Active"));
  strcpy(scrollBuffer, "                    ");
  strcat(scrollBuffer, adminMenu);
  strcat(scrollBuffer, "                    ");
}

// Get keypad input
String readKeypad(bool mask)
{
  stringData = "";
  char key;

  key = keypad.getKey();
  while (key != 'E')
  {
    if (key == 'C') { 
      stringData = "";
      showMessageOnVFD("                    ",1); 
    }
    else {
      if (key) { 
        stringData += String(key); 
        char b[20];
        if (mask)
        {
          String s = "*********************";
          s.toCharArray(b, stringData.length() + 1);
        }
        else
        {          
          stringData.toCharArray(b, stringData.length() + 1);
        }
        strcpy(fixedBuffer, b);
        showMessageOnVFD(b,1); 
      }
    }
    
    key = keypad.getKey();
    delay(1);

    // Must continue checking game
    generalPoll();
  }
  return stringData;
}

// ------------------------------------------------------------------------------------------------------------
// Tournament functions
// ------------------------------------------------------------------------------------------------------------

bool startTournamentMode(String credits)
{
  // if arduino clock is slow then start time will not be right - compensate
  if (startTime - now()>30) {
    int diff = (startTime - now())-30;
    startTime -= diff;
    endTime -= diff;
  }
          
  inTournament=true;
  waitingForStart=true;
  addCredits(credits);
  delay(1000);
  slotCommand(DBILL,4,"BV Disabled");
  delay(500);
  slotCommand(LOCK,4,"Game Locked");
  tournamentScore = 0; 
    
  Serial.println(F("Tournament Started"));
  vfd.clear();
  strcpy(scrollBuffer, "                    ");
  strcat(scrollBuffer, tournamentMessage);
  strcat(scrollBuffer, "                    ");
   
  return true;
}

void exitTournamentMode()
{
   // Reset display attract mode to last state
  Serial.println(F("Tournament Finished"));
  totalWon = pollMeters(mTotWon);
  if (totalWon>0) tournamentScore = totalWon - winMeterStart;
          
  slotCommand(EBILL,4,"BV Enabled");
  inTournament=false;
  waitingForStart=false;
  startTime=0;
  endTime=0;

  vfd.clear();
  showMessageOnVFD("WAITING ON HOST",1); 
  if (cardType == 1) setupPlayerMessage(false);
  else setupAttractMessage();  
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
  if (ini.getValue(NULL, "changeCredits", buffer, 256)) changeCredits = String(buffer);
  if (ini.getValue(NULL, "gameName", buffer, 256)) gameName = String(buffer);
  if (ini.getValue(NULL, "compPercentage", buffer, 256)) compPercentage = atof(buffer);
  if (ini.getValue(NULL, "autoAddCredits", buffer, 256)) autoAddCredits =  atoi(buffer);
  if (ini.getValue(NULL, "creditFloor", buffer, 256)) creditFloor = atol(buffer);

  if (creditFloor == 0 && autoAddCredits == 1)
  {
    // creditFloor cannot be zero or ticket cashout will not work properly
    autoAddCredits = 0;
    Serial.println(F("Unable to enable autoAddCredits because creditFloor is set to 0"));
  }

    if (changeCredits == 0 && autoAddCredits == 1)
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

// Read player data from the SD card

String readPlayerDataFromSD(String cid, bool remote)
{
  stringData = "";
  String CardFilename = cid + ".txt";
  IniFile ini(CardFilename.c_str());

  if (!SD.exists(CardFilename))
  {
    // Create file with defaults
    writePlayerDataToSD(cid, 1, "Player", 0, 0, 0, 0, 0);
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
      if (ini.getValue(NULL, "creditsToAdd", buffer, 30)) stringData += String(buffer) + "|";
      if (ini.getValue(NULL, "playerComps", buffer, 30)) stringData += String(buffer);
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
      if (ini.getValue(NULL, "playerComps", buffer, 30)) playerComps = atof(buffer);
     
      if (cardHolder == "") cardHolder = "Player";
      if (playerComps < 0) playerComps = 0;
    }
    ini.close();
  }

  return stringData;
}

// Write player data to remote server

void writePlayerDataToServer(String cid, int ct, String cn, long pg, long pw, long pl, long ptw, float pc)
{
  EthernetClient client;
  String sIP = String(serverIP[0]) + '.' + String(serverIP[1]) + '.' + String(serverIP[2]) + '.' + String(serverIP[3]);
  cn.replace(" ", "+");
  String playerData = cn + "|" + String(ct) + "|" + String(pg) + "|" + String(pw) + "|" + String(pl) + "|" + String(ptw) + "|" + String(pc);

  if (client.connect(serverIP, 80)) {
    Serial.print(F("Connected to "));
    Serial.println(client.remoteIP());

    // Make an HTTP request
    client.println("GET /?pu=&cardID=" + cid + "&data=" + playerData + " HTTP/1.1");
    client.println("Host: " + sIP);
    client.println("Connection: close");
    client.println();

    Serial.print(F("Card data uploaded for: ")); Serial.println(cid);
    client.stop();
    return;

  } else {
    Serial.println(F("Connection to server failed!"));
    showMessageOnVFD("Card Update Fail", 0);
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
    Serial.print(F("Connected to ")); Serial.println(client.remoteIP());

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

    stringData = client.readStringUntil('\n'); // this should contain our data

    if (stringData.indexOf("|") == -1) {
      clearStats();
      client.stop();
      Serial.print(F("Unable to get card data for: ")); Serial.print(cid);
      showMessageOnVFD("Card Load Fail", 0);
      delay(2000);
      return;
    }

    cardHolder = getValue(stringData, '|', 0);
    cardType = getValue(stringData, '|', 1).toInt();
    if (cardType == 3)
    {
      creditsToAdd = getValue(stringData, '|', 2);
    }
    else if (cardType == 1)
    {
      playerTotalGames = getValue(stringData, '|', 2).toInt();
      playerGamesWon = getValue(stringData, '|', 3).toInt();
      playerGamesLost = getValue(stringData, '|', 4).toInt();
      playerTotalWon = getValue(stringData, '|', 5).toInt();
      playerComps = getValue(stringData, '|', 6).toInt();
    }

    Serial.print(F("Card data downloaded for: ")); Serial.println(cid);
    client.stop();
    return;

  } else {
    Serial.println(F("Connection to server failed!"));
    showMessageOnVFD("Card Load Fail", 0);
    delay(2000);
  }
}

// Write player data to SD card

bool writePlayerDataToSD(String cid, int ct, String cn, long pg, long pw, long pl, long ptw, float pc)
{
  String CardFilename = cid + ".txt";
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
    showMessageOnVFD("Card Update Fail", 0);
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

// Initialize the Ethernet Shield

void initEthernet()
{
  // Generate unique MAC Address
  getMacAddress(mac);

  Ethernet.init(10);  
  
  // Start the Ethernet connection and the server
  if (useDHCP) Ethernet.begin(mac);
  else Ethernet.begin(mac, ip);

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println(F("Ethernet shield was not found. Remote Access will not be available."));

    showMessageOnVFD("No Network", 0);
    delay(2000);
    return;
  }
  
  Serial.print(F("IP address: "));
  Serial.println(Ethernet.localIP());
  ip = Ethernet.localIP();
  ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);

  showMessageOnVFD("Net Connected", 0);
  showMessageOnVFD(ipStr.c_str(), 1);
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
      ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
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
      ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      break;

    default:
      // Nothing happened
      break;
  }
}

// ------------------------------------------------------------------------------------------------------------
// Card and player functions
// ------------------------------------------------------------------------------------------------------------

// Check for player card insertion

bool checkForPlayerCard()
{
  if (mfrc522.PICC_IsNewCardPresent())
  {
    if (mfrc522.PICC_ReadCardSerial())
    {
      // Card present - identify
      cardID = "";

      for (byte i = 0; i < mfrc522.uid.size; i++)
      {
        cardID.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
        cardID.concat(String(mfrc522.uid.uidByte[i], HEX));
      }

      cardID.trim();
      cardID.toUpperCase();
      cardID = cardID.substring(1);
      cardID.replace(" ", "");

      if (cardID != lastCardID)
      {
        showMessageOnVFD("CARD INSERTED", 0);

        if (localStorage) readPlayerDataFromSD(cardID, false);
        else readPlayerDataFromServer(cardID);

        lastCardID = cardID;
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

          showMessageOnVFD("GOOD LUCK!", 0);
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

        showMessageOnVFD("UNKNOWN CARD", 0);
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
    if (lastCardID != "")
    {
      previous = true;
      current = !mfrc522.PICC_IsNewCardPresent();

      if (current && previous)
      {
        // Card Removed
        setupAttractMessage();
        showMessageOnVFD("CARD REMOVED", 0);
        Serial.println(F("Card Removed"));

        if (cardType == 2) {
          exitMenu();
          clearStats();
          return true;         
        }

        updatePlayerStats();

        if (inTournament)
        {
          Serial.println(F("Card removed while in Tournament Play. Player is no longer in game."));
          showMessageOnVFD("WAIT FOR HOST", 0);
          slotCommand(LOCK,4,"Game Locked");
        }
        
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
  startTime = 0;
  endTime = 0;
  haveStartingStats = false;
  cardHolder = "No Card Inserted";
  cardID="";
  lastCardID = "";
  cardType = 0;
  stringData = "";
  inAdminMenu=false;
  inPlayerMenu=false;
  inTournament=false;
  waitingForStart=false;
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
    if (!inTournament)
    {
      char b[10];
      credits.toCharArray(b, credits.length() + 1);
    
      strcpy(fixedBuffer, b);
      strcat(fixedBuffer, " credits");
      Serial.print(credits); Serial.println(F(" credits added"));
      showMessageOnVFD(fixedBuffer, 0);
      showMessageOnVFD("added", 1);
      delay(2000);
      showMessageOnVFD(casinoName, 0);
    }
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

// Show static message on VFD

void showMessageOnVFD(const char message[], int line)
{
  if (onlyTITO) return;

  if (line == 0 && !inTournament) vfd.clear();
  int startPos = floor((displayWidth - strlen(message)*(displayWidth/displayCols))/2);
  vfd.setCursor(startPos, line * floor(displayHeight/displayRows));
  vfd.print(message);
}

// Scrolls message on VFD

void scrollText(int printStart, int startLetter)
{
  vfd.setCursor(printStart, floor(displayHeight/displayRows));
  for (int letter = startLetter; letter <= startLetter + displayCols - 1; letter++)  // Print only X chars in Line #2 starting 'startLetter'
  {
    vfd.print(scrollBuffer[letter]);
  }

  vfd.print(" ");
  delay(scrollDelay);
}

bool setupAttractMessage()
{ 
  strcpy(scrollBuffer, "                    ");
  strcat(scrollBuffer, attractMessage);
  strcat(scrollBuffer, "                    ");

  // Look for variables in message and replace
  stringData = String(scrollBuffer);
  stringData.replace("[CARDHOLDER]", cardHolder);
  stringData.replace("[CASINONAME]", casinoName);
  stringData.toCharArray(scrollBuffer, stringData.length() + 1);
  resetScroll=true;
  return true;
}

bool setupPlayerMessage(bool skipBuffer)
{
  if (!skipBuffer) {
    strcpy(scrollBuffer, "                    ");
    strcat(scrollBuffer, playerMessage);
  }
  strcat(scrollBuffer, "                    ");
  
  // Look for variables in message and replace
  stringData = String(scrollBuffer);
  stringData.replace("[CARDHOLDER]", cardHolder);
  stringData.replace("[CASINONAME]", casinoName);  
  if (playerComps>1 && !skipBuffer) stringData += "You have Comp Credits available! Press [ENT] to access Player Menu.                    ";
  stringData.toCharArray(scrollBuffer, stringData.length() + 1); 
  resetScroll=true;
  return true;
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

// Get a MAC Address for ethernet

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

// Set Time

void processSyncMessage(unsigned long pctime)
{
  const unsigned long DEFAULT_TIME = 1613347200; // 2-15-2021 00:00:00 GMT
 
  if( pctime >= DEFAULT_TIME) { // check the integer is a valid time (greater than Jan 1 2013)
  setTime(pctime); // Sync Arduino clock to the time received from the host
  Serial.println("Arduino clock set by host");
  }
}

// Not used yet

time_t requestSync()
{
  Serial.println("Arduino time sync may be off");
  return 0;
}

// ------------------------------------------------------------------------------------------------------------
// HTML Server
// ------------------------------------------------------------------------------------------------------------

void htmlPoll()
{
  EthernetClient client = server.available();
  if (!client) return;
  client.setConnectionTimeout(1000);
  
  if (client.connected()) // If client is present and connected
  {
    String url = "";
    String request = "";
    String querystring = "";
    bool reqResult = false;

    stringData = client.readStringUntil('\n'); // Get the first line of request
    
    // Check for specific POST for SD card updates
    if (stringData.indexOf("POST") != -1 && stringData.indexOf("UPDATE") != -1) 
    { 
      String fn = "config.txt";
      if (stringData.indexOf("UPDATEHTML") != -1) fn = "index.htm";
            
      // Find and discard headers
      while (client.available() > 0)
      {
        stringData = client.readStringUntil('\n');
        if (stringData == "\r") break; // end of headers - this might be an endless loop if response is malformed - look into this
      }   

      sdFile = SD.open(fn, O_WRITE | O_CREAT | O_TRUNC);

      // if the file opened okay, write to it:
      if (sdFile)
      {
        while (client.available() > 0)
        {
          char c = client.read();
          sdFile.print(c);
        }
 
        // Close the file
        sdFile.close();
        Serial.print(fn);Serial.println(F(" updated. Restarting..."));  

        // Return status/result to host     
        client.print(textHeader); 
        client.print(F("OK"));
        client.stop();

        showMessageOnVFD("REBOOTING", 0);
        delay(2000);
        resetFunc();
      }
      else
      {
        // Unable to write file
        Serial.print(F("Unable to write file: "));Serial.println(fn);
        
        // Return status/result to host     
        client.print(textHeader); 
        client.print(F("ERROR"));
        client.stop();
        return;
      }
    }

    // Check for known GET commands
    while (client.available()) client.read(); // Get the rest of the header and discard

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
        client.print(htmlHeader);

        if (cardType == 1)
        {
          client.print(F("<head><meta name='viewport' content='initial-scale=1.0'><style>body {font-family: Tahoma;}</style></head><body><h2>PLAYER STATISTICS: "));
          if (readGameData())
          {
            client.print(cardHolder + "</h2> (Live)<br><br>");
            client.print("Total Games<br>" + String(playerTotalGames + (totalGames - tempTotalGames)) + "<br>Games Won<br>" + String(playerGamesWon + (gamesWon - tempGamesWon)) + "<br>Games Lost<br>" + String(playerGamesLost + (gamesLost - tempGamesLost)) + "<br>Total Won<br>" + String(playerTotalWon + (totalWon - tempTotalWon)) + "<br>Comps Earned<br>" + String(playerComps + ((totalIn - tempTotalIn) * compPercentage)) + "<br>");
          }
          else
          {
            client.print(cardHolder + "</h2> (Saved)<br><br>");
            client.print("Total Games<br>" + String(playerTotalGames) + "<br>Games Won<br>" + String(playerGamesWon) + "<br>Games Lost<br>" + String(playerGamesLost) + "<br>Total Won<br>" + String(playerTotalWon) + "<br>Comps Earned<br>" + String(playerComps) + "<br>");
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

      if (command == "ds") // Game Statistics
      {
        client.print(htmlHeader);
        if (readGameData())
        {
          client.print(F("<head><meta name='viewport' content='initial-scale=1.0'><style>body {font-family: Tahoma;}</style></head><body><h2>GAME STATISTICS</h2><br>"));
          client.print("Credits<br>" + String(Credits) + "<br>Total In<br>" + String(totalIn) + "<br>Total Won<br>" + String(totalWon) + "<br>Total Games<br>" + String(totalGames) + "<br>Games Won<br>" + String(gamesWon) + "<br>Games Lost<br>" + String(gamesLost) + "<br>");
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

      if (command == "pd")  // Request player card data
      {
        String cid = getValue(getValue(querystring, '&', 1), '=', 1);
        client.print(textHeader);
            
        if (cardType == 1 && cid == "0")
        {
          if (readGameData())
          {
            client.print(cardHolder + "|" + String(playerTotalGames + (totalGames - tempTotalGames)) + "|" + String(playerGamesWon + (gamesWon - tempGamesWon)) + "|" + String(playerGamesLost + (gamesLost - tempGamesLost)) + "|" + String(playerTotalWon + (totalWon - tempTotalWon)) + "|" + String(playerComps + ((totalIn - tempTotalIn) * compPercentage)));
          }
          else
          {
            client.print(cardHolder + "|" + String(playerTotalGames) + "|" + String(playerGamesWon) + "|" + String(playerGamesLost) + "|" + String(playerTotalWon) + "|" + String(playerComps));
          }
          Serial.println(F("Current player data requested by host"));
        }
        else
        {        
          if (cid != "0" && cid != "")
          {
            String playerData = readPlayerDataFromSD(cid, true);
    
            for (int i = 0; i < 7; i++) {
              client.print(getValue(playerData, '|', i) + "|");
            }
            Serial.print(F("Card Data Request: ")); Serial.println(cid);
          }
          else
          {
            client.print(F("||||||"));
          }
        }

        client.stop();
        return;
      }

      if (command == "gd")  // Request game data
      {
        int clockSync = urlDecode(getValue(getValue(querystring, '&', 0), '=', 1)).toInt();
        
        client.print(textHeader);
        
        if (clockSync == 1) client.print(gameName + "|" + now() + "||||||");
        else if (readGameData())
        {
          client.print(gameName + "|" + now() +  "|" + String(Credits) + "|" + String(totalIn) + "|" + String(totalWon) + "|" + String(totalGames) + "|" + String(gamesWon) + "|" + String(gamesLost) + "|" + versionString);
        }
        else
        {
          client.print(gameName + "|" + now() + "|||||||" + versionString);
        }        

        client.stop();
        Serial.println(F("Game data requested by host"));
        return;
      }

      if (command == "et") // End tournament and return score
      {
        exitTournamentMode();
        client.print(textHeader);
        client.print(cardHolder + "|" + String(tournamentScore));
        client.stop();
        Serial.println(F("Sent tournament data to host"));
        return;
      }

      if (command == "rb")
      {
        // Return status/result to host     
        client.print(textHeader); 
        client.print(F("OK"));
        client.stop();
        
        showMessageOnVFD("REBOOTING", 0);
        delay(2000);
        resetFunc();
        return;
      }

      // All of the remaining commands return a single status or a result code
      
      if (command == "pu")  // Upload player card data to server
      {
        String cid = getValue(getValue(querystring, '&', 1), '=', 1);
        String playerData = getValue(getValue(querystring, '&', 2), '=', 1);

        reqResult = writePlayerDataToSD(cid, getValue(playerData, '|', 1).toInt(), urlDecode(getValue(playerData, '|', 0)), getValue(playerData, '|', 2).toInt(), getValue(playerData, '|', 3).toInt(), getValue(playerData, '|', 4).toInt(), getValue(playerData, '|', 5).toInt(), getValue(playerData, '|', 6).toInt());
      }

      if (command == "mo") reqResult=slotCommand(MUTE,4,"Sound Off");
      if (command == "mt") reqResult=slotCommand(UMUTE,4,"Sound On");
      if (command == "lk") reqResult=slotCommand(LOCK,4,"Game Locked");
      if (command == "uk") reqResult=slotCommand(ULOCK,4,"Game Unlocked");
      if (command == "eb") reqResult=slotCommand(EBILL,4,"BV Enabled");
      if (command == "db") reqResult=slotCommand(DBILL,4,"BV Disabled");
      if (command == "rh") reqResult=resetHandpay();
      if (command == "ec") reqResult=changeButtonToCredits(true);
      if (command == "dc") reqResult=changeButtonToCredits(false);
      
      if (command == "pn") // Rename player card
      {
        if (cardType == 1)
        {
          cardHolder=urlDecode(getValue(getValue(querystring, '&', 1), '=', 1));
          reqResult = true;
          showMessageOnVFD("Card Renamed", 0);
          delay(2000);
        }
        else
        {
          reqResult = false;
          showMessageOnVFD("No Card", 0);
          delay(2000);
        }
      }

      if (command == "pc")  // Set comps to card
      {        
        if (cardType == 1)
        {
        float comps = getValue(getValue(querystring, '&', 1), '=', 1).toInt();
        playerComps = comps;
        reqResult = true;

        // Call this now to write back the changed playerComps; the other values will be updated when the card is removed
        if (localStorage) writePlayerDataToSD(lastCardID, cardType, cardHolder, playerTotalGames, playerGamesWon, playerGamesLost, playerTotalWon, playerComps);
        else writePlayerDataToServer(lastCardID, cardType, cardHolder, playerTotalGames, playerGamesWon, playerGamesLost, playerTotalWon, playerComps);
    
        showMessageOnVFD("Comps Applied", 0);
        delay(2000);
        }
        else
        {
          reqResult = false;
          showMessageOnVFD("No Card", 0);
          delay(2000);
        }
      }
      
      if (command == "ud") // Update Ticket Data
      {
        String location = urlDecode(getValue(getValue(querystring, '&', 0), '=', 1));
        String address1 = urlDecode(getValue(getValue(querystring, '&', 1), '=', 1));
        String address2 = urlDecode(getValue(getValue(querystring, '&', 2), '=', 1));

        reqResult=SetTicketData(location, address1, address2);
        showMessageOnVFD("Ticket Updated", 0);
        delay(2000);
      }
 
      if (command == "tr")  // Tournament Results
      {
        String customMsg = "                    " + getValue(getValue(urlDecode(querystring), '&', 1), '=', 1);
        customMsg.toCharArray(scrollBuffer, customMsg.length() + 1);
        reqResult=setupPlayerMessage(true);
        reqResult = slotCommand(ULOCK,4,"Game Unlocked");
      }

      if (command == "st")  // Start Tournament Mode
      {
        String credits = urlDecode(getValue(getValue(querystring, '&', 1), '=', 1));
        startTime = urlDecode(getValue(getValue(querystring, '&', 2), '=', 1)).toInt();
        endTime = urlDecode(getValue(getValue(querystring, '&', 3), '=', 1)).toInt();

        reqResult=startTournamentMode(credits);        
      }
      
      if (command == "cl")  // Set Arduino Clock
      {
        unsigned long pctime = getValue(getValue(querystring, '&', 0), '=', 1).toInt();
        processSyncMessage(pctime);
        reqResult=true;
      }
      
      if (command == "dt")  // Set DateTime on Game
      {
        String datetime = getValue(getValue(querystring, '&', 0), '=', 1);
        if (datetime.length()==14) reqResult = SetDateTime(datetime);
      }
      
      if (command == "cr")  // Add Credits to game
      {
        String credits = getValue(getValue(querystring, '&', 1), '=', 1);
        reqResult=addCredits(credits);
      }

      if (command == "cm")  // Update Scrolling Message
      {
        String customMsg = "                    " + getValue(getValue(urlDecode(querystring), '&', 1), '=', 1);
        String msgID = getValue(getValue(urlDecode(querystring), '&', 0), '=', 1);
        
        if (msgID == "1") {
          customMsg.toCharArray(scrollBuffer, customMsg.length() + 1);
          reqResult=setupPlayerMessage(true);
        }
        else {
          customMsg.toCharArray(attractMessage, customMsg.length() + 1);
          reqResult=setupAttractMessage();
        }
        Serial.print(F("Scrolling message updated: ")); Serial.println(customMsg);
      }

      // Return status/result to client     
      client.print(textHeader); 
      if (reqResult) client.print(F("OK"));
      else client.print(F("ERROR"));
      client.stop();
      if (!inTournament && !isLocked) showMessageOnVFD(casinoName, 0);
      return;
    }
    else if (url.equals("/"))
    {
      // Show web interface

      sdFile = SD.open("index.htm", O_READ);
      if (sdFile)
      {
        client.print(htmlHeader);
        client.print(F("<head><meta name='viewport' content='initial-scale=1.0'><title>Arduino TITO and Player Tracking</title>"));
        client.print(F("<style>body {font-family: Tahoma;} button {font-family: inherit; font-size: 1.0em; background-color: #008CBA; color: white; border: none; text-decoration: none; border-radius: 4px; transition-duration: 0.4s;} button:hover { background-color: white; color: black; border: 2px solid #008CBA; } td { margin-left: 7.5px; margin-right: 7.5px; } </style></head><body>"));
        client.print(F("<div style='max-width: 100%; margin: auto; text-align:center;'>"));
        client.print(F("<h2>Arduino TITO and Player Tracking</h2>"));
        client.print("Game Name: <b>" + gameName + "</b>&nbsp;&nbsp;&nbsp;");
        client.print("Current player: <b> " + cardHolder + "</b><br>");
        client.print("IP Address: <b>" + ipStr + "</b>&nbsp;&nbsp;&nbsp;");
        client.print("Version: <b> " + versionString + "</b></div>");

        while (sdFile.available()) {
          client.print(sdFile.readStringUntil('\n'));
        }

        sdFile.close();
        client.print(F("</div></body>"));
        client.print(htmlFooter);
        Serial.println(F("Web Interface Loaded"));
      }
      else
      {
        // UI Not available (SD Card error?)

        client.print(htmlHeader);
        client.print(F("<head><meta name='viewport' content='initial-scale=1.0'><style>body {font-family: Tahoma;}</style></head><body>"));
        client.print(F("<div style='max-width: 100%; margin: auto; text-align:center;'>"));
        client.print(F("<h2>Arduino TITO and Player Tracking</h2>"));
        client.print("Game Name: <b>" + gameName + "</b>&nbsp;&nbsp;&nbsp;");
        client.print("Current player: <b> " + cardHolder + "</b><br>");
        client.print("IP Address: <b>" + ipStr + "</b>&nbsp;&nbsp;&nbsp;");
        client.print("Version: <b> " + versionString + "</b><br><hr>");
        client.print(F("Unable to load User Interface!<br>This device can still be controlled remotely with the Game Manager Windows app or the BETTORSlots Android/IOS apps.<br>"));
        client.print(F("</div></body>"));
        client.print(htmlFooter);
        Serial.println(F("Web Interface failed to load"));
      }
    }
    else
    {
      // Everything else is a 404
      client.print(F("HTTP/1.1 404 Not Found\r\n\r\n"));
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
  byte eventCode = 0;

  UCSR0B = 0b10011101;
  Serial.write(0x80);
  delay(20);
  Serial.write(0x81);
  UCSR0B = 0b10011100;

  delay(10);  // Wait for data on the serial bus
  if (Serial.available() > 0) {
    eventCode = Serial.read();
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
    Serial.println(F("Unable to read data - timeout"));
    memset(ret, 0, sz);
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
      showMessageOnVFD("GAME TIMEOUT", 0); 
      Serial.println(F("Timeout waiting for ACK"));
      delay(2000);
      return false;
  }
  
  Serial.println(msg);
  if (!inTournament) {
    showMessageOnVFD(msg, 0);
    delay(2000);
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
    showMessageOnVFD("C-to-C Enabled", 0);
  }
  else
  {
    changeToCredits = false;
    Serial.println(F("Change to Credits Disabled"));
    showMessageOnVFD("C-to-C Disabled", 0);
  }
  return true;
}

bool resetHandpay()
{
  SendTypeS(RHP, sizeof(RHP));
  return waitForACK(SASAdr,"Handpay reset");
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
