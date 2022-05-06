#include <Arduino.h>

#include <Comms.h>
#include <Radio.h>
#include <Si446x.h>

long startTime = 0;
long txCutoff = 0;

unsigned long previousMillis = 0;
const long interval = 5;

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
  Comms::initComms();
  Radio::initRadio();
  startTime = millis(); 
}

void loop() {
  Comms::processWaitingPackets();
}