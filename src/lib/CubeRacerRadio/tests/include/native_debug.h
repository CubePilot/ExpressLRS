#pragma once
#define DBG_PIN_TOGGLE() ((void)0)
#define DBG2_PIN_TOGGLE() ((void)0)

#include "native.h"
#undef GPIO_PIN_BUSY
#define GPIO_PIN_BUSY 67
#define GPIO_PIN_BUSY_2 UNDEF_PIN
#define LOW 0
int digitalRead(int pin);

#define HIGH 1
#define OUTPUT 1
#define GPIO_PIN_RST 83
#define GPIO_PIN_RST_2 UNDEF_PIN
#define GPIO_PIN_PA_ENABLE UNDEF_PIN
#define GPIO_PIN_RX_ENABLE UNDEF_PIN
#define GPIO_PIN_RX_ENABLE_2 UNDEF_PIN
#define GPIO_PIN_TX_ENABLE UNDEF_PIN
#define GPIO_PIN_TX_ENABLE_2 UNDEF_PIN
void digitalWrite(int pin,int value);
void pinMode(int pin,int mode);
