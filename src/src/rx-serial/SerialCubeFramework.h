#pragma once
#ifdef CUBERACER_M4
#include "SerialIO.h"
class SerialCubeFramework final : public SerialIO {
public:
    SerialCubeFramework() : SerialIO(nullptr,nullptr) {}
    uint32_t sendRCFrame(bool frameAvailable,bool frameMissed,uint32_t *channels) override;
    bool sendImmediateRC() override { return true; }
    void sendQueuedData(uint32_t) override {}
    void processSerialInput() override {}
private:
    void processBytes(uint8_t *,uint16_t) override {} // No unframed serial input on this backend.
};
#endif
