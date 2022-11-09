#pragma once

#include <SPIFlash.h>
#include "Comms.h"

namespace BlackBox {

    const uint32_t FLASH_SIZE = 1.6e7;

    void init();
    void writePacket(Comms::Packet packet);
    Comms::Packet getData(uint32_t byteAddress);
    void erase();

    void packetHandler(Comms::Packet packet);

    uint32_t getAddr();
}