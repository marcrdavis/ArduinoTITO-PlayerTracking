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
#include <GU7000_Serial_Sync.h>
#include <Noritake_VFD_GU7000.h>

// ****************************************************
// ****************************************************
// Uncomment one of the communication interfaces below.
//
//GU7000_Serial_Async interface(38400,3, 5, 7); // BAUD RATE,SIN,BUSY,RESET
//GU7000_Serial_Sync interface(3, 5, 6, 7); // SIN,BUSY,SCK,RESET
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
  vfd.GU7000_reset();       // reset module
  vfd.GU7000_init();        // initialize module
  
  // A 140x16 image must be loaded on the
  // module's Flash ROM address 0x00000
  vfd.GU7000_drawFROMImage(0, 16, 140, 16); // logo16.jpg
}

void loop() {
}

