#include "SerialCubeFramework.h"
#ifdef CUBERACER_M4
#include "receiver_link.h"
#include "receiver_rc.h"
#include "common.h"
#include "crsf_protocol.h"
#include "device.h"
static_assert(ElrsCfRcPublisher::CHANNEL_UNSET==CRSF_CHANNEL_VALUE_UNSET &&
    ElrsCfRcPublisher::CHANNEL_EMPTY==CRSF_CHANNEL_VALUE_EXT_MIN, "Match the normal SerialIO unset-channel conversion");
uint32_t SerialCubeFramework::sendRCFrame(bool frameAvailable,bool frameMissed,uint32_t *channels)
{
    (void)frameMissed; // A missing window never republishes the previous channels.
    elrsCfPublishChannels(frameAvailable,connectionHasModelMatch,
        !teamraceHasModelMatch || connectionState!=connected,channels);
    return DURATION_IMMEDIATELY;
}
#endif
