/*
  IeeFlipNoFrills Library - Hello World
 
 Demonstrates the use an IEE Flip "No Frills" VFD. Tested
 with model 03601-95A-40 2x20 alphanumeric display.
 
 This sketch prints "Hello World!" to the VFD
 and shows the number of seconds since the sketch
 started.
 
  The circuit:
  
 * Display pins 1 (BUSY) connected to Arduino pin 2
 * Display pins 2 (WRITE STROBE) connected to Arduino pin 3
 * Display pins 3-10 (D7-D0) connected to Arduino pins 4-11
 * Display pin 11 (+5V @ 370mA) connected to +5V
 * Display pin 12 (GND) connected to GND
 * Display pins 13 & 14 unconnected
  
 Created Mar 6, 2011.
 Ported to IeeFlipNoFrills on April 10, 2011.
 
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
  // Print a message to the VFD.
  vfd.print("hello, world!");
}

void loop() {
  
  // Set the cursor to the first column (0) of the second row (1).
  vfd.setCursor(0, 1);
  // Print the number of seconds since reset.
  vfd.print(millis()/1000);
}

