#pragma once
#include <cstdint>
#define TIM1 ((void *)0x40010000)
#define TIMER_OUTPUT_COMPARE 1
#define TICK_FORMAT 0
#define MICROSEC_FORMAT 1
struct HardwareTimer {
    static unsigned writes, setups, ticks;
    static bool active;
    static void (*callback)();
    void setup(void *) { writes++;setups++; }
    void attachInterrupt(void (*cb)()) { writes++;callback=cb; }
    void setMode(int,int) { writes++; }
    void setPreloadEnable(bool) { writes++; }
    void setOverflow(uint32_t value,int) { writes++;ticks=value; }
    uint32_t getOverflow(int) { return ticks; }
    void setCount(uint32_t) { writes++; }
    void pause() { writes++;active=false; }
    void resume() { writes++;active=true; }
    void refresh() { writes++;if(active && callback)callback(); }
};
