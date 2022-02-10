#include <Arduino.h>

#include <Comms.h>

long startTime = 0;
long txCutoff = 0;

void setup() 
{
  Comms::initComms();
  startTime = millis();
}

void loop() {
  #ifdef FLIGHT
  long timeElapsed = millis() - startTime;
  if(timeElapsed%1000 < Comms::txInterval){
    if(Comms::radioMode == Comms::RX){
      Comms::radioMode = Comms::TX;
    }
    Comms::processWaitingPackets();
  }else{
    if(Comms::radioMode == Comms::TX){
      Comms::radioMode = Comms::RX;
      Comms::transmitRadioBuffer(true);
    }
    Comms::processWaitingRadioPacket();
  }
  #elif GROUND
  if(millis() > txCutoff){
    if(Comms::radioMode == Comms::TX){
      Comms::radioMode = Comms::RX;
      Comms::transmitRadioBuffer();
    }
    bool swap = Comms::processWaitingRadioPacket();
    if(swap){
      Comms::radioMode = Comms::TX;
      txCutoff = millis() + (1000-50-Comms::txInterval);
    }
  }else{
    if(Comms::radioMode == Comms::RX){
      Comms::radioMode = Comms::TX;
    }
    Comms::processWaitingPackets();
  }
  #endif
}