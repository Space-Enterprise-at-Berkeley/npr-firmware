#include <Arduino.h>

#include <Comms.h>
#include <Radio.h>
#include <Si446x.h>

long startTime = 0;
long txCutoff = 0;

unsigned long previousMillis = 0;
const long interval = 15;

Comms::Packet spoofPacket = {.id = 10};

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


void setup() 
{
  Serial.begin(115200);
  Radio::initRadio();
  startTime = millis(); 

  for(int i = 0; i<7;i++){
    Comms::packetAddFloat(&spoofPacket, 69);
  }
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
    uint32_t timestamp = millis();
    spoofPacket.timestamp[0] = timestamp & 0xFF;
    spoofPacket.timestamp[1] = (timestamp >> 8) & 0xFF;
    spoofPacket.timestamp[2] = (timestamp >> 16) & 0xFF;
    spoofPacket.timestamp[3] = (timestamp >> 24) & 0xFF;

    //calculate and append checksum to struct
    uint16_t checksum = Comms::computePacketChecksum(&spoofPacket);
    spoofPacket.checksum[0] = checksum & 0xFF;
    spoofPacket.checksum[1] = checksum >> 8;

    Radio::forwardPacket(&spoofPacket);
  }
}