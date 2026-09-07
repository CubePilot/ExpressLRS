#pragma once
#include <cstdint>

namespace CubeRacer
{
enum class RadioFault
{
    None,
    SpiTimeout,
    SpiError
};
}

// M7 prepares shared pins, clocks and power before booting M4.
// M4 owns SPI4/TIM1 and the radio GPIOs until reset.
bool cuberacerRadioInit(void (*dio1)());
void cuberacerRadioEnd();
bool cuberacerRadioQuiescent();
void cuberacerRadioTimerEnable();
bool cuberacerRadioButtonRead(uint8_t pin, bool &high);
CubeRacer::RadioFault cuberacerRadioFault();

// Timer callbacks run only while the initialized radio is healthy.
bool cuberacerRadioTimerReady();
