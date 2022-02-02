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
  rf24.setTxPower(0x20);

  #ifdef TX
  rf24.setModeTx();
  Serial.println("Starting in transmit mode");
  #else
  rf24.setModeRx();
  Serial.println("Starting in receive mode");
  #endif
}
 
char UDP_RX_BUF[250];
int UDP_BUF_IDX = 0;
char RADIO_RX_BUF[250];
int RADIO_BUF_IDX = 0;

char packetBuffer[100]; 
uint8_t packetLen;

void loop() {
  #ifdef TX
  packetLen = Udp.parsePacket();
  if (packetLen) {
    // Serial.print("Received ethernet packet of size ");
    // Serial.println(packetLen);
    // Serial.print("From ");
    // IPAddress remote = Udp.remoteIP();
    // for (int i=0; i < 4; i++) {
    //   Serial.print(remote[i], DEC);
    //   if (i < 3) {
    //     Serial.print(".");
    //   }
    // }
    // Serial.print(", port ");
    // Serial.println(Udp.remotePort());

    if(UDP_BUF_IDX + packetLen > 250){
      memset(UDP_RX_BUF + UDP_BUF_IDX, 255, 250-UDP_BUF_IDX);
      bool success = rf24.send((uint8_t *) UDP_RX_BUF, UDP_BUF_IDX);
      Serial.println("Forwarding packet over radio");
      if(!success){
        Serial.println("Error forwarding packet over radio");
      }
      UDP_BUF_IDX = 0;
    }

    Udp.read(packetBuffer, packetLen);
    //Serial.print("Contents:");
    //Serial.println(packetBuffer);

    memcpy(UDP_RX_BUF + UDP_BUF_IDX, packetBuffer, packetLen);
    UDP_BUF_IDX += packetLen;

    //Serial.print("UDPBuffer is now size:" ); Serial.println(UDP_BUF_IDX);
  }

  #else
  bool received = rf24.recv((uint8_t *) RADIO_RX_BUF, &packetLen);
  if(received){
    Serial.print("Received radio packet of size ");
    Serial.println(packetLen);

    uint8_t lastRssi = (uint8_t)rf24.lastRssi();
    Serial.print("RSSI:" );
    Serial.println(lastRssi);

    Serial.println("Forwarding through ethernet...");

    int idx = 0;
    while(idx<250){
      int len = RADIO_RX_BUF[idx+1];
      if(len == 255 || len == 0){
        break;
      }
      Serial.println((int) RADIO_RX_BUF[idx]);
      memcpy(packetBuffer, RADIO_RX_BUF + idx, len+8);
      idx += len + 8;
      Udp.beginPacket(ground1, port);
      Udp.write(packetBuffer, len + 8);
      Udp.endPacket();
    }
  }
  #endif
}