#pragma once
#include "targets.h"
#undef GPIO_PIN_LED_RED
#undef GPIO_PIN_LED_GREEN
#undef GPIO_PIN_LED_BLUE
#undef GPIO_LED_RED_INVERTED
#undef GPIO_LED_GREEN_INVERTED
#undef GPIO_LED_BLUE_INVERTED
#define GPIO_PIN_LED_RED 91
#define GPIO_PIN_LED_GREEN UNDEF_PIN
#define GPIO_PIN_LED_BLUE UNDEF_PIN
#define GPIO_LED_RED_INVERTED false
#define GPIO_LED_GREEN_INVERTED false
#define GPIO_LED_BLUE_INVERTED false
#define LOW 0
#define HIGH 1
#define OUTPUT 1
extern unsigned genericWrites;
inline void pinMode(int,int) { genericWrites++; }
void digitalWrite(int pin,int value);
