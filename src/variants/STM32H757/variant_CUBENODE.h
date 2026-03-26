#define USE_PWR_DIRECT_SMPS_SUPPLY
#include "variant_generic.h"

/* Debug timing pin PB0 - general ISR steps */
#define DBG_PIN PB_0
#define DBG_PIN_PORT GPIOB
#define DBG_PIN_INIT()   do { pinMode(DBG_PIN, OUTPUT); digitalWrite(DBG_PIN, LOW); } while(0)
#define DBG_PIN_HIGH()   digitalWrite(DBG_PIN, HIGH)
#define DBG_PIN_LOW()    digitalWrite(DBG_PIN, LOW)
#define DBG_PIN_TOGGLE() digitalToggle(DBG_PIN)

/* Debug timing pin PA5 - GetIrqStatus marker */
#define DBG2_PIN PA_5
#define DBG2_PIN_PORT GPIOA
#define DBG2_PIN_INIT()   do { pinMode(DBG2_PIN, OUTPUT); digitalWrite(DBG2_PIN, LOW); } while(0)
#define DBG2_PIN_HIGH()   digitalWrite(DBG2_PIN, HIGH)
#define DBG2_PIN_LOW()    digitalWrite(DBG2_PIN, LOW)
#define DBG2_PIN_TOGGLE() digitalToggle(DBG2_PIN)
