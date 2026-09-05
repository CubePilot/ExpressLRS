#ifdef CUBERACER_M4
#include "receiver_link.h"
#include <Arduino.h>
#include <CubePilotFW/ReceiverEndpoint.h>
#include <CubePilotFW/ReceiverLayout.h>
#include <CubePilotFW/pal/HSEM.h>
using namespace CubePilotFW;
namespace {
struct BootRecord { uint32_t magic,value,inverse; };
static_assert(sizeof(BootRecord)==12);
volatile BootRecord record __attribute__((section(".noinit.cuberacer_boot")));
constexpr uint32_t BOOT_MAGIC=0x43525831;
ReceiverEndpoint endpoint;
uint32_t lastHello=0;
bool active=false;
}
void elrsCfInit(void)
{
    const uint32_t boot=record.magic==BOOT_MAGIC ? cfRxNextBoot(record.value,record.inverse) : 1;
    record.magic=0;record.value=boot;record.inverse=~boot;__DMB();record.magic=BOOT_MAGIC;
    HwSemaphore::init(); // Local MPU/IRQ only: shared HSEM clock is M7-owned.
    const uint32_t now=micros();
    active=endpoint.begin(false,boot,reinterpret_cast<uint8_t *>(ReceiverLayout::BASE),now);
    lastHello=now-ReceiverSession::HEARTBEAT_US;
}
void elrsCfPoll(uint32_t nowUs)
{
    if (!active) return;
    const bool wasLinked=endpoint.session().linked(nowUs);
    const bool received=endpoint.pollControl(nowUs);
    if ((received && !wasLinked) || uint32_t(nowUs-lastHello)>=ReceiverSession::HEARTBEAT_US) {
        endpoint.sendHello(false,false);lastHello=nowUs;
    }
    // No GPIO, EEPROM, radio or configuration application before the later adapter.
}
#endif
