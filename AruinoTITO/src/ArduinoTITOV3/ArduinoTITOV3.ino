/*
  Arduino TITO v3.0.20240117T
  by Marc R. Davis - Copyright (c) 2020-2024 All Rights Reserved
  https://github.com/marcrdavis/ArduinoTITO-PlayerTracking

  Portions of the Arduino SAS protocol implementation by Ian Walker - Thank you!
  Additional testing and troubleshooting by NLG member Eddiiie - Thank you!

  Hardware requirements: 
  Arduino Uno; Serial Port

  For TITO setup please follow the included documentation

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
// Core Variables
// ------------------------------------------------------------------------------------------------------------

bool sasError = false;
bool changeToCredits = 0; // UPDATE BEFORE COMPILING - Set to 1 to enable Change to Credits
char* changeCredits = "500"; // UPDATE BEFORE COMPILING - Set the number of credits to add on each push of the change/service button

// ------------------------------------------------------------------------------------------------------------
// SAS Protocol Variables
// ------------------------------------------------------------------------------------------------------------

byte SASAdr = 0x01;
byte CRCH = 0x00;
byte CRCL = 0x00;

byte SVNS[2] = {SASAdr, 0x57};
byte TP[2] = {SASAdr, 0x70};
byte EVInfo[5] = {SASAdr, 0x4D, 0x00, 0x00, 0x00};
byte transComplete[6] = {SASAdr, 0x71, 0x01, 0xFF, 0x1F, 0xD0};

byte LBS [9];
byte SVN [13];
byte TPS [35];
byte HPS [24];
byte TEQ [21];
byte TRS [21];
byte COT [10];
byte COS [5];

// ------------------------------------------------------------------------------------------------------------
// Setup - called once during init
// ------------------------------------------------------------------------------------------------------------

void setup()
{
  // Setup SAS/TITO Communications
  Serial.begin(19200);
  Serial.setTimeout(200);
  pinMode(LED_BUILTIN, OUTPUT);
  
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

  // Check game
  generalPoll();
  
  // Flash the LED
  digitalWrite(LED_BUILTIN, LOW);
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

// ------------------------------------------------------------------------------------------------------------
// SAS Protocol
// NOTE: This does not implement the full protocol - just enough for TITO and System Bonusing
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

void waitForResponse(byte & waitfor, byte * ret, int sz)
{
  sasError = false;
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

void SystemValidation()
{
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

void CashOutState()
{
  SendTypeS(EVInfo, sizeof(EVInfo));
  waitForResponse(EVInfo[1], TPS, sizeof(TPS));
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
