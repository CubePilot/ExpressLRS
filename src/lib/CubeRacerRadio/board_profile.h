#pragma once
#include <stdint.h>

namespace CubeRacer
{
// Canonical digital indices in the pinned STM32H757 variant. PA4/PF3/PF13
// Arduino macros are analog aliases (>=192), unsuitable for int8_t LED pins.
constexpr uint8_t NSS_PIN = 20, BUSY_PIN = 67, DIO1_PIN = 4, RESET_PIN = 83;
constexpr uint8_t SCK_PIN = 64, MISO_PIN = 75, MOSI_PIN = 76, ANTENNA_PIN = 68;
constexpr uint8_t BUTTON_PIN = 81, LED_PIN = 91;
constexpr int8_t MAX_OUTPUT_DBM = 10;

template <class Options>
void bootOptions(Options &options)
{
    options = {}; // No appended/user-flashed UID or standalone persistence.
    options.wifi_auto_on_interval = -1;
    options.uart_baud = 420000;
#ifdef Regulatory_Domain_EU_CE_2400
    options.domain = 1;
#endif
}
}
