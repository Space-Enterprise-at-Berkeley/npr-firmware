#include <Arduino.h>

#include <Common.h>
#include <Comms.h>

#include <Radio.h>

#include <Si446x.h>

namespace Radio {

    mode radioMode;
    int txInterval = TX_INT;

    uint8_t radioBuffer[MAX_RADIO_TRX_SIZE];
    uint8_t radioBufferSize= 0;

    volatile bool transmitting = false;

    volatile recvRadio_t recvRadio;

    char packetBuffer[sizeof(Comms::Packet)];
    
    void initRadio() {

        Si446x_init();
        Si446x_setTxPower(127);
        Si446x_setupCallback(SI446X_CBS_SENT, 1); 

        #ifdef FLIGHT
        radioMode = TX;
        DEBUG("Starting in flight mode");
        #else
        Si446x_RX(0);
        radioMode  = RX;
        DEBUG("Starting in ground mode");
        #endif
    }

    void transmitRadioBuffer(bool swapFlag){
        if(radioBufferSize == 0){
            return;
        }
        if(swapFlag){
            radioBuffer[radioBufferSize] = 255;
            radioBufferSize++;
        }
        bool success = Si446x_TX(radioBuffer, radioBufferSize, 0, SI446X_STATE_RX);
        transmitting = true;
        DEBUG("Transmitting Radio Packet\n");
        if(!success){
            DEBUG("Error Transmitting Radio Packet");
        }
        radioBufferSize = 0;
    }
    void transmitRadioBuffer(){ transmitRadioBuffer(false);}

    void forwardPacket(Comms::Packet *packet){
        int packetLen = packet->len + 8;
        if(radioBufferSize + packetLen > MAX_RADIO_TRX_SIZE - 1){
            transmitRadioBuffer();
        }
        memcpy(radioBuffer + radioBufferSize, (uint8_t *) packet, packetLen);
        radioBufferSize += packetLen;
    }

    bool processWaitingRadioPacket() {
        if(recvRadio.ready == 1){
            DEBUG("Received radio packet of size ");
            DEBUG(recvRadio.length);
            DEBUG("\n");

            int16_t lastRssi = recvRadio.rssi;
            DEBUG("RSSI:" );
            DEBUG(lastRssi);
            DEBUG("\n");

            memcpy(radioBuffer, (uint8_t *)recvRadio.buffer, recvRadio.length);

            recvRadio.ready == 0;

            int idx = 0;
            while(idx<recvRadio.length){
                int packetID = radioBuffer[idx];
                if(packetID == 255){
                    radioBufferSize = 0;
                    return true;
                }

                int packetLen = radioBuffer[idx+1];

                memcpy(packetBuffer, (uint8_t *)radioBuffer + idx, packetLen+8);

                idx += packetLen + 8;

                Comms::Packet *packet = (Comms::Packet *) &packetBuffer;

                if(packet->id == 10){
                    float rssi = (float) recvRadio.rssi;
                    uint32_t rawData = * ( uint32_t * ) &rssi;
                    packet->data[0] = rawData & 0xFF;
                    packet->data[1] = rawData >> 8 & 0xFF;
                    packet->data[2] = rawData >> 16 & 0xFF;
                    packet->data[3] = rawData >> 24 & 0xFF;

                    uint16_t checksum = computePacketChecksum(packet);
                    packet->checksum[0] = checksum & 0xFF;
                    packet->checksum[1] = checksum >> 8;
                }
                Comms::emitPacket(packet, false);

            }
            recvRadio.ready = 0;
        }
        return false;
    }
}
