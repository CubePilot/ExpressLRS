#include "crsf_connector.h"
#if defined(CUBERACER_M4) || defined(UNIT_TEST)
#include "receiver_link.h"
#include "CRSFRouter.h"
#include <CubePilotFW/ReceiverCrsf.h>

CubeFrameworkCrsfConnector::CubeFrameworkCrsfConnector()
{
    addDevice(CRSF_ADDRESS_FLIGHT_CONTROLLER);
}

bool CubeFrameworkCrsfConnector::receive(const uint8_t *frame, uint8_t length)
{
    if (!elrsCfConfigReady() || !CubePilotFW::receiverCrsfValid(frame, length) ||
        crsfRouter.crsf_crc.calc(frame + 2, length - 3) != frame[length - 1])
        return false;
    const uint8_t type = frame[2];
    // Shared CRSF carries telemetry/management; RC and statistics use typed mailboxes.
    if (type == CRSF_FRAMETYPE_RC_CHANNELS_PACKED || type == 0x17 /* subset RC */ || type == CRSF_FRAMETYPE_LINK_STATISTICS)
        return false;
    if ((type == CRSF_FRAMETYPE_PARAMETER_READ || type == CRSF_FRAMETYPE_PARAMETER_WRITE || type == CRSF_FRAMETYPE_COMMAND) && length < 8)
        return false;
    // RXEndpoint consumes five encapsulated MSP bytes for model-ID writes.
    if (type == CRSF_FRAMETYPE_MSP_WRITE && (frame[3] == CRSF_ADDRESS_CRSF_RECEIVER || frame[3] == CRSF_ADDRESS_BROADCAST) && length < 11)
        return false;
    // The upstream endpoint reads two parameter bytes even for a payload-less ping.
    // Copy to a zero-filled bounded frame so those unused bytes are defined.
    uint8_t copy[CRSF_MAX_PACKET_LEN] = {};
    memcpy(copy, frame, length);
    elrsCfTelemetrySensor(type);
    crsfRouter.processMessage(this, reinterpret_cast<const crsf_header_t *>(copy));
    return true;
}

void CubeFrameworkCrsfConnector::forwardMessage(const crsf_header_t *message)
{
    if (!message || !elrsCfForwardToFcAllowed())
        return;
    const unsigned length = unsigned(message->frame_size) + 2;
    const auto *frame = reinterpret_cast<const uint8_t *>(message);
    if (!CubePilotFW::receiverCrsfValid(frame, length) || message->type < CRSF_FRAMETYPE_DEVICE_PING)
        return;
    const auto *ext = reinterpret_cast<const crsf_ext_header_t *>(message);
    if (ext->dest_addr != CRSF_ADDRESS_FLIGHT_CONTROLLER && ext->dest_addr != CRSF_ADDRESS_BROADCAST)
        return;
    elrsCfSendCrsf(frame, length);
}
#endif
