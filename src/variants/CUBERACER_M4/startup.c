#include "stm32h7xx.h"
#include "stm32h7xx_hal.h"
#include "dwt.h"

#if !defined(CORE_CM4) || defined(CORE_CM7)
#error "CubeRacer receiver must compile for CM4"
#endif

// Called before .data/.bss initialization. Only touch this core's registers.
void __wrap_SystemInit(void)
{
    SCB->CPACR |= (3UL << 20) | (3UL << 22);
    SCB->VTOR = 0x08180000UL;
    __DSB();
    __ISB();
}

// Supply and shared clocks were configured by M7 before releasing M4.
void __wrap_ExitRun0Mode(void) {}

// Override Arduino's weak init: HAL_Init would reset shared peripheral policy.
void init(void)
{
    SystemCoreClockUpdate();
    HAL_InitTick(TICK_INT_PRIORITY);
    // Arduino delayMicroseconds uses this core's cycle counter.
    dwt_init();
}
