#include <Arduino.h>
#include <SPI.h>
#include <RH_RF24.h>

RH_RF24 rf24(PA4, PA3, PA1);

void setup() 
{
  Serial.begin(9600);
  if (!rf24.init()) Serial.println("init failed");
  // The default radio config is for 30MHz Xtal, 434MHz base freq 2GFSK 5kbps 10kHz deviation
  // power setting 0x10
  // If you want a different frequency mand or modulation scheme, you must generate a new
  // radio config file as per the RH_RF24 module documentation and recompile
  // You can change a few other things programatically:
  rf24.setFrequency(433.0); // Only within the same frequency band
  rf24.setTxPower(0x10);
}
 

void loop() {
  digitalWrite(PB_1, HIGH);
  Serial.println(rf24.get_temperature());
  delay(1000);
  digitalWrite(PB_1, LOW);
  delay(1000);
}