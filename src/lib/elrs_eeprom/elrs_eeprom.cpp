#include "elrs_eeprom.h"
#include "targets.h"
#include "logging.h"

// Integrated M4 deliberately has no local persistent-storage implementation.
#if !defined(TARGET_NATIVE) && !defined(CUBERACER_M4)
#include <EEPROM.h>

void
ELRS_EEPROM::Begin()
{
#if defined(PLATFORM_STM32)
    EEPROM.begin();
#else
    EEPROM.begin(RESERVED_EEPROM_SIZE);
#endif
}

uint8_t
ELRS_EEPROM::ReadByte(const uint32_t address)
{
    if (address >= RESERVED_EEPROM_SIZE)
    {
        // address is out of bounds
        ERRLN("EEPROM address is out of bounds");
        return 0;
    }
    return EEPROM.read(address);
}

void
ELRS_EEPROM::WriteByte(const uint32_t address, const uint8_t value)
{
    if (address >= RESERVED_EEPROM_SIZE)
    {
        // address is out of bounds
        ERRLN("EEPROM address is out of bounds");
        return;
    }
#if defined(PLATFORM_STM32)
    // Use buffered write to avoid flash erase+reprogram per byte
    eeprom_buffered_write_byte(address, value);
#else
    EEPROM.write(address, value);
#endif
}

void
ELRS_EEPROM::Commit()
{
#if defined(PLATFORM_STM32)
    eeprom_buffer_flush();
#else
    if (!EEPROM.commit())
    {
      ERRLN("EEPROM commit failed");
    }
#endif
}

#endif /* !TARGET_NATIVE */
#if defined(CUBERACER_M4)
// Checkpoint A has no persistence adapter yet. Fail closed if a legacy callback
// reaches this API; never erase M7 configuration or pretend a save succeeded.
void ELRS_EEPROM::Begin() { __builtin_trap(); }
uint8_t ELRS_EEPROM::ReadByte(uint32_t) { __builtin_trap(); }
void ELRS_EEPROM::WriteByte(uint32_t, uint8_t) { __builtin_trap(); }
void ELRS_EEPROM::Commit() { __builtin_trap(); }
#endif
