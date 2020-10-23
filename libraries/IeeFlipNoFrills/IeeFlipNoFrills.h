/*
  IeeFlipNoFrills Library - Declaration file
  
  Originally created 5 March 2011
  Ported to IeeFlipNoFrills on April 10, 2011.  
  Patched for 1.8.12 by Marc Davis on Oct 15, 2020.

  This file is in the public domain.  
*/

#ifndef IeeFlipNoFrills_h
#define IeeFlipNoFrills_h
#define __PROG_TYPES_COMPAT__
#include  <avr/pgmspace.h>

#include <inttypes.h>
#include <avr/pgmspace.h> // Allows storing of data in program memory, used for charater bitmaps.
#include "Print.h"
#include "Arduino.h"

#define prog_uint8_t uint8_t PROGMEM

class IeeFlipNoFrills : public Print {
public:

  // Special control characters supported by IEE-Flip no frills displays. These differ from other IEE Flip models.
  static const uint8_t BACKSPACE_CHARACTER = 0x08; 
  static const uint8_t ADVANCE_CURSOR__CHARACTER = 0x09; // Tab forward 1 character.
  static const uint8_t LINE_FEED_CHARACTER = 0x0A;
  static const uint8_t CARRIAGE_RETURN_CHARACTER = 0x0D;
  static const uint8_t BLINK_CURSOR_OFF_CHARACTER = 0x0E; 
  static const uint8_t BLINK_CURSOR_ON_CHARACTER = 0x0F; 

  static const uint8_t END_OF_LINE_WRAP_CHARATCER = 0x11;
  static const uint8_t END_OF_LINE_NO_ADVANCE_CHARATCER = 0x12; // Cursor stops on the last position at end of display.
  static const uint8_t END_OF_LINE_HORIZONTAL_SCROLL_CHARATCER = 0x13;

  static const uint8_t RESET_CHARACTER = 0x14;
  static const uint8_t DISPLAY_CLEAR_CHARACTER = 0x15;
  static const uint8_t CURSOR_HOME_CHARACTER = 0x16;
  static const uint8_t BEGIN_USER_DEFINED_CHAR_CHARACTER = 0x18;
  static const uint8_t BIT_7_HIGH_FOR_NEXT_BYTE_CHARACTER = 0x19;

  static const uint8_t CURSOR_SET_CHARACTER = 0x1B; 

  static const uint8_t BRIGHTNESS_12_PERCENT_CHARACTER = 0x1C;
  static const uint8_t BRIGHTNESS_25_PERCENT_CHARACTER = 0x1D;
  static const uint8_t BRIGHTNESS_50_PERCENT_CHARACTER = 0x1E;
  static const uint8_t BRIGHTNESS_100_PERCENT_CHARACTER = 0x1F;
  
  static const uint8_t FIRST_CUSTOM_CHAR_OFFSET = 0xF6;
  
  // This library supports multiple displays. In this case, all pins
  // can be shared except busy and write strobe. This library does not support
  // sharing pins with other devices. Arduino pin 13 (the one connected
  // to the on board LED) can be used for data lines but it can not
  // be used for any of the control lines.
  // TODO: Test multiple displays.
  IeeFlipNoFrills(uint8_t busyPin,
                  uint8_t writeStrobePin,
                  uint8_t data0Pin, uint8_t data1Pin, uint8_t data2Pin, uint8_t data3Pin,
                  uint8_t data4Pin, uint8_t data5Pin, uint8_t data6Pin, uint8_t data7Pin);
             
  // Sets the character dimensions of the display.
  void begin(uint8_t numColumns, int numRows);

  void clear();
  void home();

  void noBlink();
  void blink();
  void noCursor(); // This is the same as blink on this display.
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
  static const uint16_t WAIT_FOR_NOT_BUSY_TIMEOUT_IN_US = 2000;

  static const uint8_t CHARACTER_WIDTH = 5;
  static const uint8_t CHARACTER_HEIGHT_ACTUAL = 7;
  static const uint8_t CHARACTER_HEIGHT_PRETEND = 8;
  static const prog_uint8_t sCustomCharacterBitMapping[CHARACTER_WIDTH*CHARACTER_HEIGHT_ACTUAL] PROGMEM;
  
  uint8_t _busyPin;
  uint8_t _writeStrobePin;
  uint8_t _dataPins[NUM_DATA_PINS];
  
  uint8_t _numDisplayColumns;
  uint8_t _numDisplayRows;
  uint8_t _numDisplayCharacters;

  void writeInternal(uint8_t character);
  bool waitForNotBusy();
};
  
#endif // IeeFlipNoFrills_h

