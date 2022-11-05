#pragma once

#include <SPIFlash.h>
#include "Comms.h"

namespace BlackBox {

    void init();
    void writePacket(Comms::Packet packet);
    Comms::Packet getData(uint32_t byteAddress);
    void erase();

    void packetHandler(Comms::Packet packet);
}