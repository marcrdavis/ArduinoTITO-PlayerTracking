/*
  FutabaVFD Library - Declaration file
  
  Originally created Oct 19 2022 by Marc Davis
  
  This is NOT a complete implementation of the VFD functionality; just enough to get it working for the project.
*/

#ifndef FutabaVFD_h
#define FutabaVFD_h
#define __PROG_TYPES_COMPAT__
#include  <avr/pgmspace.h>

#include <inttypes.h>
#include <avr/pgmspace.h> // Allows storing of data in program memory, used for charater bitmaps.
#include "Print.h"
#include "Arduino.h"

#define prog_uint8_t uint8_t PROGMEM

class FutabaVFD : public Print {
public:

  // Special control characters supported by Futaba
  static const uint8_t BACKSPACE_CHARACTER = 0x08; 
  static const uint8_t ADVANCE_CURSOR__CHARACTER = 0x09; // Tab forward 1 character.
  static const uint8_t LINE_FEED_CHARACTER = 0x0A;
  static const uint8_t START_BLINK_CHARACTER = 0x0B;
  static const uint8_t END_BLINK_CHARACTER = 0x0C;
  static const uint8_t CARRIAGE_RETURN_CHARACTER = 0x0D;
  static const uint8_t BLINK_CURSOR_OFF_CHARACTER = 0x0E; 
  static const uint8_t BLINK_CURSOR_ON_CHARACTER = 0x0F; 

  static const uint8_t END_OF_LINE_WRAP_CHARACTER = 0x11;
  static const uint8_t END_OF_LINE_NO_ADVANCE_CHARACTER = 0x12; // Cursor stops on the last position at end of display.
  static const uint8_t END_OF_LINE_HORIZONTAL_SCROLL_CHARACTER = 0x13;

  static const uint8_t RESET_CHARACTER = 0x14;
  static const uint8_t DISPLAY_CLEAR_CHARACTER = 0x15;
  static const uint8_t CURSOR_HOME_CHARACTER = 0x16;
  static const uint8_t BEGIN_USER_DEFINED_CHAR_CHARACTER = 0x18;
  static const uint8_t BIT_7_HIGH_FOR_NEXT_BYTE_CHARACTER = 0x19;

  static const uint8_t CURSOR_SET_CHARACTER = 0x1B; 
 
  static const uint8_t FIRST_CUSTOM_CHAR_OFFSET = 0xF6;
  
  FutabaVFD(uint8_t resetPin,
                  uint8_t writeStrobePin,
                  uint8_t data0Pin, uint8_t data1Pin, uint8_t data2Pin, uint8_t data3Pin,
                  uint8_t data4Pin, uint8_t data5Pin, uint8_t data6Pin, uint8_t data7Pin);
             
  // Sets the character dimensions of the display.
  void begin(uint8_t numColumns, int numRows);

  void clear();
  void home();
  void noBlink();
  void blink();
  void noCursor(); 
  void cursor();
  
  // Methods supported by LiquidCrystal but not by our library.
//  void noDisplay();
//  void display();
//  void scrollDisplayLeft();
//  void scrollDisplayRight();
//  void leftToRight();
//  void rightToLeft();
//  void autoscroll();
//  void noAutoscroll();
//  void command(uint8_t);

  void setCursor(uint8_t column, uint8_t row); 
  void createChar(uint8_t, uint8_t[]);
  virtual size_t write(uint8_t character);

private:

  static const uint8_t NUM_DATA_PINS = 8;
  
  static const uint16_t RESET_TIME_IN_MS = 500;
  static const uint16_t SOFT_RESET_TIME_IN_US = 710;

  static const uint8_t CHARACTER_WIDTH = 5;
  static const uint8_t CHARACTER_HEIGHT_ACTUAL = 7;
  
  uint8_t _resetPin;
  uint8_t _writeStrobePin;
  uint8_t _dataPins[NUM_DATA_PINS];
  
  uint8_t _numDisplayColumns;
  uint8_t _numDisplayRows;
  uint8_t _numDisplayCharacters;

  void writeInternal(uint8_t character);
};
  
#endif // FutabaVFD_h

