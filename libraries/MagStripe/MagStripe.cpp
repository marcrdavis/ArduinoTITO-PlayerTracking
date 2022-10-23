/*
 * MagStripe - Read data from a magnetic stripe card.
 *
 * Copyright (c) 2010 Carlos Rodrigues <cefrodrigues@gmail.com>
 * Modified by Marc Davis - March 12, 2021
 * This version only supports insert-type card readers
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


#include "MagStripe.h"

#if ARDUINO < 100
#  include <WProgram.h>
#else
#  include <Arduino.h>
#endif


// Enough bits to read any of the three tracks...
#define BIT_BUFFER_LEN 800 


// Variables used by the interrupt handlers...
static volatile bool next_bit = 0;                       // next bit to read
static volatile unsigned char bits[BIT_BUFFER_LEN / 8];  // buffer for bits being read
static volatile short num_bits = 0;                      // number of bits already read


// Manipulate the bit buffer...
static void bits_set(short index, bool bit);
static bool bits_get(short index);

// The interrupt handlers...
static void handle_data(void);
static void handle_clock(void);


MagStripe::MagStripe(unsigned char cls, unsigned char cld):
    pin_cls(cls),
    pin_cld(cld),
    direction(READ_UNKNOWN)
{}


void MagStripe::begin(unsigned char track)
{
    this->track = track;

    pinMode(MAGSTRIPE_RDT, INPUT);
    pinMode(MAGSTRIPE_RCL, INPUT);
    pinMode(this->pin_cls, INPUT);
    pinMode(this->pin_cld, INPUT);

    // Reading is more reliable when using interrupts...
    attachInterrupt(digitalPinToInterrupt(MAGSTRIPE_RDT), handle_data, CHANGE);    // data pin
    attachInterrupt(digitalPinToInterrupt(MAGSTRIPE_RCL), handle_clock, FALLING);  // clock pin
}


void MagStripe::stop()
{
    detachInterrupt(digitalPinToInterrupt(MAGSTRIPE_RDT));
    detachInterrupt(digitalPinToInterrupt(MAGSTRIPE_RCL));
}


void MagStripe::flush()
{
	// Do nothing for now
}

bool MagStripe::available()
{
    return digitalRead(this->pin_cls) == LOW;
}

bool MagStripe::available2()
{
    return digitalRead(this->pin_cld) == LOW;
}


short MagStripe::read(char *data, unsigned char size)
{
    // Empty the bit buffer...
    num_bits = 0;
    next_bit = 0;
    
    unsigned long currentMillis = millis();

    // Wait while the data is being read by the interrupt routines...
    while (!this->available2() && (millis() - currentMillis < 900) ) { }

    // Decode the raw bits...
    short chars = this->decode_bits(data, size);
    this->direction = READ_FORWARD;

    // If the data looks bad, reverse and try again...
    if (chars < 0) {
        this->reverse_bits();
        chars = this->decode_bits(data, size);
        this->direction = READ_BACKWARD;
     }

    // The card could not be read successfully...
    if (chars < 0) {
        this->direction = READ_UNKNOWN;
    }

    return chars;
}


enum ReadDirection MagStripe::read_direction()
{
    return this->direction;
}


void MagStripe::reverse_bits()
{
    for (short i = 0; i < num_bits / 2; i++) {
        bool b = bits_get(i);

        bits_set(i, bits_get(num_bits - i - 1));
        bits_set(num_bits - i - 1, b);
    }
}


bool MagStripe::verify_parity(unsigned char c)
{
    unsigned char parity = 0;

    for (unsigned char i = 0; i < 8; i++) {
        parity += (c >> i) & 1;
    }

    // The parity must be odd...
    return parity % 2 != 0;
}


bool MagStripe::verify_lrc(short start, short length)
{
    unsigned char parity_bit = (this->track == 1 ? 7 : 5);

    // Count the number of ones per column (ignoring parity bits)...
    for (short i = 0; i < (parity_bit - 1); i++) {
        short parity = 0;

        for (short j = i; j < length; j += parity_bit) {
            parity += bits_get(start + j);
        }

        // Even parity is what we want...
        if (parity % 2 != 0) {
            return false;
        }
    }

    return true;
}


short MagStripe::find_sentinel(unsigned char pattern)
{
    unsigned char bit_accum = 0;
    unsigned char bit_length = (this->track == 1 ? 7 : 5);

    for (short i = 0; i < num_bits; i++) {
        bit_accum >>= 1;                               // rotate the bits to the right...
        bit_accum |= bits_get(i) << (bit_length - 1);  // ...and add the current bit
        // Stop when the start sentinel pattern is found...
        if (bit_accum == pattern) {
            return i - (bit_length -1);
        }
    }

    // No start sentinel was found...
    return -1;
}


short MagStripe::decode_bits(char *data, unsigned char size) {
    short bit_count = 0;
    unsigned char chars = 0;
    unsigned char bit_accum = 0;
    unsigned char bit_length = (this->track == 1 ? 7 : 5);
    
    short bit_start = this->find_sentinel(this->track == 1 ? 0x45 : 0x0b);
    if (bit_start < 0) {  // error, start sentinel not found
        return -1;
    }

    for (short i = bit_start; i < num_bits; i++) {
        bit_accum >>= 1;                                 // rotate the bits to the right...
        bit_accum |= (bits_get(i) << (bit_length - 1));  // ...and add the current bit
    
        bit_count++;
    
        if (bit_count % bit_length == 0) {
            if (chars >= size) {  // error, the buffer is too small
                return -1;
            }

            // A null means we reached the end of the data...
            if (bit_accum == 0) {
                break;
            }
           
            // The parity must be odd... 
            if (!verify_parity(bit_accum)) {
                return -1;
            }

            // Remove the parity bit...
            bit_accum &= ~(1 << (bit_length - 1));

            // Convert the character to ASCII...
            data[chars] = bit_accum + (this->track == 1 ? 0x20 : 0x30);
            chars++;
     
            // Reset...
            bit_accum = 0;
        }
    }
    
    // Turn the data into a null-terminated string...
    data[chars] = '\0';
  
    if (data[chars - 2] != '?') {  // error, the end sentinel is not in the right place
        return -1;
    }

    // Verify the LRC (even parity across columns)...
    if (!verify_lrc(bit_start, chars * bit_length)) {
        return -1;
    }

    // Cleanup returned value
    data[chars - 2] = 0;
    strcpy(data,data + 1);
    chars -= 2;

   char tmp[chars];
   strcpy(tmp, data);

   for (int i = 0; i < chars; i++) {
     if (data[i] == '0') strcpy(tmp, data + i+1);
     else break;
   }

   for (int i = sizeof(tmp)-1; i >= 0; i--) {
     if (tmp[i] == '0' | tmp[i] == 0) tmp[i] = 0;
     else break;
   }
   
   strcpy(data, tmp);
   //strcpy(tmp, data);
   if (sizeof(data) > 7) strcpy(data, tmp + sizeof(tmp)-7);   

   return sizeof(data);
}


static void bits_set(short index, bool bit)
{
    volatile unsigned char *b = &bits[index / 8];
    unsigned char m = 1 << (index % 8);

    *b = bit ? (*b | m) : (*b & ~m);
}


static bool bits_get(short index)
{
    return bits[index / 8] & (1 << (index % 8));
}


static void handle_data()
{
    next_bit = !next_bit;
}
 

static void handle_clock()
{
    // Avoid a crash in case there are too many bits (garbage)...
    if (num_bits >= BIT_BUFFER_LEN) {
        return;
    }

    bits_set(num_bits, next_bit);
    num_bits++;
}


/* vim: set expandtab ts=4 sw=4: */
