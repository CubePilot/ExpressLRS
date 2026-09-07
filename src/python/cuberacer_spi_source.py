"""Keep STM32duino SPI setup/transfers; adapt shared writes and polling for M4."""
import re


def adapt_spi_source(source):
    def replace(old, new):
        nonlocal source
        if source.count(old) != 1:
            raise ValueError('Pinned STM32 SPI source changed: ' + old)
        source = source.replace(old, new)

    # M7 has already configured the shared GPIO banks and peripheral clocks.
    source = re.sub(r'^\s*(?:pinmap_pinout|pin_PullConfig)\([^;]+;', '', source, flags=re.M)
    source = re.sub(r'^\s*__HAL_RCC_SPI\d+_(?:CLK_ENABLE|CLK_DISABLE|FORCE_RESET|RELEASE_RESET)\(\);', '', source, flags=re.M)
    replace('    uint32_t pull = 0;', '')
    source = re.sub(r'^\s*pull = [^;]+;', '', source, flags=re.M)
    replace('      /* Configure SPI GPIO pins */', '      if (obj->spi != SPI4) return; // Only the granted radio peripheral.')

    helper = '''static spi_status_e spi_wait_flag(SPI_TypeDef *spi, uint32_t flag, uint32_t start)
{
  for (unsigned polls = 0; polls < 100000; ++polls) {
    const uint32_t status = spi->SR;
    if (status & (SPI_SR_UDR | SPI_SR_OVR | SPI_SR_CRCE | SPI_SR_TIFRE | SPI_SR_MODF)) return SPI_ERROR;
    if (status & flag) return SPI_OK;
    if ((uint32_t)(micros() - start) >= 1000) break;
  }
  return SPI_TIMEOUT;
}

'''
    replace('spi_status_e spi_transfer(', helper + 'spi_status_e spi_transfer(')
    replace('tickstart = HAL_GetTick();', 'tickstart = micros();')
    for flag in ('TXP', 'RXP'):
        replace('while (!LL_SPI_IsActiveFlag_' + flag + '(_SPI));',
                'if ((ret = spi_wait_flag(_SPI, SPI_SR_' + flag + ', tickstart)) != SPI_OK) goto transfer_done;')
    replace('''      if ((SPI_TRANSFER_TIMEOUT != HAL_MAX_DELAY) &&
          (HAL_GetTick() - tickstart >= SPI_TRANSFER_TIMEOUT)) {''',
            '''      if ((uint32_t)(micros() - tickstart) >= 1000) {''')
    replace('''    // Add a delay before disabling SPI otherwise last-bit/last-clock may be truncated
    // See https://github.com/stm32duino/Arduino_Core_STM32/issues/1294
    // Computed delay is half SPI clock
    delayMicroseconds(obj->disable_delay);''',
            '''    if (ret == SPI_OK) ret = spi_wait_flag(_SPI, SPI_SR_EOT, tickstart);
transfer_done:
    // All exits disable SPI; SPIEx releases NSS and reports the result.''')
    return source
