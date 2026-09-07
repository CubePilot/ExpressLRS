"""Compile the production early hooks against a core-register-only fake HAL."""
from pathlib import Path
import subprocess
import tempfile
import unittest


class StartupTest(unittest.TestCase):
    def test_startup_only_initializes_local_core_and_inherits_clock(self):
        with tempfile.TemporaryDirectory() as directory:
            d = Path(directory)
            (d / 'stm32h7xx.h').write_text('''
#include <stdint.h>
typedef struct { uint32_t CPACR, VTOR; } Core;
extern Core core;
#define SCB (&core)
void barrier(void);
#define __DSB() barrier()
#define __ISB() barrier()
void SystemCoreClockUpdate(void);
''')
            (d / 'stm32h7xx_hal.h').write_text('''
#define TICK_INT_PRIORITY 15
int HAL_InitTick(unsigned priority);
''')
            (d / 'dwt.h').write_text('unsigned dwt_init(void);\n')
            (d / 'test.c').write_text('''
#include <assert.h>
#include "stm32h7xx.h"
Core core = {1, 0};
static unsigned barriers, phase;
void barrier(void) { ++barriers; }
void SystemCoreClockUpdate(void) { assert(phase == 0); phase = 1; }
int HAL_InitTick(unsigned priority) {
    assert(phase == 1); assert(priority == 15); phase = 2; return 0;
}
unsigned dwt_init(void) { assert(phase == 2); phase = 3; return 0; }
void __wrap_SystemInit(void);
void __wrap_ExitRun0Mode(void);
void init(void);
int main(void) {
    __wrap_ExitRun0Mode();
    assert(core.CPACR == 1 && core.VTOR == 0 && phase == 0);
    __wrap_SystemInit();
    assert(core.CPACR == (1 | (15u << 20)));
    assert(core.VTOR == 0x08180000 && barriers == 2 && phase == 0);
    init(); assert(phase == 3);
}
''')
            subprocess.run(['cc', '-std=c11', '-Wall', '-Werror', '-DCORE_CM4',
                            '-I', str(d), str(Path(__file__).parents[1] / 'startup.c'),
                            str(d / 'test.c'), '-o', str(d / 'test')], check=True)
            subprocess.run([str(d / 'test')], check=True)


if __name__ == '__main__':
    unittest.main()
