#include "elrs_eeprom.h"
#include "targets.h"
#include "logging.h"

#if !defined(TARGET_NATIVE)
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