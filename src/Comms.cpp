#include <Arduino.h>

#include<Common.h>
#include <Comms.h>

#include <Ethernet.h>
#include <EthernetUdp.h>
#include <RH_RF24.h>

namespace Comms {

    #define MAX_RADIO_TRX_SIZE 249

    std::map<uint8_t, commFunction> callbackMap;
    EthernetUDP Udp;
    char packetBuffer[sizeof(Packet)];

    RH_RF24 rf24(PA4, PA3, PA1);

    uint8_t radioBuffer[MAX_RADIO_TRX_SIZE];
    uint8_t radioBufferSize= 0;

    int txInterval = TX_INT;

    mode radioMode;

    void initComms() {
        Serial.begin(115200);
  
        Ethernet.init(PA11);
        Ethernet.begin((uint8_t *)mac, ip);

        Udp.begin(port);

        rf24.init();
        rf24.setFrequency(433.0); // Only within the same frequency band
        rf24.setTxPower(0x10);

        #ifdef FLIGHT
        rf24.setModeTx();
        radioMode = TX;
        DEBUG("Starting in flight mode");
        #else
        rf24.setModeRx();
        radioMode  = RX;
        DEBUG("Starting in ground mode");
        #endif
    }

    void registerCallback(uint8_t id, commFunction function) {
        callbackMap.insert(std::pair<int, commFunction>(id, function));
    }

    /**
     * @brief Checks checksum of packet and tries to call the associated callback function.
     * 
     * @param packet Packet to be processed.
     */
    bool evokeCallbackFunction(Packet *packet) {
        uint16_t checksum = *(uint16_t *)&packet->checksum;
        if (checksum == computePacketChecksum(packet)) {
            // DEBUG("Packet with ID ");
            // DEBUG(packet->id);
            // DEBUG(" has correct checksum!\n");
            //try to access function, checking for out of range exception
            if(callbackMap.count(packet->id)) {
                callbackMap.at(packet->id)(*packet);
                return true;
            } else {
                // DEBUG("ID ");
                // DEBUG(packet->id);
                // DEBUG(" does not have a registered callback function.\n");
                return false;
            }
        } else {
            DEBUG("Packet with ID ");
            DEBUG(packet->id);
            DEBUG(" does not have correct checksum!\n");
        }
    }

    void transmitRadioBuffer(bool swapFlag){
        if(radioBufferSize == 0){
            return;
        }

        if(swapFlag){
            radioBuffer[radioBufferSize] = 255;
            radioBufferSize++;
        }
        bool success = rf24.send(radioBuffer, radioBufferSize);
        DEBUG("Transmitting Radio Packet\n");
        if(!success){
            DEBUG("Error Transmitting Radio Packet");
        }
        radioBufferSize = 0;
    }
    void transmitRadioBuffer(){ transmitRadioBuffer(false);}


    void processWaitingPackets() {
        if(Udp.parsePacket()) {
            if(Udp.remotePort() != port) return; // make sure this packet is for the right port
            if(!(Udp.remoteIP() == ethDestination1) && !(Udp.remoteIP() == ethDestination2)) return; // make sure this packet is from a ground station computer
            Udp.read(packetBuffer, sizeof(Packet));

            Packet *packet = (Packet *)&packetBuffer;

            if(!evokeCallbackFunction(packet)){
                int packetLen = packet->len + 8;
                if(radioBufferSize + packetLen > MAX_RADIO_TRX_SIZE){
                    transmitRadioBuffer();
                }

                memcpy(radioBuffer + radioBufferSize, packetBuffer, packetLen);
                radioBufferSize += packetLen;
            }
        }
    }

    bool processWaitingRadioPacket() {
        bool received = rf24.recv(radioBuffer, &radioBufferSize);
        if(received){
            DEBUG("Received radio packet of size ");
            DEBUG(radioBufferSize);
            DEBUG("\n");

            uint8_t lastRssi = (uint8_t)rf24.lastRssi();
            DEBUG("RSSI:" );
            DEBUG(lastRssi);
            DEBUG("\n");

            int idx = 0;
            while(idx<radioBufferSize){
                int packetID = radioBuffer[idx];
                if(packetID == 255){
                    radioBufferSize = 0;
                    return true;
                }
                int packetLen = radioBuffer[idx+1];

                memcpy(packetBuffer, radioBuffer + idx, packetLen+8);

                idx += packetLen + 8;

                Packet *packet = (Packet *)&packetBuffer;

                emitPacket(packet, false);
            }
        }
        return false;
    }


    void packetAddFloat(Packet *packet, float value) {
        uint32_t rawData = * ( uint32_t * ) &value;
        packet->data[packet->len] = rawData & 0xFF;
        packet->data[packet->len + 1] = rawData >> 8 & 0xFF;
        packet->data[packet->len + 2] = rawData >> 16 & 0xFF;
        packet->data[packet->len + 3] = rawData >> 24 & 0xFF;
        packet->len += 4;
    }

    void packetAddUint8(Packet *packet, uint8_t value) {
        packet->data[packet->len] = value;
        packet->len++;
    }

    float packetGetFloat(Packet *packet, uint8_t index) {
        uint32_t rawData = packet->data[index+3];
        rawData <<= 8;
        rawData += packet->data[index+2];
        rawData <<= 8;
        rawData += packet->data[index+1];
        rawData <<= 8;
        rawData += packet->data[index];
        return * ( float * ) &rawData;
    }

    uint32_t packetGetUint32(Packet *packet, uint8_t index) {
        uint32_t rawData = packet->data[index+3];
        rawData <<= 8;
        rawData += packet->data[index+2];
        rawData <<= 8;
        rawData += packet->data[index+1];
        rawData <<= 8;
        rawData += packet->data[index];
        return rawData;
    }

    /**
     * @brief Sends packet to both groundstations.
     * 
     * @param packet Packet to be sent.
     */
    void emitPacket(Packet *packet, bool genHeader = true) {

        if(genHeader){
            //add timestamp to struct
            uint32_t timestamp = millis();
            packet->timestamp[0] = timestamp & 0xFF;
            packet->timestamp[1] = (timestamp >> 8) & 0xFF;
            packet->timestamp[2] = (timestamp >> 16) & 0xFF;
            packet->timestamp[3] = (timestamp >> 24) & 0xFF;

            //calculate and append checksum to struct
            uint16_t checksum = computePacketChecksum(packet);
            packet->checksum[0] = checksum & 0xFF;
            packet->checksum[1] = checksum >> 8;
        }

        // Send over serial, but disable if in debug mode
        #ifndef DEBUG_MODE
        // Serial.print(packet->id);
        // Serial.print(packet->len);
        // Serial.print(packet->timestamp, 4);
        // Serial.print(packet->checksum, 2);
        // Serial.print(packet->data, packet->len);
        // Serial.print('\n');
        #endif

        //Send over ethernet to both ground stations
        Udp.beginPacket(ethDestination1, port);
        Udp.write(packet->id);
        Udp.write(packet->len);
        Udp.write(packet->timestamp, 4);
        Udp.write(packet->checksum, 2);
        Udp.write(packet->data, packet->len);
        Udp.endPacket();

        // Udp.beginPacket(ethDestination2, port);
        // Udp.write(packet->id);
        // Udp.write(packet->len);
        // Udp.write(packet->timestamp, 4);
        // Udp.write(packet->checksum, 2);
        // Udp.write(packet->data, packet->len);
        // Udp.endPacket();
    }

    void emitPacket(Packet *packet) { emitPacket(packet, true); }

    /**
     * @brief generates a 2 byte checksum from the information of a packet
     * 
     * @param data pointer to data array
     * @param len length of data array
     * @return uint16_t 
     */
    uint16_t computePacketChecksum(Packet *packet) {

        uint8_t sum1 = 0;
        uint8_t sum2 = 0;

        sum1 = sum1 + packet->id;
        sum2 = sum2 + sum1;
        sum1 = sum1 + packet->len;
        sum2 = sum2 + sum1;
        
        for (uint8_t index = 0; index < 4; index++) {
            sum1 = sum1 + packet->timestamp[index];
            sum2 = sum2 + sum1;
        }

        for (uint8_t index = 0; index < packet->len; index++) {
            sum1 = sum1 + packet->data[index];
            sum2 = sum2 + sum1;
        }
        return (((uint16_t)sum2) << 8) | (uint16_t) sum1;
    }
};