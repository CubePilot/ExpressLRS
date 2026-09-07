"""Exercise the actual pinned clock-hook bodies with fake RCC writes."""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile
import unittest
sys.path.insert(0,str(Path(__file__).parents[3]/'python'))
from cuberacer_timer_source import adapt_timer_source

PATTERN=r'void %s\(TIM_TypeDef \*instance\)\n\{.*?^\}'
class TimerOwnershipTest(unittest.TestCase):
    def source(self):
        root=Path(os.environ['STM32_CORE_ROOT'])
        return (root/'libraries/SrcWrapper/src/stm32/timer.c').read_text()
    def test_pinned_hooks_leave_clocks_unchanged_and_reject_unowned_timer(self):
        source=adapt_timer_source(self.source())
        bodies='\n'.join(re.search(PATTERN%name,source,re.M|re.S).group() for name in ('enableTimerClock','disableTimerClock'))
        with tempfile.TemporaryDirectory() as tmp:
            d=Path(tmp)
            (d/'test.c').write_text('''
#include <stdint.h>
#include <assert.h>
typedef struct { unsigned unused; } TIM_TypeDef;
static TIM_TypeDef timer1,timer2;
#define TIM1_BASE 1
#define TIM1 (&timer1)
static unsigned clockWrites,errors;
#define __HAL_RCC_TIM1_CLK_ENABLE() (++clockWrites)
#define __HAL_RCC_TIM1_CLK_DISABLE() (++clockWrites)
void Error_Handler(void) { errors++; }
''' + bodies + '''
int main(void) {
    enableTimerClock(TIM1);disableTimerClock(TIM1);
    assert(clockWrites==0 && errors==0);
    enableTimerClock(&timer2);disableTimerClock(&timer2);assert(errors==2);
}
''')
            subprocess.run(['cc','-std=c11','-Wall','-Werror',str(d/'test.c'),'-o',str(d/'test')],check=True)
            subprocess.run([str(d/'test')],check=True)
    def test_rejects_changed_or_duplicate_vendor_hooks(self):
        source=self.source()
        for changed in [source.replace('void enableTimerClock(', 'void renamedClock('),source+'\n'+source]:
            with self.assertRaises(ValueError):adapt_timer_source(changed)
    def test_preserves_vendor_code_outside_the_clock_hooks(self):
        source=self.source();adapted=adapt_timer_source(source)
        for name in ('enableTimerClock','disableTimerClock'):
            source=re.sub(PATTERN%name,'HOOK',source,flags=re.M|re.S)
            adapted=re.sub(PATTERN%name,'HOOK',adapted,flags=re.M|re.S)
        adapted=adapted.replace('\nextern int cuberacerTimerClockAllowed(uintptr_t instance);\n','')
        self.assertEqual(source,adapted)
        self.assertNotIn('__HAL_RCC_',adapted)
        self.assertNotIn('HAL_RCC_',adapted)

if __name__=='__main__':unittest.main()
