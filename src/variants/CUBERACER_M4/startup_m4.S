/**
  ******************************************************************************
  * @file      startup_stm32h757xx.s
  * @author    MCD Application Team
  * @brief     STM32H757xx Devices vector table for GCC based toolchain.
  *            This module performs:
  *                - Set the initial SP
  *                - Set the initial PC == Reset_Handler,
  *                - Set the vector table entries with the exceptions ISR address
  *                - Branches to main in the C library (which eventually
  *                  calls main()).
  *            After Reset the Cortex-M processor is in Thread mode,
  *            priority is Privileged, and the Stack is set to Main.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

  .syntax unified
  .cpu cortex-m4
  .fpu softvfp
  .thumb

.global  g_pfnVectors
.global  Default_Handler

/* start address for the initialization values of the .data section.
defined in linker script */
.word  _sidata
/* start address for the .data section. defined in linker script */
.word  _sdata
/* end address for the .data section. defined in linker script */
.word  _edata
/* start address for the .bss section. defined in linker script */
.word  _sbss
/* end address for the .bss section. defined in linker script */
.word  _ebss
/* stack used for SystemInit_ExtMemCtl; always internal RAM used */

/**
 * @brief  This is the code that gets called when the processor first
 *          starts execution following a reset event. Only the absolutely
 *          necessary set is performed, after which the application
 *          supplied main() routine is called.
 * @param  None
 * @retval : None
*/

    .section  .text.Reset_Handler
  .weak  Reset_Handler
  .type  Reset_Handler, %function
Reset_Handler:
  ldr   sp, =_estack      /* set stack pointer */

/* Call the ExitRun0Mode function to configure the power supply */
  bl  ExitRun0Mode
/* Call the clock system initialization function.*/
  bl  SystemInit

/* Copy the data segment initializers from flash to SRAM */
  ldr r0, =_sdata
  ldr r1, =_edata
  ldr r2, =_sidata
  movs r3, #0
  b LoopCopyDataInit

CopyDataInit:
  ldr r4, [r2, r3]
  str r4, [r0, r3]
  adds r3, r3, #4

LoopCopyDataInit:
  adds r4, r0, r3
  cmp r4, r1
  bcc CopyDataInit
/* Zero fill the bss segment. */
  ldr r2, =_sbss
  ldr r4, =_ebss
  movs r3, #0
  b LoopFillZerobss

FillZerobss:
  str  r3, [r2]
  adds r2, r2, #4

LoopFillZerobss:
  cmp r2, r4
  bcc FillZerobss

/* Call static constructors */
    bl __libc_init_array
/* Call the application's entry point.*/
    bl  main
LoopForever:
    b LoopForever

.size  Reset_Handler, .-Reset_Handler

/**
 * @brief  This is the code that gets called when the processor receives an
 *         unexpected interrupt.  This simply enters an infinite loop, preserving
 *         the system state for examination by a debugger.
 * @param  None
 * @retval None
*/
    .section  .text.Default_Handler,"ax",%progbits
    .type Default_Handler, %function
    .thumb_func
Default_Handler:
Infinite_Loop:
  b  Infinite_Loop
  .size  Default_Handler, .-Default_Handler
/******************************************************************************
*
* The minimal vector table for a Cortex M. Note that the proper constructs
* must be placed on this to ensure that it ends up at physical address
* 0x0000.0000.
*
*******************************************************************************/
   .section  .isr_vector,"a",%progbits
  .type  g_pfnVectors, %object


g_pfnVectors:
  .word  _estack
  .word  Reset_Handler

  .word  NMI_Handler
  .word  HardFault_Handler
  .word  MemManage_Handler
  .word  BusFault_Handler
  .word  UsageFault_Handler
  .word  0
  .word  0
  .word  0
  .word  0
  .word  SVC_Handler
  .word  DebugMon_Handler
  .word  0
  .word  PendSV_Handler
  .word  SysTick_Handler

  /* CubeRacer M4 owns only EXTI4, TIM1 update/compare and HSEM2. Do not
   * reference generic STM32duino handlers for M7-owned peripherals: vector
   * references retain those handlers even when radio startup is disabled.
   * Keep reserved zero slots and route every other IRQ to the local trap.
   */
  .word     Default_Handler                   /* Window WatchDog Interrupt ( wwdg1_it, wwdg2_it) */
  .word     Default_Handler                /* PVD/AVD through EXTI Line detection */
  .word     Default_Handler             /* Tamper and TimeStamps through the EXTI line */
  .word     Default_Handler               /* RTC Wakeup through the EXTI line */
  .word     Default_Handler                  /* FLASH                        */
  .word     Default_Handler                    /* RCC                          */
  .word     Default_Handler                  /* EXTI Line0                   */
  .word     Default_Handler                  /* EXTI Line1                   */
  .word     Default_Handler                  /* EXTI Line2                   */
  .word     Default_Handler                  /* EXTI Line3                   */
  .word     CUBERACER_DIO1_IRQHandler                  /* EXTI Line4                   */
  .word     Default_Handler           /* DMA1 Stream 0                */
  .word     Default_Handler           /* DMA1 Stream 1                */
  .word     Default_Handler           /* DMA1 Stream 2                */
  .word     Default_Handler           /* DMA1 Stream 3                */
  .word     Default_Handler           /* DMA1 Stream 4                */
  .word     Default_Handler           /* DMA1 Stream 5                */
  .word     Default_Handler           /* DMA1 Stream 6                */
  .word     Default_Handler                    /* ADC1, ADC2 and ADC3s         */
  .word     Default_Handler             /* FDCAN1 interrupt line 0      */
  .word     Default_Handler             /* FDCAN2 interrupt line 0      */
  .word     Default_Handler             /* FDCAN1 interrupt line 1      */
  .word     Default_Handler             /* FDCAN2 interrupt line 1      */
  .word     Default_Handler                /* External Line[9:5]s          */
  .word     Default_Handler               /* TIM1 Break interrupt         */
  .word     TIM1_UP_IRQHandler                /* TIM1 Update interrupt        */
  .word     Default_Handler           /* TIM1 Trigger and Commutation interrupt */
  .word     TIM1_CC_IRQHandler                /* TIM1 Capture Compare         */
  .word     Default_Handler                   /* TIM2                         */
  .word     Default_Handler                   /* TIM3                         */
  .word     Default_Handler                   /* TIM4                         */
  .word     Default_Handler                /* I2C1 Event                   */
  .word     Default_Handler                /* I2C1 Error                   */
  .word     Default_Handler                /* I2C2 Event                   */
  .word     Default_Handler                /* I2C2 Error                   */
  .word     Default_Handler                   /* SPI1                         */
  .word     Default_Handler                   /* SPI2                         */
  .word     Default_Handler                 /* USART1                       */
  .word     Default_Handler                 /* USART2                       */
  .word     Default_Handler                 /* USART3                       */
  .word     Default_Handler              /* External Line[15:10]s        */
  .word     Default_Handler              /* RTC Alarm (A and B) through EXTI Line */
  .word     0                                 /* Reserved                     */
  .word     Default_Handler         /* TIM8 Break and TIM12         */
  .word     Default_Handler          /* TIM8 Update and TIM13        */
  .word     Default_Handler     /* TIM8 Trigger and Commutation and TIM14 */
  .word     Default_Handler                /* TIM8 Capture Compare         */
  .word     Default_Handler           /* DMA1 Stream7                 */
  .word     Default_Handler                    /* FMC                          */
  .word     Default_Handler                 /* SDMMC1                       */
  .word     Default_Handler                   /* TIM5                         */
  .word     Default_Handler                   /* SPI3                         */
  .word     Default_Handler                  /* UART4                        */
  .word     Default_Handler                  /* UART5                        */
  .word     Default_Handler               /* TIM6 and DAC1&2 underrun errors */
  .word     Default_Handler                   /* TIM7                         */
  .word     Default_Handler           /* DMA2 Stream 0                */
  .word     Default_Handler           /* DMA2 Stream 1                */
  .word     Default_Handler           /* DMA2 Stream 2                */
  .word     Default_Handler           /* DMA2 Stream 3                */
  .word     Default_Handler           /* DMA2 Stream 4                */
  .word     Default_Handler                    /* Ethernet                     */
  .word     Default_Handler               /* Ethernet Wakeup through EXTI line */
  .word     Default_Handler              /* FDCAN calibration unit interrupt  */
  .word     Default_Handler                /* CM7 Send event interrupt for CM4  */
  .word     Default_Handler                /* CM4 Send event interrupt for CM7  */
  .word     0                                 /* Reserved                     */
  .word     0                                 /* Reserved                     */
  .word     Default_Handler           /* DMA2 Stream 5                */
  .word     Default_Handler           /* DMA2 Stream 6                */
  .word     Default_Handler           /* DMA2 Stream 7                */
  .word     Default_Handler                 /* USART6                       */
  .word     Default_Handler                /* I2C3 event                   */
  .word     Default_Handler                /* I2C3 error                   */
  .word     Default_Handler         /* USB OTG HS End Point 1 Out   */
  .word     Default_Handler          /* USB OTG HS End Point 1 In    */
  .word     Default_Handler            /* USB OTG HS Wakeup through EXTI */
  .word     Default_Handler                 /* USB OTG HS                   */
  .word     Default_Handler                   /* DCMI                         */
  .word     Default_Handler                   /* Crypto                       */
  .word     Default_Handler               /* Hash and Rng                 */
  .word     Default_Handler                    /* FPU                          */
  .word     Default_Handler                  /* UART7                        */
  .word     Default_Handler                  /* UART8                        */
  .word     Default_Handler                   /* SPI4                         */
  .word     Default_Handler                   /* SPI5                         */
  .word     Default_Handler                   /* SPI6                         */
  .word     Default_Handler                   /* SAI1                         */
  .word     Default_Handler                   /* LTDC                         */
  .word     Default_Handler                /* LTDC error                   */
  .word     Default_Handler                  /* DMA2D                        */
  .word     Default_Handler                   /* SAI2                         */
  .word     Default_Handler                /* QUADSPI                      */
  .word     Default_Handler                 /* LPTIM1                       */
  .word     Default_Handler                    /* HDMI_CEC                     */
  .word     Default_Handler                /* I2C4 Event                   */
  .word     Default_Handler                /* I2C4 Error                   */
  .word     Default_Handler               /* SPDIF_RX                     */
  .word     Default_Handler         /* USB OTG FS End Point 1 Out   */
  .word     Default_Handler          /* USB OTG FS End Point 1 In    */
  .word     Default_Handler            /* USB OTG FS Wakeup through EXTI */
  .word     Default_Handler                 /* USB OTG FS                   */
  .word     Default_Handler            /* DMAMUX1 Overrun interrupt    */
  .word     Default_Handler          /* HRTIM Master Timer global Interrupt */
  .word     Default_Handler            /* HRTIM Timer A global Interrupt */
  .word     Default_Handler            /* HRTIM Timer B global Interrupt */
  .word     Default_Handler            /* HRTIM Timer C global Interrupt */
  .word     Default_Handler            /* HRTIM Timer D global Interrupt */
  .word     Default_Handler            /* HRTIM Timer E global Interrupt */
  .word     Default_Handler             /* HRTIM Fault global Interrupt   */
  .word     Default_Handler            /* DFSDM Filter0 Interrupt        */
  .word     Default_Handler            /* DFSDM Filter1 Interrupt        */
  .word     Default_Handler            /* DFSDM Filter2 Interrupt        */
  .word     Default_Handler            /* DFSDM Filter3 Interrupt        */
  .word     Default_Handler                   /* SAI3 global Interrupt          */
  .word     Default_Handler                 /* Serial Wire Interface 1 global interrupt */
  .word     Default_Handler                  /* TIM15 global Interrupt      */
  .word     Default_Handler                  /* TIM16 global Interrupt      */
  .word     Default_Handler                  /* TIM17 global Interrupt      */
  .word     Default_Handler             /* MDIOS Wakeup  Interrupt     */
  .word     Default_Handler                  /* MDIOS global Interrupt      */
  .word     Default_Handler                   /* JPEG global Interrupt       */
  .word     Default_Handler                   /* MDMA global Interrupt       */
  .word     Default_Handler                    /* DSI global Interrupt        */
  .word     Default_Handler                 /* SDMMC2 global Interrupt     */
  .word     Default_Handler                  /* HSEM1 global Interrupt      */
  .word     HSEM2_IRQHandler                  /* HSEM1 global Interrupt      */
  .word     Default_Handler                   /* ADC3 global Interrupt       */
  .word     Default_Handler            /* DMAMUX Overrun interrupt    */
  .word     Default_Handler          /* BDMA Channel 0 global Interrupt */
  .word     Default_Handler          /* BDMA Channel 1 global Interrupt */
  .word     Default_Handler          /* BDMA Channel 2 global Interrupt */
  .word     Default_Handler          /* BDMA Channel 3 global Interrupt */
  .word     Default_Handler          /* BDMA Channel 4 global Interrupt */
  .word     Default_Handler          /* BDMA Channel 5 global Interrupt */
  .word     Default_Handler          /* BDMA Channel 6 global Interrupt */
  .word     Default_Handler          /* BDMA Channel 7 global Interrupt */
  .word     Default_Handler                  /* COMP1 global Interrupt     */
  .word     Default_Handler                 /* LP TIM2 global interrupt   */
  .word     Default_Handler                 /* LP TIM3 global interrupt   */
  .word     Default_Handler                 /* LP TIM4 global interrupt   */
  .word     Default_Handler                 /* LP TIM5 global interrupt   */
  .word     Default_Handler                /* LP UART1 interrupt         */
  .word     Default_Handler               /* Window Watchdog reset interrupt (exti_d2_wwdg_it, exti_d1_wwdg_it) */
  .word     Default_Handler                    /* Clock Recovery Global Interrupt */
  .word     Default_Handler                    /* ECC diagnostic Global Interrupt */
  .word     Default_Handler                   /* SAI4 global interrupt      */
  .word     0                                 /* Reserved                   */
  .word     Default_Handler              /* Hold core interrupt        */
  .word     Default_Handler             /* Interrupt for all 6 wake-up pins */

  .size  g_pfnVectors, .-g_pfnVectors

/*******************************************************************************
*
* Provide weak aliases for each Exception handler to the Default_Handler.
* As they are weak aliases, any function with the same name will override
* this definition.
*
*******************************************************************************/
   .weak      NMI_Handler
   .thumb_set NMI_Handler,Default_Handler

   .weak      HardFault_Handler
   .thumb_set HardFault_Handler,Default_Handler

   .weak      MemManage_Handler
   .thumb_set MemManage_Handler,Default_Handler

   .weak      BusFault_Handler
   .thumb_set BusFault_Handler,Default_Handler

   .weak      UsageFault_Handler
   .thumb_set UsageFault_Handler,Default_Handler

   .weak      SVC_Handler
   .thumb_set SVC_Handler,Default_Handler

   .weak      DebugMon_Handler
   .thumb_set DebugMon_Handler,Default_Handler

   .weak      PendSV_Handler
   .thumb_set PendSV_Handler,Default_Handler

   .weak      SysTick_Handler
   .thumb_set SysTick_Handler,Default_Handler

   .weak      WWDG_IRQHandler
   .thumb_set WWDG_IRQHandler,Default_Handler

   .weak      PVD_AVD_IRQHandler
   .thumb_set PVD_AVD_IRQHandler,Default_Handler

   .weak      TAMP_STAMP_IRQHandler
   .thumb_set TAMP_STAMP_IRQHandler,Default_Handler

   .weak      RTC_WKUP_IRQHandler
   .thumb_set RTC_WKUP_IRQHandler,Default_Handler

   .weak      FLASH_IRQHandler
   .thumb_set FLASH_IRQHandler,Default_Handler

   .weak      RCC_IRQHandler
   .thumb_set RCC_IRQHandler,Default_Handler

   .weak      EXTI0_IRQHandler
   .thumb_set EXTI0_IRQHandler,Default_Handler

   .weak      EXTI1_IRQHandler
   .thumb_set EXTI1_IRQHandler,Default_Handler

   .weak      EXTI2_IRQHandler
   .thumb_set EXTI2_IRQHandler,Default_Handler

   .weak      EXTI3_IRQHandler
   .thumb_set EXTI3_IRQHandler,Default_Handler

   .weak      CUBERACER_DIO1_IRQHandler
   .thumb_set CUBERACER_DIO1_IRQHandler,Default_Handler

   .weak      DMA1_Stream0_IRQHandler
   .thumb_set DMA1_Stream0_IRQHandler,Default_Handler

   .weak      DMA1_Stream1_IRQHandler
   .thumb_set DMA1_Stream1_IRQHandler,Default_Handler

   .weak      DMA1_Stream2_IRQHandler
   .thumb_set DMA1_Stream2_IRQHandler,Default_Handler

   .weak      DMA1_Stream3_IRQHandler
   .thumb_set DMA1_Stream3_IRQHandler,Default_Handler

   .weak      DMA1_Stream4_IRQHandler
   .thumb_set DMA1_Stream4_IRQHandler,Default_Handler

   .weak      DMA1_Stream5_IRQHandler
   .thumb_set DMA1_Stream5_IRQHandler,Default_Handler

   .weak      DMA1_Stream6_IRQHandler
   .thumb_set DMA1_Stream6_IRQHandler,Default_Handler

   .weak      ADC_IRQHandler
   .thumb_set ADC_IRQHandler,Default_Handler

   .weak      FDCAN1_IT0_IRQHandler
   .thumb_set FDCAN1_IT0_IRQHandler,Default_Handler

   .weak      FDCAN2_IT0_IRQHandler
   .thumb_set FDCAN2_IT0_IRQHandler,Default_Handler

   .weak      FDCAN1_IT1_IRQHandler
   .thumb_set FDCAN1_IT1_IRQHandler,Default_Handler

   .weak      FDCAN2_IT1_IRQHandler
   .thumb_set FDCAN2_IT1_IRQHandler,Default_Handler

   .weak      EXTI9_5_IRQHandler
   .thumb_set EXTI9_5_IRQHandler,Default_Handler

   .weak      TIM1_BRK_IRQHandler
   .thumb_set TIM1_BRK_IRQHandler,Default_Handler

   .weak      TIM1_UP_IRQHandler
   .thumb_set TIM1_UP_IRQHandler,Default_Handler

   .weak      TIM1_TRG_COM_IRQHandler
   .thumb_set TIM1_TRG_COM_IRQHandler,Default_Handler

   .weak      TIM1_CC_IRQHandler
   .thumb_set TIM1_CC_IRQHandler,Default_Handler

   .weak      TIM2_IRQHandler
   .thumb_set TIM2_IRQHandler,Default_Handler

   .weak      TIM3_IRQHandler
   .thumb_set TIM3_IRQHandler,Default_Handler

   .weak      TIM4_IRQHandler
   .thumb_set TIM4_IRQHandler,Default_Handler

   .weak      I2C1_EV_IRQHandler
   .thumb_set I2C1_EV_IRQHandler,Default_Handler

   .weak      I2C1_ER_IRQHandler
   .thumb_set I2C1_ER_IRQHandler,Default_Handler

   .weak      I2C2_EV_IRQHandler
   .thumb_set I2C2_EV_IRQHandler,Default_Handler

   .weak      I2C2_ER_IRQHandler
   .thumb_set I2C2_ER_IRQHandler,Default_Handler

   .weak      SPI1_IRQHandler
   .thumb_set SPI1_IRQHandler,Default_Handler

   .weak      SPI2_IRQHandler
   .thumb_set SPI2_IRQHandler,Default_Handler

   .weak      USART1_IRQHandler
   .thumb_set USART1_IRQHandler,Default_Handler

   .weak      USART2_IRQHandler
   .thumb_set USART2_IRQHandler,Default_Handler

   .weak      USART3_IRQHandler
   .thumb_set USART3_IRQHandler,Default_Handler

   .weak      EXTI15_10_IRQHandler
   .thumb_set EXTI15_10_IRQHandler,Default_Handler

   .weak      RTC_Alarm_IRQHandler
   .thumb_set RTC_Alarm_IRQHandler,Default_Handler

   .weak      TIM8_BRK_TIM12_IRQHandler
   .thumb_set TIM8_BRK_TIM12_IRQHandler,Default_Handler

   .weak      TIM8_UP_TIM13_IRQHandler
   .thumb_set TIM8_UP_TIM13_IRQHandler,Default_Handler

   .weak      TIM8_TRG_COM_TIM14_IRQHandler
   .thumb_set TIM8_TRG_COM_TIM14_IRQHandler,Default_Handler

   .weak      TIM8_CC_IRQHandler
   .thumb_set TIM8_CC_IRQHandler,Default_Handler

   .weak      DMA1_Stream7_IRQHandler
   .thumb_set DMA1_Stream7_IRQHandler,Default_Handler

   .weak      FMC_IRQHandler
   .thumb_set FMC_IRQHandler,Default_Handler

   .weak      SDMMC1_IRQHandler
   .thumb_set SDMMC1_IRQHandler,Default_Handler

   .weak      TIM5_IRQHandler
   .thumb_set TIM5_IRQHandler,Default_Handler

   .weak      SPI3_IRQHandler
   .thumb_set SPI3_IRQHandler,Default_Handler

   .weak      UART4_IRQHandler
   .thumb_set UART4_IRQHandler,Default_Handler

   .weak      UART5_IRQHandler
   .thumb_set UART5_IRQHandler,Default_Handler

   .weak      TIM6_DAC_IRQHandler
   .thumb_set TIM6_DAC_IRQHandler,Default_Handler

   .weak      TIM7_IRQHandler
   .thumb_set TIM7_IRQHandler,Default_Handler

   .weak      DMA2_Stream0_IRQHandler
   .thumb_set DMA2_Stream0_IRQHandler,Default_Handler

   .weak      DMA2_Stream1_IRQHandler
   .thumb_set DMA2_Stream1_IRQHandler,Default_Handler

   .weak      DMA2_Stream2_IRQHandler
   .thumb_set DMA2_Stream2_IRQHandler,Default_Handler

   .weak      DMA2_Stream3_IRQHandler
   .thumb_set DMA2_Stream3_IRQHandler,Default_Handler

   .weak      DMA2_Stream4_IRQHandler
   .thumb_set DMA2_Stream4_IRQHandler,Default_Handler

   .weak      ETH_IRQHandler
   .thumb_set ETH_IRQHandler,Default_Handler

   .weak      ETH_WKUP_IRQHandler
   .thumb_set ETH_WKUP_IRQHandler,Default_Handler

   .weak      FDCAN_CAL_IRQHandler
   .thumb_set FDCAN_CAL_IRQHandler,Default_Handler

   .weak      CM7_SEV_IRQHandler
   .thumb_set CM7_SEV_IRQHandler,Default_Handler

   .weak      CM4_SEV_IRQHandler
   .thumb_set CM4_SEV_IRQHandler,Default_Handler

   .weak      DMA2_Stream5_IRQHandler
   .thumb_set DMA2_Stream5_IRQHandler,Default_Handler

   .weak      DMA2_Stream6_IRQHandler
   .thumb_set DMA2_Stream6_IRQHandler,Default_Handler

   .weak      DMA2_Stream7_IRQHandler
   .thumb_set DMA2_Stream7_IRQHandler,Default_Handler

   .weak      USART6_IRQHandler
   .thumb_set USART6_IRQHandler,Default_Handler

   .weak      I2C3_EV_IRQHandler
   .thumb_set I2C3_EV_IRQHandler,Default_Handler

   .weak      I2C3_ER_IRQHandler
   .thumb_set I2C3_ER_IRQHandler,Default_Handler

   .weak      OTG_HS_EP1_OUT_IRQHandler
   .thumb_set OTG_HS_EP1_OUT_IRQHandler,Default_Handler

   .weak      OTG_HS_EP1_IN_IRQHandler
   .thumb_set OTG_HS_EP1_IN_IRQHandler,Default_Handler

   .weak      OTG_HS_WKUP_IRQHandler
   .thumb_set OTG_HS_WKUP_IRQHandler,Default_Handler

   .weak      OTG_HS_IRQHandler
   .thumb_set OTG_HS_IRQHandler,Default_Handler

   .weak      DCMI_IRQHandler
   .thumb_set DCMI_IRQHandler,Default_Handler

   .weak      CRYP_IRQHandler
   .thumb_set CRYP_IRQHandler,Default_Handler

   .weak      HASH_RNG_IRQHandler
   .thumb_set HASH_RNG_IRQHandler,Default_Handler

   .weak      FPU_IRQHandler
   .thumb_set FPU_IRQHandler,Default_Handler

   .weak      UART7_IRQHandler
   .thumb_set UART7_IRQHandler,Default_Handler

   .weak      UART8_IRQHandler
   .thumb_set UART8_IRQHandler,Default_Handler

   .weak      SPI4_IRQHandler
   .thumb_set SPI4_IRQHandler,Default_Handler

   .weak      SPI5_IRQHandler
   .thumb_set SPI5_IRQHandler,Default_Handler

   .weak      SPI6_IRQHandler
   .thumb_set SPI6_IRQHandler,Default_Handler

   .weak      SAI1_IRQHandler
   .thumb_set SAI1_IRQHandler,Default_Handler

   .weak      LTDC_IRQHandler
   .thumb_set LTDC_IRQHandler,Default_Handler

   .weak      LTDC_ER_IRQHandler
   .thumb_set LTDC_ER_IRQHandler,Default_Handler

   .weak      DMA2D_IRQHandler
   .thumb_set DMA2D_IRQHandler,Default_Handler

   .weak      SAI2_IRQHandler
   .thumb_set SAI2_IRQHandler,Default_Handler

   .weak      QUADSPI_IRQHandler
   .thumb_set QUADSPI_IRQHandler,Default_Handler

   .weak      LPTIM1_IRQHandler
   .thumb_set LPTIM1_IRQHandler,Default_Handler

   .weak      CEC_IRQHandler
   .thumb_set CEC_IRQHandler,Default_Handler

   .weak      I2C4_EV_IRQHandler
   .thumb_set I2C4_EV_IRQHandler,Default_Handler

   .weak      I2C4_ER_IRQHandler
   .thumb_set I2C4_ER_IRQHandler,Default_Handler

   .weak      SPDIF_RX_IRQHandler
   .thumb_set SPDIF_RX_IRQHandler,Default_Handler

   .weak      OTG_FS_EP1_OUT_IRQHandler
   .thumb_set OTG_FS_EP1_OUT_IRQHandler,Default_Handler

   .weak      OTG_FS_EP1_IN_IRQHandler
   .thumb_set OTG_FS_EP1_IN_IRQHandler,Default_Handler

   .weak      OTG_FS_WKUP_IRQHandler
   .thumb_set OTG_FS_WKUP_IRQHandler,Default_Handler

   .weak      OTG_FS_IRQHandler
   .thumb_set OTG_FS_IRQHandler,Default_Handler

   .weak      DMAMUX1_OVR_IRQHandler
   .thumb_set DMAMUX1_OVR_IRQHandler,Default_Handler

   .weak      HRTIM1_Master_IRQHandler
   .thumb_set HRTIM1_Master_IRQHandler,Default_Handler

   .weak      HRTIM1_TIMA_IRQHandler
   .thumb_set HRTIM1_TIMA_IRQHandler,Default_Handler

   .weak      HRTIM1_TIMB_IRQHandler
   .thumb_set HRTIM1_TIMB_IRQHandler,Default_Handler

   .weak      HRTIM1_TIMC_IRQHandler
   .thumb_set HRTIM1_TIMC_IRQHandler,Default_Handler

   .weak      HRTIM1_TIMD_IRQHandler
   .thumb_set HRTIM1_TIMD_IRQHandler,Default_Handler

   .weak      HRTIM1_TIME_IRQHandler
   .thumb_set HRTIM1_TIME_IRQHandler,Default_Handler

   .weak      HRTIM1_FLT_IRQHandler
   .thumb_set HRTIM1_FLT_IRQHandler,Default_Handler

   .weak      DFSDM1_FLT0_IRQHandler
   .thumb_set DFSDM1_FLT0_IRQHandler,Default_Handler

   .weak      DFSDM1_FLT1_IRQHandler
   .thumb_set DFSDM1_FLT1_IRQHandler,Default_Handler

   .weak      DFSDM1_FLT2_IRQHandler
   .thumb_set DFSDM1_FLT2_IRQHandler,Default_Handler

   .weak      DFSDM1_FLT3_IRQHandler
   .thumb_set DFSDM1_FLT3_IRQHandler,Default_Handler

   .weak      SAI3_IRQHandler
   .thumb_set SAI3_IRQHandler,Default_Handler

   .weak      SWPMI1_IRQHandler
   .thumb_set SWPMI1_IRQHandler,Default_Handler

   .weak      TIM15_IRQHandler
   .thumb_set TIM15_IRQHandler,Default_Handler

   .weak      TIM16_IRQHandler
   .thumb_set TIM16_IRQHandler,Default_Handler

   .weak      TIM17_IRQHandler
   .thumb_set TIM17_IRQHandler,Default_Handler

   .weak      MDIOS_WKUP_IRQHandler
   .thumb_set MDIOS_WKUP_IRQHandler,Default_Handler

   .weak      MDIOS_IRQHandler
   .thumb_set MDIOS_IRQHandler,Default_Handler

   .weak      JPEG_IRQHandler
   .thumb_set JPEG_IRQHandler,Default_Handler

   .weak      MDMA_IRQHandler
   .thumb_set MDMA_IRQHandler,Default_Handler

   .weak      DSI_IRQHandler
   .thumb_set DSI_IRQHandler,Default_Handler

   .weak      SDMMC2_IRQHandler
   .thumb_set SDMMC2_IRQHandler,Default_Handler

   .weak      HSEM1_IRQHandler
   .thumb_set HSEM1_IRQHandler,Default_Handler

   .weak      HSEM2_IRQHandler
   .thumb_set HSEM2_IRQHandler,Default_Handler

   .weak      ADC3_IRQHandler
   .thumb_set ADC3_IRQHandler,Default_Handler

   .weak      DMAMUX2_OVR_IRQHandler
   .thumb_set DMAMUX2_OVR_IRQHandler,Default_Handler

   .weak      BDMA_Channel0_IRQHandler
   .thumb_set BDMA_Channel0_IRQHandler,Default_Handler

   .weak      BDMA_Channel1_IRQHandler
   .thumb_set BDMA_Channel1_IRQHandler,Default_Handler

   .weak      BDMA_Channel2_IRQHandler
   .thumb_set BDMA_Channel2_IRQHandler,Default_Handler

   .weak      BDMA_Channel3_IRQHandler
   .thumb_set BDMA_Channel3_IRQHandler,Default_Handler

   .weak      BDMA_Channel4_IRQHandler
   .thumb_set BDMA_Channel4_IRQHandler,Default_Handler

   .weak      BDMA_Channel5_IRQHandler
   .thumb_set BDMA_Channel5_IRQHandler,Default_Handler

   .weak      BDMA_Channel6_IRQHandler
   .thumb_set BDMA_Channel6_IRQHandler,Default_Handler

   .weak      BDMA_Channel7_IRQHandler
   .thumb_set BDMA_Channel7_IRQHandler,Default_Handler

   .weak      COMP1_IRQHandler
   .thumb_set COMP1_IRQHandler,Default_Handler

   .weak      LPTIM2_IRQHandler
   .thumb_set LPTIM2_IRQHandler,Default_Handler

   .weak      LPTIM3_IRQHandler
   .thumb_set LPTIM3_IRQHandler,Default_Handler

   .weak      LPTIM4_IRQHandler
   .thumb_set LPTIM4_IRQHandler,Default_Handler

   .weak      LPTIM5_IRQHandler
   .thumb_set LPTIM5_IRQHandler,Default_Handler

   .weak      LPUART1_IRQHandler
   .thumb_set LPUART1_IRQHandler,Default_Handler

   .weak      WWDG_RST_IRQHandler
   .thumb_set WWDG_RST_IRQHandler,Default_Handler

   .weak      CRS_IRQHandler
   .thumb_set CRS_IRQHandler,Default_Handler

   .weak      ECC_IRQHandler
   .thumb_set ECC_IRQHandler,Default_Handler

   .weak      SAI4_IRQHandler
   .thumb_set SAI4_IRQHandler,Default_Handler

   .weak      HOLD_CORE_IRQHandler
   .thumb_set HOLD_CORE_IRQHandler,Default_Handler

   .weak      WAKEUP_PIN_IRQHandler
   .thumb_set WAKEUP_PIN_IRQHandler,Default_Handler


