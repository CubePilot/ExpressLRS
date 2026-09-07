#pragma once
#include <cstdint>
namespace CubeRacer { enum class RadioFault { None, NoGrant, SpiTimeout, SpiError }; }
// The M4 lifecycle accepts M7 grants. PE10 power stays exclusively with M7;
// SPIEx/Arduino supply SPI and GPIO access after the grant.
bool cuberacerRadioGrant(uint32_t session);
bool cuberacerRadioInit(void (*dio1)());
void cuberacerRadioEnd();
bool cuberacerRadioQuiescent();
void cuberacerRadioTimerEnable();
bool cuberacerRadioTransfer(uint8_t *data,unsigned size);
bool cuberacerRadioButtonRead(uint8_t pin,bool &high);
CubeRacer::RadioFault cuberacerRadioFault();

// Guards for STM32duino timer setup; neither function writes shared clocks.
bool cuberacerRadioTimerReady();
extern "C" int cuberacerTimerClockAllowed(uintptr_t timer);
