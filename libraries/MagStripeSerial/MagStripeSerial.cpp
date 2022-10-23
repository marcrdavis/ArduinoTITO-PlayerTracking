/*
 * MagStripeSerial - Read data from a magnetic stripe card.
 *
 * Copyright (c) 2022 Marc Davis 
 * This version only supports insert-type card readers
 * Tested Devices: XS Technologies PI70-120-TLA-DFR
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


#include "MagStripeSerial.h"

#if ARDUINO < 100
#  include <WProgram.h>
#else
#  include <Arduino.h>
#endif


MagStripeSerial::MagStripeSerial(unsigned char cls, unsigned char cld):
    pin_cls(cls),
    pin_cld(cld)
{}


void MagStripeSerial::begin(unsigned char track)
{
    Serial2.begin(9600,SERIAL_7E2);  // Assumes Serial2 from the Arduino TITO Project MEGA Sketches
	pinMode(2, INPUT);    // sets the digital pin 2 as input for the card detect
    this->track = track;  // Not used - for backwards compatibility only
} 


void MagStripeSerial::stop()
{
	// Do nothing for now
}

void MagStripeSerial::flush()
{
	// Clear the serial buffer
	for (int i = 0; i < 10; i++) {
		while (Serial2.available() > 0) {
			Serial2.read();
		}
	}
}

bool MagStripeSerial::available()
{	
    return digitalRead(this->pin_cls) == LOW;
}


bool MagStripeSerial::available2()
{	
    return digitalRead(this->pin_cld) == LOW;
}


short MagStripeSerial::read(char *data, unsigned char size)
{
	String tmp;
	String empty;
	bool valid = false;
	
	if (Serial2.available()) {
		int length = Serial2.available();
		//for (int i = 0; i < 10; i++) {
			while (Serial2.available() > 0) {
				char c = Serial2.read();
				
				if (c == '\r') break;
				if (c == '#') continue;
				if (c == '!' && valid) break;
				if (c == '&' && valid) break;
				if (c == '&') valid=true;
				if (isAlphaNumeric(c)) tmp += (char)c;
			}
			
			length = tmp.length();
			if (length < 8) {
				strcpy(data,empty.c_str());
				return 0;
			}
			valid=true;
		//}
		if (valid) {
			strcpy(data,tmp.c_str());
			return length;
		}
	}

	strcpy(data,empty.c_str());
	return 0;

}


