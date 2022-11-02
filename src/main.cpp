#include <Arduino.h>

#include <Comms.h>
#include <Radio.h>
#include <Si446x.h>

#include <BlackBox.h>

int arr[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09};
int ctr = 0;
void SI446X_CB_SENT(void)
{
    Radio::transmitting = false;
}

void SI446X_CB_RXCOMPLETE(uint8_t length, int16_t rssi)
{   
    if(length > MAX_RADIO_TRX_SIZE) length = MAX_RADIO_TRX_SIZE;

    Radio::recvRadio.ready = 1;
    Radio::recvRadio.rssi = rssi;
    Radio::recvRadio.length = length;

    Si446x_read((uint8_t*)Radio::recvRadio.buffer, length);
    Si446x_RX(0);
}

void SI446X_CB_RXINVALID(int16_t rssi)
{
	Si446x_RX(0);

	// Printing to serial inside an interrupt is bad!
	// If the serial buffer fills up the program will lock up!
	// Don't do this in your program, this only works here because we're not printing too much data
	Serial.print(F("Packet CRC failed (RSSI: "));
	Serial.print(rssi);
	Serial.println(F(")"));
}

Comms::Packet testPacket = {.id = 5};


void setup() 
{
  Serial.begin(115200);
  Serial.println("starting up");
  
  Comms::initComms();
  Radio::initRadio();
  BlackBox::init();
  
  Serial.println("hi");
  Serial.println("HII");
}

int delayS;
void loop() {

  #ifdef TEST
    if ((millis() % 10000) > 5000) {
      delayS = 10;
    } else {
      delayS = 0;
    }
    testPacket.len = 0;
    Comms::packetAddFloat(&testPacket, millis() % 1000);
    Comms::packetAddFloat(&testPacket, millis() % 1000);
    Comms::packetAddFloat(&testPacket, millis() % 1000);
    Comms::emitPacket(&testPacket);
    delay(delayS);
  #endif
  #ifdef FLIGHT
  Comms::processWaitingPackets();
  #endif

  //Comms::processWaitingPackets();
  //Radio::txCalib10(arr, 0);
  //delay(1000);
  // digitalWrite(33, LOW);
  // digitalWrite(33, HIGH);


  // Radio::txZeros(33); //tx's [0, 1, 2]
  // delayMicroseconds(2000000);  
  // Radio::txZeros(33); //tx's [0, 1, 2]
  // delayMicroseconds(5000000);
}
