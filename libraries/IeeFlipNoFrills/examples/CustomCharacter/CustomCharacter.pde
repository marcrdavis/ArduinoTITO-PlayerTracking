/*
  IeeFlipNoFrills Library - Custom Character
 
 Demonstrates the use an IEE Flip "No Frills" VFD. Tested
 with model 03601-95A-40 2x20 alphanumeric display.
 
 This sketch sets up alternate heart beat custom characters
 and displays them.
 
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
            
byte openHeart[8] = {
  0b00000,
  0b01010,
  0b10101,
  0b10001,
  0b01010,
  0b00100,
  0b00000, 
  0b00000
};

byte filledHeart[8] = {
  0b00000,
  0b01010,
  0b11111,
  0b11111,
  0b01110,
  0b00100,
  0b00000,
  0b00000
};

void setup() {
  // Start the library/displaying to the vfd 
  vfd.begin(20, 2);

  // Create open heart as custom character 0.
  vfd.createChar(0, openHeart);
  
  // Create filled heart as custom character 1.
  vfd.createChar(1, filledHeart);

  // Print description text
  vfd.setCursor(0, 0);
  vfd.print("Heartbeat made using");
  vfd.setCursor(0, 1);
  vfd.print("custom characters: ");
}

void loop() {
  
  // Display open heart.
  vfd.setCursor(19, 1);
  vfd.write((char)0);
  delay(750);

  // Display closed heart for shorter period.
  vfd.setCursor(19, 1);
  vfd.write((char)1);
  delay(250);
}

