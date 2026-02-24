#include "PWM.h"

#if defined(PLATFORM_STM32)

#include "logging.h"

#define MAX_PWM_CHANNELS 16

static struct
{
    int8_t pin;
    uint32_t interval; // period in microseconds
    HardwareTimer *timer;
    uint32_t channel;
} pwm_config[MAX_PWM_CHANNELS];

pwm_channel_t PWMController::allocate(uint8_t pin, uint32_t frequency)
{
    for (int ch = 0; ch < MAX_PWM_CHANNELS; ch++)
    {
        if (pwm_config[ch].pin == -1)
        {
            PinName pinName = digitalPinToPinName(pin);
            TIM_TypeDef *instance = (TIM_TypeDef *)pinmap_peripheral(pinName, PinMap_PWM);
            if (instance == nullptr)
            {
                DBGLN("No timer for pin %d", pin);
                return -1;
            }

            uint32_t timerChannel = STM_PIN_CHANNEL(pinmap_function(pinName, PinMap_PWM));

            HardwareTimer *timer = new HardwareTimer(instance);
            timer->setMode(timerChannel, TIMER_OUTPUT_COMPARE_PWM1, pin);
            timer->setOverflow(frequency, HERTZ_FORMAT);
            timer->setCaptureCompare(timerChannel, 0, TICK_COMPARE_FORMAT);
            timer->resume();

            pwm_config[ch].pin = pin;
            pwm_config[ch].interval = 1000000U / frequency;
            pwm_config[ch].timer = timer;
            pwm_config[ch].channel = timerChannel;

            DBGLN("PWM allocate ch %d on pin %d, freq %dHz", ch, pin, frequency);
            return ch;
        }
    }
    DBGLN("No PWM channels available");
    return -1;
}

void PWMController::release(pwm_channel_t channel)
{
    if (channel >= 0 && channel < MAX_PWM_CHANNELS && pwm_config[channel].pin != -1)
    {
        pwm_config[channel].timer->pause();
        delete pwm_config[channel].timer;
        pwm_config[channel].timer = nullptr;
        pwm_config[channel].pin = -1;
        pwm_config[channel].interval = 0;
    }
}

void PWMController::setDuty(pwm_channel_t channel, uint16_t duty)
{
    if (channel >= 0 && channel < MAX_PWM_CHANNELS && pwm_config[channel].timer)
    {
        uint32_t overflow = pwm_config[channel].timer->getOverflow(TICK_FORMAT);
        uint32_t compare = (uint32_t)duty * overflow / 1000;
        pwm_config[channel].timer->setCaptureCompare(pwm_config[channel].channel, compare, TICK_COMPARE_FORMAT);
    }
}

void PWMController::setMicroseconds(pwm_channel_t channel, uint16_t microseconds)
{
    if (channel >= 0 && channel < MAX_PWM_CHANNELS && pwm_config[channel].timer)
    {
        uint32_t overflow = pwm_config[channel].timer->getOverflow(TICK_FORMAT);
        uint32_t compare = (uint32_t)microseconds * overflow / pwm_config[channel].interval;
        pwm_config[channel].timer->setCaptureCompare(pwm_config[channel].channel, compare, TICK_COMPARE_FORMAT);
    }
}

#endif
