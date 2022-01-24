#include <Arduino.h>
#include <SPI.h>
#include <RH_RF24.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

RH_RF24 rf24(PA4, PA3, PA1);

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
  
  Ethernet.init(PA11);
  Ethernet.begin(mac, ip);
  
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

  if (!rf24.init()) Serial.println("init failed");
  // The default radio config is for 30MHz Xtal, 434MHz base freq 2GFSK 5kbps 10kHz deviation
  // power setting 0x10
  // If you want a different frequency mand or modulation scheme, you must generate a new
  // radio config file as per the RH_RF24 module documentation and recompile
  // You can change a few other things programatically:
  rf24.setFrequency(433.0); // Only within the same frequency band
  rf24.setTxPower(0x10);

  #ifdef TX
  rf24.setModeTx();
  Serial.println("Starting in transmit mode");
  #else
  rf24.setModeRx();
  Serial.println("Starting in receive mode");
  #endif
}
 
char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; 
// char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; 
uint8_t packetLen;

void loop() {
  #ifdef TX
  packetLen = Udp.parsePacket();
  if (packetLen) {
    Serial.print("Received ethernet packet of size ");
    Serial.println(packetLen);
    Serial.print("From ");
    IPAddress remote = Udp.remoteIP();
    for (int i=0; i < 4; i++) {
      Serial.print(remote[i], DEC);
      if (i < 3) {
        Serial.print(".");
      }
    }
    Serial.print(", port ");
    Serial.println(Udp.remotePort());

    // read the packet into packetBufffer
    Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    Serial.print("Contents:");
    Serial.println(packetBuffer);

    Serial.println("Forwarding over radio...");

    bool success = rf24.send((uint8_t *) packetBuffer, packetLen);
    if(!success){
      Serial.println("Error forwarding packet over radio");
    }
  }
  #else
  bool received = rf24.recv((uint8_t *) packetBuffer, &packetLen);
  if(received){
    Serial.print("Received radio packet of size ");
    Serial.println(packetLen);

    Serial.print("Contents:");
    Serial.println(packetBuffer);

    uint8_t lastRssi = (uint8_t)rf24.lastRssi();
    Serial.print("RSSI:" );
    Serial.println(lastRssi);

    Serial.println("Forwarding through ethernet...");

    Udp.beginPacket(ground1, port);
    Udp.write(packetBuffer, packetLen);
    Udp.endPacket();
  }
  #endif
}