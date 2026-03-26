/*
  CubeNode platform support:
  - Early init (constructor): FPU fix, flash clock, DFU check
  - DFU reboot: magic value in DTCM + NVIC reset
*/
#include "stm32h7xx_hal.h"

/* Linker symbol for top of stack */
extern char _estack[];

/* DFU magic value stored in DTCM RAM — survives NVIC_SystemReset */
#define DFU_MAGIC_ADDR  ((uint32_t *)0x20000000)
#define DFU_MAGIC_VALUE 0xDEADBEEFU

/* STM32H757 system memory bootloader (DFU) entry point */
#define SYSMEM_ADDR     0x1FF09800U

// __attribute__((constructor(50))) void cubenode_early_init(void)
// {
//     /* Check for DFU reboot request before any peripheral init.
//        The magic value is set by cubenode_reset_into_dfu() and
//        survives NVIC_SystemReset because DTCM is not cleared. */
//     if (*DFU_MAGIC_ADDR == DFU_MAGIC_VALUE)
//     {
//         *DFU_MAGIC_ADDR = 0;

//         /* Minimal cleanup for clean DFU entry */
//         __disable_irq();
//         HAL_RCC_DeInit();
//         HAL_DeInit();
//         SysTick->CTRL = 0;
//         SysTick->LOAD = 0;
//         SysTick->VAL  = 0;

//         /* Clear all NVIC interrupts */
//         for (int i = 0; i < 8; i++) {
//             NVIC->ICER[i] = 0xFFFFFFFFU;
//             NVIC->ICPR[i] = 0xFFFFFFFFU;
//         }

//         /* Jump to system memory bootloader */
//         __set_MSP(*(uint32_t *)SYSMEM_ADDR);
//         ((void (*)(void))(*(uint32_t *)(SYSMEM_ADDR + 4)))();
//         while (1) {} /* never reached */
//     }

//     /* Enable flash interface clock — needed for bank 2 access on H757 */
//     RCC->AHB3ENR |= RCC_AHB3ENR_FLASHEN;

//     /* Disable FPU lazy stacking — stm32duino has no ISR epilogue for it */
//     FPU->FPCCR &= ~FPU_FPCCR_LSPEN_Msk;
// }

/* Called from ELRS to reboot into USB DFU mode.
   Jumps directly to the system memory bootloader.
   Reference: https://community.st.com/t5/stm32-mcus/jump-to-bootloader-from-application-on-stm32h7-devices/ta-p/49510 */
void cubenode_reset_into_dfu(void)
{
    /* Deinit HAL and clocks */
    HAL_DeInit();
    HAL_RCC_DeInit();

    /* Disable interrupts */
    __disable_irq();

    /* Disable SysTick */
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;

    /* Disable all NVIC interrupts and clear pending */
    for (uint32_t i = 0; i < sizeof(NVIC->ICER)/sizeof(NVIC->ICER[0]); i++) {
        NVIC->ICER[i] = 0xFFFFFFFFU;
        NVIC->ICPR[i] = 0xFFFFFFFFU;
    }

    /* Re-enable interrupts — DFU bootloader needs them */
    __enable_irq();

    /* Reset vector table to system memory */
    SCB->VTOR = SYSMEM_ADDR;

    /* Ensure thread mode uses MSP */
    __set_CONTROL(0);
    __ISB();

    /* Set MSP from bootloader vector table */
    __set_MSP(*(uint32_t *)SYSMEM_ADDR);
    __DSB();
    __ISB();

    /* Jump to bootloader reset handler */
    ((void (*)(void))(*(uint32_t *)(SYSMEM_ADDR + 4)))();
    while (1) {}
}
