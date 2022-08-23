#include <Arduino.h>

#include <Common.h>
#include <Comms.h>

#include <Radio.h>

namespace Comms {

    std::map<uint8_t, commFunction> callbackMap;
    uint8_t packetBuffer[1000];
    uint8_t packetBufferCtr = 0;
    uint32_t goodPackets, failedPackets;

    void initComms() {
        Serial.begin(115200);
        Serial1.begin(115200);
        Serial.setTimeout(1);
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
            return false;
        }
    }

    void processWaitingPackets() {

        while(Serial1.available()) {

            int serialByte = Serial1.read();
            if(serialByte == -1) return;
            packetBuffer[packetBufferCtr] = serialByte;
            if ((packetBufferCtr > 3) && (packetBuffer[packetBufferCtr-2] == '\r') && (packetBuffer[packetBufferCtr-1] == '\n') && (packetBuffer[packetBufferCtr] == '\n')) {
                Packet* packet = (Packet*) &packetBuffer; 
                uint16_t receivedCheck = *(uint16_t *)&packet->checksum;
                if (computePacketChecksum(packet) != receivedCheck) {
                    Serial.printf("packet checksum failed: received id %d. received check: %d, expected check: %d, failed %: %.3f\n", 
                    packet->id, packet->checksum, computePacketChecksum(packet), (double)failedPackets / (double)goodPackets); 
                    failedPackets++;
                } else {
                    // Serial.printf("got good packet! with id %d, len %d\n", packet->id, packet->len);
                    goodPackets++;
                    Radio::forwardPacket(packet);
                }
                packetBufferCtr = -1;
            }
            packetBufferCtr++;
        }
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
        // Udp.beginPacket(ethDestination1, port);
        // Udp.write(packet->id);
        // Udp.write(packet->len);
        // Udp.write(packet->timestamp, 4);
        // Udp.write(packet->checksum, 2);
        // Udp.write(packet->data, packet->len);
        // Udp.endPacket();

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