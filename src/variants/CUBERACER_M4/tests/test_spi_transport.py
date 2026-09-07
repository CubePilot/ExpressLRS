"""Run the adapted Arduino transfer and real SPIEx transaction against fake I/O."""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile
import unittest
sys.path.insert(0, str(Path(__file__).parents[3] / 'python'))
from cuberacer_spi_source import adapt_spi_source


class SpiTransportTest(unittest.TestCase):
    def test_transfer_and_fault_cleanup(self):
        root = Path(os.environ['STM32_CORE_ROOT'])
        source = (root / 'libraries/SPI/src/utility/spi_com.c').read_text()
        adapted = adapt_spi_source(source)
        self.assertNotRegex(adapted, r'__HAL_RCC_SPI\d+_|\bpinmap_pinout\(|\bpin_PullConfig\(')
        for old in ('spi_status_e spi_transfer(', 'tickstart = HAL_GetTick();'):
            with self.assertRaises(ValueError):
                adapt_spi_source(source.replace(old, 'changed'))
        bodies = re.search(r'static spi_status_e spi_wait_flag\(.*?^\}', adapted, re.M | re.S).group()
        bodies += '\n' + re.search(r'spi_status_e spi_transfer\(.*?^\}', adapted, re.M | re.S).group()
        spiex = (Path(__file__).parents[3] / 'lib/SPIEx/SPIEx.cpp').read_text()
        bodies += '\n' + re.search(r'spi_status_e SPIExClass::transferChecked\(.*?^\}', spiex, re.M | re.S).group()
        with tempfile.TemporaryDirectory() as tmp:
            test = Path(tmp) / 'test.cpp'
            test.write_text('''
#include <cstdint>
#include <cassert>
#define CUBERACER_M4 1
#define SPI_CR2_TSIZE 1
#define SPI_IFCR_EOTC 1
#define SPI_SR_TXP 2
#define SPI_SR_RXP 1
#define SPI_SR_EOT 8
#define SPI_SR_UDR 32
#define SPI_SR_OVR 64
#define SPI_SR_CRCE 128
#define SPI_SR_TIFRE 256
#define SPI_SR_MODF 512
#define GPIO_PIN_NSS 4
#define LOW 0
#define HIGH 1
struct SPI_TypeDef { uint32_t SR; } peripheral;
struct spi_t { SPI_TypeDef *spi=&peripheral; };
enum spi_status_e { SPI_OK, SPI_ERROR, SPI_TIMEOUT };
class SPIExClass {
    spi_t _spi;
public: spi_status_e transferChecked(uint8_t *,uint32_t);
};
static uint32_t ticks, calls, mask, lows, highs, sent, received, size;
static bool frozen, enabled, selected;
static uint8_t byte;
uint32_t micros() { assert(++calls<300000);return frozen?ticks:ticks++; }
uint32_t __get_PRIMASK() { return mask; }
void __disable_irq() { mask=1; }
void __set_PRIMASK(uint32_t value) { mask=value; }
void digitalWrite(unsigned pin,unsigned value) {
    assert(pin==4 && mask==1);
    if(value) { assert(!enabled);highs++;selected=false; }
    else { lows++;selected=true; }
}
void delayMicroseconds(unsigned us) { assert(us==1); }
void LL_SPI_SetTransferSize(SPI_TypeDef *,uint32_t value) { size=value; }
void LL_SPI_Enable(SPI_TypeDef *) { enabled=true; }
void LL_SPI_StartMasterTransfer(SPI_TypeDef *) { assert(selected); }
void LL_SPI_TransmitData8(SPI_TypeDef *,uint8_t value) { byte=value^0x5a;sent++; }
uint8_t LL_SPI_ReceiveData8(SPI_TypeDef *) { received++;return byte; }
void LL_SPI_ClearFlag_EOT(SPI_TypeDef *) {}
void LL_SPI_ClearFlag_TXTF(SPI_TypeDef *) {}
void LL_SPI_Disable(SPI_TypeDef *) { enabled=false; }
''' + bodies + '''
static void check(uint32_t flags, spi_status_e expected, bool freeze=false) {
    peripheral.SR=flags;ticks=0xfffffff0;calls=0;frozen=freeze;
    for(unsigned previous:{0u,1u}) {
        mask=previous;lows=highs=sent=received=0;
        uint8_t data[259];for(unsigned i=0;i<259;i++)data[i]=i;
        SPIExClass spi;
        assert(spi.transferChecked(data,259)==expected);
        assert(mask==previous && lows==1 && highs==1 && !enabled && !selected);
        if(expected==SPI_OK) {
            assert(size==259 && sent==259 && received==259);
            for(unsigned i=0;i<259;i++)assert(data[i]==uint8_t(i^0x5a));
        }
    }
}
int main() {
    check(11,SPI_OK);
    check(0,SPI_TIMEOUT); // TX never ready.
    check(2,SPI_TIMEOUT); // RX never ready.
    check(3,SPI_TIMEOUT); // EOT never arrives.
    check(11|SPI_SR_MODF,SPI_ERROR); // Error wins over ready flags.
    check(0,SPI_TIMEOUT,true); // Stalled timebase still terminates.
    SPIExClass spi;uint8_t data=0;lows=highs=0;
    assert(spi.transferChecked(nullptr,1)==SPI_ERROR);
    assert(spi.transferChecked(&data,0)==SPI_ERROR);
    assert(spi.transferChecked(&data,65536)==SPI_ERROR);
    assert(lows==0 && highs==0);
}
''')
            binary = str(Path(tmp) / 'test')
            subprocess.run(['c++', '-std=c++11', '-include', 'initializer_list', str(test), '-o', binary], check=True)
            subprocess.run([binary], check=True, timeout=3)


if __name__ == '__main__':
    unittest.main()
