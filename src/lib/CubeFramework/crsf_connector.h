#pragma once
#if defined(CUBERACER_M4) || defined(UNIT_TEST)
#include "CRSFConnector.h"

class CubeFrameworkCrsfConnector final : public CRSFConnector
{
public:
    CubeFrameworkCrsfConnector();
    bool receive(const uint8_t *frame, uint8_t length);
    void forwardMessage(const crsf_header_t *message) override;
};

bool elrsCfForwardToFcAllowed();
void elrsCfTelemetrySensor(uint8_t type);
#endif
