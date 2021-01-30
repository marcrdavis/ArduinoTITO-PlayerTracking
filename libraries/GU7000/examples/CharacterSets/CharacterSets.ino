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
  vfd.isModelClass(7000);
  //vfd.isGeneration('B');    // Uncomment this for B generation
  vfd.GU7000_reset();       // reset module
  vfd.GU7000_init();        // initialize module
  
  
  vfd.GU7000_setFontStyle(true, false);
  
  for (int i = 0; ; i++) {
    vfd.GU7000_clearScreen();
    
    if (i % 4 == 0) {
      vfd.GU7000_setCharset(1);
      vfd.print("\xb2\xdb\xca\xc6\xce\xcd\xc4\x20\xc1\xd8\xc7\xd9\xa6\x20\xdc\xb6\xd6\xc0\xda\xbf\x20\xc2\xc8\xc5\xd7\xd1\x20\xb3\xb2\xc9\xb5\xb8\xd4\xcf\x20\xb9\xcc\xba\xb4\xc3\x20\xb1\xbb\xb7\xd5\xd2\xd0\xbc");
    } else if (i % 4 == 1) {
      vfd.GU7000_setCharset(0);
      vfd.print("L'H\x93pital's rule for limits\r\ninvolving 0/0 and \xec/\xec");
    } else if (i % 4 == 2) {
      vfd.GU7000_setCharset(0);
      vfd.print("Pi\244a Colada ");
      vfd.GU7000_setCharset(16);
      vfd.print("\xa3\x31\x30\x2f\x80\x31\x32\x2f\xa5\x31\x33\x30\x30");
      vfd.GU7000_setCharset(0);
      vfd.print("\r\nCacha\207a ");
      vfd.GU7000_setCharset(16);
      vfd.print("\xa3\x31\x32\x2f\x80\x31\x35\x2f\xa5\x31\x36\x30\x30");
    } else if (i % 4 == 3) {
      vfd.GU7000_setCharset(17); // Set character set
      vfd.print("\x92\xa0\xaa\x20\xa3\xae\xa2\xae\xe0\xa8\xab\xa0\x20\xa2\x20\xa8\xee\xab\xa5\x20\x31\x38\x30\x35\x20\xa3\xae\xa4\xa0\x20\xa8\xa7\xa2\xa5\xe1\xe2\xad\xa0\xef\x20\x80\xad\xad\xa0");
    }
    delay(1000);
  }
}

void loop() {
}

