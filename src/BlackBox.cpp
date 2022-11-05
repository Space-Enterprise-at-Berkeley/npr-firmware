#include "BlackBox.h"

namespace BlackBox {

    uint16_t expectedDeviceID=0xEF40;
    SPIFlash flash(1, expectedDeviceID);
    uint32_t addr;

    bool enable = false;

    void init() {
        pinMode(1, OUTPUT);
        if (flash.initialize()) {
            Serial.println("Init OK!");
        } else {
            Serial.print("Init FAIL, expectedDeviceID(0x");
            Serial.print(expectedDeviceID, HEX);
            Serial.print(") mismatched the read value: 0x");
            Serial.println(flash.readDeviceId(), HEX);
        }

        Comms::registerCallback(200, packetHandler);
    }

    void packetHandler(Comms::Packet packet) {
        uint8_t data = packet.data[0];
        enable = data;
        if (!data) {
            erase();
        }
    }

    void writePacket(Comms::Packet packet) {
        if (enable) {
            uint16_t len = 8 + packet.len;
            flash.writeBytes(addr, &packet, len);
            addr += len;
        }
    }

    Comms::Packet getData(uint32_t byteAddress) {
        Comms::Packet packet;
        flash.readBytes(byteAddress, &packet, sizeof(Comms::Packet));
        return packet;
    }

    void erase() {
        Serial.println("starting chip erase");
        flash.chipErase();
    }

    void getAllData() {
        for(int i = 0; i < addr; i++) {
            Serial.write(flash.readByte(addr));
        }
    }
}