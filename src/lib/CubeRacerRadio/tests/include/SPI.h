#pragma once
#include <cstdint>

enum spi_status_e
{
    SPI_OK,
    SPI_ERROR,
    SPI_TIMEOUT
};

enum
{
    HAL_SPI_STATE_RESET,
    HAL_SPI_STATE_READY
};

struct SPI_HandleTypeDef
{
    unsigned State = HAL_SPI_STATE_READY;
};

struct spi_t
{
    SPI_HandleTypeDef handle;
};

class SPIClass
{
protected:
    spi_t _spi;

public:
    SPI_HandleTypeDef *getHandle() { return &_spi.handle; }
};

spi_status_e spi_transfer(spi_t *, const uint8_t *, uint8_t *, uint16_t);
uint32_t __get_PRIMASK();
void __disable_irq();
void __set_PRIMASK(uint32_t);
