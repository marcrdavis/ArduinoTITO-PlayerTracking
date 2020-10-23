/*
  IeeFlipNoFrills Library - Custom Character Unit Test
 
 Demonstrates the use an IEE Flip "No Frills" VFD. Tested
 with model 03601-95A-40 2x20 alphanumeric display.
 
 This sketch sets one pixel at a time in a custom character,
 going left to right, top to bottom.
 
  The circuit:
  
 * Display pins 1 (BUSY) connected to Arduino pin 2
 * Display pins 2 (WRITE STROBE) connected to Arduino pin 3
 * Display pins 3-10 (D7-D0) connected to Arduino pins 4-11
 * Display pin 11 (+5V @ 370mA) connected to +5V
 * Display pin 12 (GND) connected to GND
 * Display pins 13 & 14 unconnected
  
 Created April 15, 2011.
 
 This example code is in the public domain.

 http://arduino.cc/playground/Main/IeeFlipNoFrills
*/

// Include the library code.
#include <IeeFlipNoFrills.h>

// Initialize the library with the numbers of the interface pins
IeeFlipNoFrills vfd(2, 3, /* control pins */
                    11, 10, 9, 8, 7, 6, 5, 4 /* data pins */);

void setup() {
  // Start the library/displaying to the vfd 
  vfd.begin(20, 2);

  // Print description text
  vfd.setCursor(0, 0);
  vfd.print("Scan left to right");
  vfd.setCursor(0, 1);
  vfd.print("top to bottom: ");
}

void loop() {
  
  uint8_t buffer[8];
  
  for (uint8_t row = 0; row < 7; row++)
  {
    for (uint8_t bitMask = 0b10000; bitMask; bitMask >>= 1)
    {
      for (uint8_t i = 0; i<8; i++)
      {
        buffer[i] = 0;
      }
      
      buffer[row] = bitMask;
      
      vfd.setCursor(14, 1);
      vfd.write(' ');
      vfd.createChar(0, buffer);
      vfd.setCursor(14, 1);
      vfd.write(0);
      
      delay(500);
    }
  }
}  

