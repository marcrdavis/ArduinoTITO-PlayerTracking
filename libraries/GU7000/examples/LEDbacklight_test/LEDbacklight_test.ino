#include <SoftwareSerial.h>
SoftwareSerial mySerial(10, 4, true); //RX, TX, _inverse_logic

//Connect Arduino pin 7 to module pin 4 to check the BUSY signal.

void setup() {
  // put your setup code here, to run once:
  Serial.begin(38400);
  while(!Serial){
    ;
  }
  mySerial.begin(38400);
  mySerial.println("Noritake");
  VFDwrite(0x1F);
  VFDwrite(0x4C);
  VFDwrite(0x00);
  VFDwrite(0xF0);
}

void loop() {
  // put your main code here, to run repeatedly:
  
}

void VFDwrite(uint8_t data){
  while(digitalRead(7));
      mySerial.write(data);
  return;
}

