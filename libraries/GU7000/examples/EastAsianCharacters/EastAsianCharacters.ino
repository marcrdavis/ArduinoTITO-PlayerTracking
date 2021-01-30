//
// THIS DEMO MAY ONLY BE USED WITH MODULES WITH MODEL
// NUMBERS ENDING IN 79xx.
// EXAMPLE
//    GU140X16G-7903  CAN RUN THIS DEMO
//    GU160X32D-7900  CAN RUN THIS DEMO
//    GU140X16G-7000  CANNOT RUN THIS DEMO
//

#include <GU7000_Interface.h>
#include <GU7000_Parallel.h>
#include <GU7000_Serial_Async.h>
#include <GU7000_Serial_SPI.h>
#include <GU7000_Serial_Sync.h>
#include <Noritake_VFD_GU7000.h>

// ****************************************************
// ****************************************************
// Uncomment one of the communication interfaces below.
//
//GU7000_Serial_Async interface(38400,3, 5, 7); // BAUD RATE,SIN,BUSY,RESET
//GU7000_Serial_Sync interface(3, 5, 6, 7); // SIN,BUSY,SCK,RESET
//GU7000_Serial_SPI interface(3, 5, 6, 7, 8); // SIN,BUSY,SCK,RESET,CS
//GU7000_Parallel interface('R', 8,9,10,11, 0,1,2,3,4,5,6,7); // Module Pin#3=RESET; BUSY,RESET,WR,RD,D0-D7
//GU7000_Parallel interface('B', 8,9,10,11, 0,1,2,3,4,5,6,7); // Module Pin#3=BUSY; BUSY,RESET,WR,RD,D0-D7
//GU7000_Parallel interface('N', 8,9,10,11, 0,1,2,3,4,5,6,7); // Module Pin#3=nothing; BUSY,RESET,WR,RD,D0-D7

//
// ****************************************************
// ****************************************************
Noritake_VFD_GU7000 vfd;

void setup() {
  _delay_ms(500);           // wait for device to power up
  vfd.begin(140, 16);       // 140x16 module
  // Enter the 4-digit model class number
  // E.g. 7040 for GU140X16G-7040A
  vfd.interface(interface); // select which interface to use
  vfd.isModelClass(7900);
  //vfd.isGeneration('B');    // Uncomment this for B generation
  vfd.GU7000_reset();       // reset module
  vfd.GU7000_init();        // initialize module
  
  for (int i = 0; ; i++) {
    vfd.GU7000_clearScreen();
    if (i % 4 == 0) {
      vfd.print("Japanese:");
      vfd.GU7000_setCursor(vfd.width - 16*3, 0);
      vfd.GU7000_setFontSize(1, 1, true); // Select 8x16 font
      vfd.GU7000_useMultibyteChars(true);  // Turn on multi-byte characters
      vfd.GU7000_setMultibyteCharset(0); // Set characterset
      vfd.print("\x93\xfa\x96\x7b\x8c\xea");
    } else if (i % 4 == 1) {
      vfd.print("Korean:");
      vfd.GU7000_setCursor(vfd.width - 16*3, 0);
      vfd.GU7000_setFontSize(1, 1, true); // Select 8x16 font
      vfd.GU7000_useMultibyteChars(true);  // Turn on multi-byte characters
      vfd.GU7000_setMultibyteCharset(1); // Set characterset
      vfd.print("\xc7\xd1\xb1\xb9\xb8\xbb");
    } else if (i % 4 == 2) {
      vfd.print("Simplified\r\nChinese: ");
      vfd.GU7000_setCursor(vfd.width - 16*3, 0);
      vfd.GU7000_setFontSize(1, 1, true); // Select 8x16 font
      vfd.GU7000_useMultibyteChars(true);  // Turn on multi-byte characters
      vfd.GU7000_setMultibyteCharset(2); // Set characterset
      vfd.print("\xbc\xf2\xcc\xe5\xd7\xd6");
    } else if (i % 4 == 3) {
      vfd.print("Traditional\r\nChinese:");
      vfd.GU7000_setCursor(vfd.width - 16*3, 0);
      vfd.GU7000_setFontSize(1, 1, true); // Select 8x16 font
      vfd.GU7000_useMultibyteChars(true);  // Turn on multi-byte characters
      vfd.GU7000_setMultibyteCharset(3); // Set characterset
      vfd.print("\xc1\x63\xc5\xe9\xa6\x72");
    }
    vfd.GU7000_useMultibyteChars(false); // Turn off multi-byte characters
    vfd.GU7000_setFontSize(1, 1, false); // Select 8x16 font
    delay(1000);
  }

}

void loop() {
}

