/*
  IeeFlipNoFrills Library - Implementation

  Originally created 5 March 2011
  Ported to IeeFlipNoFrills April 10 2011
  Patched for 1.8.12 by Marc Davis

  This file is in the public domain.  
*/

#include "IeeFlipNoFrills.h"
#include "Arduino.h"

/*static*/ const prog_uint8_t IeeFlipNoFrills::sCustomCharacterBitMapping[CHARACTER_WIDTH*CHARACTER_HEIGHT_ACTUAL] PROGMEM =
{
  // Source bit indices go high bit to low bit, top byte to bottom byte. See S03601-95B-40 datasheet.
  // High nibble is byte, low nibble is bit.
  ((7-3)<<4) | 5, // 1
  ((7-3)<<4) | 1, // 2
  ((3-3)<<4) | 6, // 3
  ((4-3)<<4) | 2, // 4
  ((6-3)<<4) | 5, // 5
  
  ((6-3)<<4) | 1, // 6
  ((4-3)<<4) | 6, // 7
  ((5-3)<<4) | 2, // 8
  ((5-3)<<4) | 5, // 9
  ((5-3)<<4) | 1, // 10
  
  ((5-3)<<4) | 6, // 11
  ((6-3)<<4) | 2, // 12
  ((4-3)<<4) | 5, // 13
  ((4-3)<<4) | 1, // 14
  ((6-3)<<4) | 6, // 15
  
  ((7-3)<<4) | 2, // 16
  ((3-3)<<4) | 5, // 17
  ((7-3)<<4) | 4, // 18
  ((7-3)<<4) | 6, // 19
  ((8-3)<<4) | 0, // 20

  ((7-3)<<4) | 0, // 21
  ((6-3)<<4) | 4, // 22
  ((4-3)<<4) | 3, // 23
  ((8-3)<<4) | 1, // 24
  ((6-3)<<4) | 0, // 25
  
  ((5-3)<<4) | 4, // 26
  ((5-3)<<4) | 3, // 27
  ((8-3)<<4) | 2, // 28
  ((5-3)<<4) | 0, // 29
  ((4-3)<<4) | 4, // 30
  
  ((6-3)<<4) | 3, // 31
  ((8-3)<<4) | 3, // 32
  ((4-3)<<4) | 0, // 33
  ((3-3)<<4) | 4, // 34
  ((7-3)<<4) | 3, // 35
};

IeeFlipNoFrills::IeeFlipNoFrills(uint8_t busyPin,
                                 uint8_t writeStrobePin,
                                 uint8_t data0Pin, uint8_t data1Pin, uint8_t data2Pin, uint8_t data3Pin,
                                 uint8_t data4Pin, uint8_t data5Pin, uint8_t data6Pin, uint8_t data7Pin)
{
  // Copy pin values.
  _busyPin = busyPin;
  _writeStrobePin = writeStrobePin;
  
  _dataPins[0] = data0Pin;
  _dataPins[1] = data1Pin;
  _dataPins[2] = data2Pin;
  _dataPins[3] = data3Pin;
  _dataPins[4] = data4Pin;
  _dataPins[5] = data5Pin;
  _dataPins[6] = data6Pin;
  _dataPins[7] = data7Pin;

  // Init screen character dimensions to something harmless.
  _numDisplayColumns = 1;
  _numDisplayRows = 1;
  _numDisplayCharacters = _numDisplayColumns * _numDisplayRows;

  // Initialize pin states. Everything to input, no pullup register.
  pinMode(busyPin, INPUT);
  digitalWrite(busyPin, LOW);

  digitalWrite(writeStrobePin, LOW);
  pinMode(writeStrobePin, OUTPUT);

  for (int i=0; i<NUM_DATA_PINS; i++)
  {
    uint8_t pin = _dataPins[i];
    
    pinMode(pin, INPUT);
    digitalWrite(pin, LOW);
  }
}

void IeeFlipNoFrills::begin(uint8_t numColumns, int numRows)
{
  _numDisplayColumns = numColumns;
  _numDisplayRows = numRows;
  _numDisplayCharacters = numColumns * numRows;

  // Make sure hardware has time to power up.
  delay(RESET_TIME_IN_MS);
  
  // Soft reset the display to get back to default state.
  writeInternal(RESET_CHARACTER);
  delayMicroseconds(SOFT_RESET_TIME_IN_US);

  // Default state is to have cursor on so we turn it off.
  noBlink();
}

void IeeFlipNoFrills::clear()
{
  writeInternal(DISPLAY_CLEAR_CHARACTER);  
}

void IeeFlipNoFrills::home()
{
  writeInternal(CURSOR_HOME_CHARACTER);  
}
  
void IeeFlipNoFrills::noBlink()
{
  writeInternal(BLINK_CURSOR_OFF_CHARACTER);
}

void IeeFlipNoFrills::blink()
{
  writeInternal(BLINK_CURSOR_ON_CHARACTER);
}

void IeeFlipNoFrills::noCursor()
{
  writeInternal(BLINK_CURSOR_OFF_CHARACTER);
}

void IeeFlipNoFrills::cursor()
{
  writeInternal(BLINK_CURSOR_ON_CHARACTER);
}

void IeeFlipNoFrills::setCursor(uint8_t column, uint8_t row)
{
  uint8_t address = row * _numDisplayColumns + column;
  if (address >= _numDisplayCharacters)
  {
    address = 0;
  }
  
  writeInternal(CURSOR_SET_CHARACTER);
  writeInternal(address);
}

void IeeFlipNoFrills::createChar(uint8_t index, uint8_t pixels[])
{
  // This display has a weird bit layout for custom characters. See datasheet.
  static const uint8_t NUM_BYTES_CONVERTED = 6;
  uint8_t convertedBytes[NUM_BYTES_CONVERTED];
  
  // Init converted bytes.
  for (uint8_t i=0; i<NUM_BYTES_CONVERTED; i++)
  {
    convertedBytes[i] = 0;
  }
  
  // Convert data.
  for (uint8_t row=0, bitNum = 0; row < CHARACTER_HEIGHT_ACTUAL; row++)
  {
    for (uint8_t mask = 0b10000; mask; mask>>=1, bitNum++)
    {
      if (pixels[row] & mask)
      {
        uint8_t mapping = pgm_read_byte(sCustomCharacterBitMapping + bitNum);
        uint8_t destIndex = mapping >> 4;
        uint8_t destBit = mapping & 0xF;
        
        convertedBytes[destIndex] |= (1<<destBit);             
      } 
    }
  }
  
  // Write out complete custom character sequence now that we have converted data.
  writeInternal(BEGIN_USER_DEFINED_CHAR_CHARACTER);
  writeInternal(index);
  for (uint8_t i=0; i<NUM_BYTES_CONVERTED; i++)
  {
    writeInternal(convertedBytes[i]);
  }
}
  

/*virtual*/ size_t IeeFlipNoFrills::write(uint8_t character)
{
  // Remap from LCD custom character locations to Flip locations.
  if (character < 8)
  {
    character += FIRST_CUSTOM_CHAR_OFFSET;
  }
  
  writeInternal(character); 
  return 1;
}

void IeeFlipNoFrills::writeInternal(uint8_t character)
{
  if (waitForNotBusy())
  {
    // Place character onto data pins.
    uint8_t tmpCharacter = character;
    for (int i=0; i<NUM_DATA_PINS; i++)
    {
      uint8_t pin = _dataPins[i];
 
      // Set pin value.
      digitalWrite(pin, (tmpCharacter & 1) ? HIGH : LOW);
          
      // Enable pin output.
      pinMode(pin, OUTPUT);  

      tmpCharacter >>= 1;
    }  
    
    // Pull write strobe high then low.
    digitalWrite(_writeStrobePin, HIGH);
    delayMicroseconds(1); // Probably don't need this, delay needed is only 250ns and digitial write is very slow.
    digitalWrite(_writeStrobePin, LOW);
  }   
}

bool IeeFlipNoFrills::waitForNotBusy()
{
  bool success = true;
  
  unsigned long startWaitTime = micros();
  while (digitalRead(_busyPin)!=LOW)
  {
    if ((micros()-startWaitTime) > WAIT_FOR_NOT_BUSY_TIMEOUT_IN_US)
    {
      success = false;
      break;
    }
  }
  
  return success;
}

