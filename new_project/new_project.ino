#include <SoftwareSerial.h>

SoftwareSerial Rfid(8, 9); // RX, TX
SoftwareSerial Bluetooth(10, 11); // RX, TX

void setup() {
  // put your setup code here, to run once:
  Serial.begin(57600);
  Rfid.begin(9600);
  Bluetooth.begin(9600);

  Rfid.listen();
  //Bluetooth.listen();
}

long int time;

void loop() {
  // put your main code here, to run repeatedly:
  
  if( Rfid.available()){
    Serial.println(Rfid.read());
  }
  
/*
  if( Bluetooth.listen() ){
    Serial.println(Bluetooth.read());
  }
*/
/*
  if( Serial.available()){
    Serial.println(Serial.read());
  }
*/
}
