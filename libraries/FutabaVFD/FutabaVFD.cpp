/*
  FutabaVFD Library - Implementation

  Originally created Oct 19 2022 by Marc Davis

  This is NOT a complete implementation of the VFD functionality; just enough to get it working for the project.
*/

#include "FutabaVFD.h"
#include "Arduino.h"

FutabaVFD::FutabaVFD(uint8_t resetPin,
                                 uint8_t writeStrobePin,
                                 uint8_t data0Pin, uint8_t data1Pin, uint8_t data2Pin, uint8_t data3Pin,
                                 uint8_t data4Pin, uint8_t data5Pin, uint8_t data6Pin, uint8_t data7Pin)
{
  // Copy pin values.
  _resetPin = resetPin;
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
  pinMode(resetPin, OUTPUT);
  digitalWrite(resetPin, HIGH);
  delayMicroseconds(500);
  digitalWrite(resetPin, LOW);

  pinMode(writeStrobePin, OUTPUT);
  digitalWrite(writeStrobePin, LOW);
  
  for (int i=0; i<NUM_DATA_PINS; i++)
  {
    uint8_t pin = _dataPins[i];
    
    pinMode(pin, INPUT);
    digitalWrite(pin, LOW);
  }
}

void FutabaVFD::begin(uint8_t numColumns, int numRows)
{
  _numDisplayColumns = numColumns;
  _numDisplayRows = numRows;
  _numDisplayCharacters = numColumns * numRows;

  // Make sure hardware has time to power up.
  delay(RESET_TIME_IN_MS);
  
  // Soft reset the display to get back to default state.
  writeInternal(RESET_CHARACTER);
  delayMicroseconds(SOFT_RESET_TIME_IN_US);
  writeInternal(END_OF_LINE_NO_ADVANCE_CHARACTER);

  // Default state is to have cursor on so we turn it off.
  noBlink();
}

void FutabaVFD::clear()
{
  writeInternal(DISPLAY_CLEAR_CHARACTER); 
}

void FutabaVFD::home()
{
  writeInternal(CURSOR_HOME_CHARACTER);  
}
  
void FutabaVFD::noBlink()
{
  writeInternal(BLINK_CURSOR_OFF_CHARACTER);
}

void FutabaVFD::blink()
{
  writeInternal(BLINK_CURSOR_ON_CHARACTER);
}

void FutabaVFD::noCursor()
{
  writeInternal(BLINK_CURSOR_OFF_CHARACTER);
}

void FutabaVFD::cursor()
{
  writeInternal(BLINK_CURSOR_ON_CHARACTER);
}

void FutabaVFD::setCursor(uint8_t column, uint8_t row)
{
  uint8_t address = row * _numDisplayColumns + column;
  if (address >= _numDisplayCharacters)
  {
    address = 0;
  }
  writeInternal(CURSOR_SET_CHARACTER);
  writeInternal(address);
}

void FutabaVFD::createChar(uint8_t index, uint8_t pixels[])
{
	// Not implemented
}

/*virtual*/ size_t FutabaVFD::write(uint8_t character)
{
  writeInternal(character); 
  return 1;
}

void FutabaVFD::writeInternal(uint8_t character)
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
   delayMicroseconds(300); 
   digitalWrite(_writeStrobePin, LOW);
}

