/*
 *******************************************************************************
 * Copyright (c) 2020-2021, STMicroelectronics
 * All rights reserved.
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 *******************************************************************************
 */
#if defined(ARDUINO_GENERIC_H742ZGTX) || defined(ARDUINO_GENERIC_H742ZITX) ||\
    defined(ARDUINO_GENERIC_H743ZGTX) || defined(ARDUINO_GENERIC_H743ZITX) ||\
    defined(ARDUINO_GENERIC_H747AGIX) || defined(ARDUINO_GENERIC_H747AIIX) ||\
    defined(ARDUINO_GENERIC_H747IGTX) || defined(ARDUINO_GENERIC_H747IITX) ||\
    defined(ARDUINO_GENERIC_H750ZBTX) || defined(ARDUINO_GENERIC_H753ZITX) ||\
    defined(ARDUINO_GENERIC_H757AIIX) || defined(ARDUINO_GENERIC_H757IITX) ||\
    defined(ARDUINO_CUBENODE)
#include "pins_arduino.h"

/**
  * @brief  System Clock Configuration
  * @param  None
  * @retval None
  */
/*
  Clock config matching ArduPilot/ChibiOS CubeNode (STM32H757, 24MHz HSE):
  - PLL1: HSE(24) /3 *100 = 800MHz VCO -> /2 = 400MHz SYSCLK, /10 = 80MHz PLL1Q
  - PLL2: HSE(24) /2 *50 = 600MHz VCO -> /3 = 200MHz, /6 = 100MHz, /3 = 200MHz
  - PLL3: HSE(24) /6 *72 = 288MHz VCO -> /6 = 48MHz (USB), /9 = 32MHz (I2C/ADC)
  - AHB = 200MHz, APBx = 100MHz, VOS Scale1, Flash latency 2
*/
WEAK void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {};

  HAL_PWREx_ConfigSupply(PWR_DIRECT_SMPS_SUPPLY);
  /* Enable USB 3.3V supply (matches ChibiOS: PWR_CR3_SMPSEN | PWR_CR3_USB33DEN) */
  HAL_PWREx_EnableUSBVoltageDetector();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /* Enable HSE 24MHz crystal first */
  __HAL_RCC_HSE_CONFIG(RCC_HSE_ON);
  while (!__HAL_RCC_GET_FLAG(RCC_FLAG_HSERDY)) {}

  /* HSE + HSI48 for RNG */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_HSI48;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  /* PLL1: 24/3*100 = 800MHz VCO */
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 3;
  RCC_OscInitStruct.PLL.PLLN = 100;
  RCC_OscInitStruct.PLL.PLLP = 2;   /* 400MHz SYSCLK */
  RCC_OscInitStruct.PLL.PLLQ = 10;  /* 80MHz */
  RCC_OscInitStruct.PLL.PLLR = 2;   /* 400MHz */
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;  /* 8-16MHz VCI range (VCI=8MHz) */
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /* SYSCLK=400MHz, AHB=200MHz, APBx=100MHz */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2
                                | RCC_CLOCKTYPE_D3PCLK1 | RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
    Error_Handler();
  }

  /* Peripheral clocks */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USB | RCC_PERIPHCLK_QSPI
                                             | RCC_PERIPHCLK_SDMMC | RCC_PERIPHCLK_ADC
                                             | RCC_PERIPHCLK_LPUART1 | RCC_PERIPHCLK_USART16
                                             | RCC_PERIPHCLK_USART234578 | RCC_PERIPHCLK_I2C123
                                             | RCC_PERIPHCLK_I2C4 | RCC_PERIPHCLK_SPI123
                                             | RCC_PERIPHCLK_SPI45 | RCC_PERIPHCLK_SPI6;
  /* PLL2: 24/2*50 = 600MHz VCO */
  PeriphClkInitStruct.PLL2.PLL2M = 2;
  PeriphClkInitStruct.PLL2.PLL2N = 50;
  PeriphClkInitStruct.PLL2.PLL2P = 3;   /* 200MHz */
  PeriphClkInitStruct.PLL2.PLL2Q = 6;   /* 100MHz (USART, SPI) */
  PeriphClkInitStruct.PLL2.PLL2R = 3;   /* 200MHz (QSPI) */
  PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_3;  /* 8-16MHz */
  PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
  PeriphClkInitStruct.PLL2.PLL2FRACN = 0;
  /* PLL3: 24/6*72 = 288MHz VCO */
  PeriphClkInitStruct.PLL3.PLL3M = 6;
  PeriphClkInitStruct.PLL3.PLL3N = 72;
  PeriphClkInitStruct.PLL3.PLL3P = 8;   /* 36MHz (not used) */
  PeriphClkInitStruct.PLL3.PLL3Q = 6;   /* 48MHz (USB) */
  PeriphClkInitStruct.PLL3.PLL3R = 9;   /* 32MHz (I2C, ADC) */
  PeriphClkInitStruct.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_2;  /* 4-8MHz (VCI=4MHz) */
  PeriphClkInitStruct.PLL3.PLL3VCOSEL = RCC_PLL3VCOWIDE;
  PeriphClkInitStruct.PLL3.PLL3FRACN = 0;
  PeriphClkInitStruct.UsbClockSelection = RCC_USBCLKSOURCE_PLL3;
  PeriphClkInitStruct.AdcClockSelection = RCC_ADCCLKSOURCE_PLL3;
  PeriphClkInitStruct.QspiClockSelection = RCC_QSPICLKSOURCE_PLL2;
  PeriphClkInitStruct.SdmmcClockSelection = RCC_SDMMCCLKSOURCE_PLL;
  PeriphClkInitStruct.Lpuart1ClockSelection = RCC_LPUART1CLKSOURCE_D3PCLK1;
  PeriphClkInitStruct.Usart16ClockSelection = RCC_USART16CLKSOURCE_PLL2;
  PeriphClkInitStruct.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_PLL2;
  PeriphClkInitStruct.I2c123ClockSelection = RCC_I2C123CLKSOURCE_PLL3;
  PeriphClkInitStruct.I2c4ClockSelection = RCC_I2C4CLKSOURCE_PLL3;
  PeriphClkInitStruct.Spi123ClockSelection = RCC_SPI123CLKSOURCE_PLL;
  PeriphClkInitStruct.Spi45ClockSelection = RCC_SPI45CLKSOURCE_PLL2;
  PeriphClkInitStruct.Spi6ClockSelection = RCC_SPI6CLKSOURCE_PLL2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
    Error_Handler();
  }
}

#endif /* ARDUINO_GENERIC_* */
