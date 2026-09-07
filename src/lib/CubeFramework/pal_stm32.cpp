#ifdef CUBERACER_M4
#include <Arduino.h>
#include <CubePilotFW/PAL.h>
#include <CubePilotFW/CriticalState.h>
#include <cstring>
static cfCriticalState_t criticalState;

extern "C" uint32_t cubefw_pal_timeNowUs(void)
{
    return micros();
}

extern "C" void cubefw_pal_sleepUs(uint32_t us)
{
    delayMicroseconds(us);
}

extern "C" void cubefw_pal_criticalEnter(void)
{
    const uint32_t previous = __get_PRIMASK();
    __disable_irq();
    cfCriticalEnter(&criticalState, previous);
}

extern "C" void cubefw_pal_criticalExit(void)
{
    uint32_t restore;
    if (cfCriticalExit(&criticalState, &restore))
        __set_PRIMASK(restore);
}

extern "C" void cubefw_pal_shMemcpy(void *out, const void *in, size_t size)
{
    memcpy(out, in, size);
}
#endif
