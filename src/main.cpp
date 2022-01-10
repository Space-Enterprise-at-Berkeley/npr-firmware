#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>


#ifdef TX
IPAddress ip(10, 0, 0, 100);
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFF, 0x00
};
#else
IPAddress ip(10, 0, 0, 101);
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFF, 0x01
};
#endif

IPAddress ground1(10, 0, 0, 69);
IPAddress ground2(10, 0, 0, 70);
unsigned int port = 42069;  
EthernetUDP Udp;

void setup() 
{
  Serial.begin(115200);
  Ethernet.begin(mac, ip);
  Ethernet.init();
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Unable to connect to ethernet module");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }
  Udp.begin(port);
  Serial.println("init success");
}
 
char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; 
uint8_t * packetLen;

void loop() {
  Serial.println(Ethernet.linkStatus());
  delay(1000);
}