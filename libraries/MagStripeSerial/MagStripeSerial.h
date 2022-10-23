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
 *
 */


#ifndef MagStripeSerial_H
#define MagStripeSerial_H


#if ARDUINO < 100
#  include <WProgram.h>
#else
#  include <Arduino.h>
#endif


#define MagStripeSerial_CLS 2  /* card present pin */
#define MagStripeSerial_CLD 2  /* card fully inserted pin - used here for backwards compatibility with MagStripe */


class MagStripeSerial {
    public:
        // The CLS pin can be changed from the default...
        MagStripeSerial(unsigned char cls=MagStripeSerial_CLS, unsigned char cld=MagStripeSerial_CLD);

        // Initialize the library 
        void begin(unsigned char track);

        // Deinitialize the library 
        void stop();

        // Flush Buffer
        void flush();

        // Check if there is a card present for reading...
        bool available();

        // Check if there is a card fully inserted for reading... for backwards compatibility only
        bool available2();

        // Read the data from the card as ASCII...
        short read(char *data, unsigned char size);

    private:
        unsigned char pin_cls;
        unsigned char pin_cld;
        unsigned char track;
};


#endif  /* MagStripeSerial_H */
