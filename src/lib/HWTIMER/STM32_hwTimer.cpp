#if defined(PLATFORM_STM32)
#include "hwTimer.h"
#include "logging.h"
#include <HardwareTimer.h>

void (*hwTimer::callbackTick)() = nullptr;
void (*hwTimer::callbackTock)() = nullptr;

volatile bool hwTimer::running = false;
volatile bool hwTimer::isTick = false;

volatile uint32_t hwTimer::HWtimerInterval = TimerIntervalUSDefault;
volatile int32_t hwTimer::PhaseShift = 0;
volatile int32_t hwTimer::FreqOffset = 0;

#if defined(TARGET_RX)
#define HWTIMER_TICKS_PER_US 5
#else
#define HWTIMER_TICKS_PER_US 1
#endif

// TIM5 is a 32-bit hardware timer, but STM32duino's HardwareTimer library
// caps MAX_RELOAD to 16-bit (65535) for generic behavior across all timers.
// We handle intervals > 65535 ticks by counting intermediate interrupts.
#define MAX_RELOAD 65535

static HardwareTimer *timer = nullptr;
static bool timerInitialised = false;

static volatile uint32_t wrapRemaining = 0;

static void setTimerInterval(uint32_t ticks)
{
    if (ticks > MAX_RELOAD)
    {
        wrapRemaining = ticks - MAX_RELOAD;
        timer->setOverflow(MAX_RELOAD, TICK_FORMAT);
    }
    else
    {
        wrapRemaining = 0;
        timer->setOverflow(ticks > 0 ? ticks : 1, TICK_FORMAT);
    }
}

void timerCallback()
{
    if (wrapRemaining > 0)
    {
        if (wrapRemaining > MAX_RELOAD)
        {
            wrapRemaining -= MAX_RELOAD;
            timer->setOverflow(MAX_RELOAD, TICK_FORMAT);
        }
        else
        {
            timer->setOverflow(wrapRemaining > 0 ? wrapRemaining : 1, TICK_FORMAT);
            wrapRemaining = 0;
        }
        return;
    }
    hwTimer::callback();
}

void hwTimer::init(void (*callbackTick)(), void (*callbackTock)())
{
    if (!timerInitialised)
    {
        hwTimer::callbackTick = callbackTick;
        hwTimer::callbackTock = callbackTock;

        timer = new HardwareTimer(TIM5);

        uint32_t timerClk = timer->getTimerClkFreq();
        uint32_t prescaler = (timerClk / (1000000 * HWTIMER_TICKS_PER_US));

        timer->setPrescaleFactor(prescaler);
        timer->setOverflow(HWtimerInterval, TICK_FORMAT);
        timer->attachInterrupt(timerCallback);
        timer->setPreloadEnable(true);

        timerInitialised = true;
        DBGLN("hwTimer Init");
    }
}

void hwTimer::stop()
{
    if (timerInitialised && running)
    {
        running = false;
        timer->pause();
        DBGLN("hwTimer stop");
    }
}

void hwTimer::resume()
{
    if (timerInitialised && !running)
    {
#if defined(TARGET_TX)
        setTimerInterval(HWtimerInterval);
#else
        isTick = false;
        wrapRemaining = 0;
        timer->setOverflow(1, TICK_FORMAT);
#endif
        timer->setCount(0);
        running = true;
        timer->resume();
        DBGLN("hwTimer resume");
    }
}

void hwTimer::updateInterval(uint32_t time)
{
    HWtimerInterval = time * HWTIMER_TICKS_PER_US;
    if (timerInitialised)
    {
        DBGLN("hwTimer interval: %d", time);
        setTimerInterval(HWtimerInterval);
    }
}

void hwTimer::phaseShift(int32_t newPhaseShift)
{
    int32_t minVal = -(HWtimerInterval >> 2);
    int32_t maxVal = (HWtimerInterval >> 2);

    PhaseShift = constrain(newPhaseShift, minVal, maxVal) * HWTIMER_TICKS_PER_US;
}

void hwTimer::callback()
{
    if (running)
    {
#if defined(TARGET_TX)
        callbackTock();
#else
        uint32_t NextInterval = (HWtimerInterval >> 1) + FreqOffset;
        if (hwTimer::isTick)
        {
            setTimerInterval(NextInterval);
            hwTimer::callbackTick();
        }
        else
        {
            NextInterval += PhaseShift;
            setTimerInterval(NextInterval);
            PhaseShift = 0;
            hwTimer::callbackTock();
        }
        hwTimer::isTick = !hwTimer::isTick;
#endif
    }
}

#endif
