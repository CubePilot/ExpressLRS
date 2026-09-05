#pragma once
#include "../STM32H757/variant_generic.h"
// PB0 and PA5 are motor outputs owned by Betaflight. Never use timing probes.
#define DBG_PIN_INIT() ((void)0)
#define DBG_PIN_HIGH() ((void)0)
#define DBG_PIN_LOW() ((void)0)
#define DBG_PIN_TOGGLE() ((void)0)
#define DBG2_PIN_INIT() ((void)0)
#define DBG2_PIN_HIGH() ((void)0)
#define DBG2_PIN_LOW() ((void)0)
#define DBG2_PIN_TOGGLE() ((void)0)
