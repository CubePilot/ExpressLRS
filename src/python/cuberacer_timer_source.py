"""Adapt only STM32duino's clock hooks; preserve its TIM1 timing/IRQ behavior."""
import re

def adapt_timer_source(source):
    for name in ('enableTimerClock','disableTimerClock'):
        pattern=r'void %s\(TIM_TypeDef \*instance\)\n\{.*?^\}'%name
        if len(re.findall(pattern,source,re.M|re.S))!=1:
            raise ValueError('Pinned STM32 timer hook changed: '+name)
        replacement='''void %s(TIM_TypeDef *instance)
{
  // M7 owns RCC. Check the grant and clock, but never enable/reset/disable it.
  if (!cuberacerTimerClockAllowed((uintptr_t)instance)) {
    Error_Handler();
    return;
  }
}'''%name
        source=re.sub(pattern,lambda _:replacement,source,flags=re.M|re.S)
    anchor='/* Private Functions */'
    if source.count(anchor)!=1 or re.search(r'\b(?:__HAL_RCC_|HAL_RCC_)',source):
        raise ValueError('Unexpected shared RCC access in STM32 timer source')
    return source.replace(anchor,'\nextern int cuberacerTimerClockAllowed(uintptr_t instance);\n'+anchor,1)
