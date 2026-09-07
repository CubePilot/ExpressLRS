#include "hwTimer.h"
#include <cassert>
#include <cstdio>
unsigned HardwareTimer::writes=0,HardwareTimer::setups=0,HardwareTimer::ticks=0;
bool HardwareTimer::active=false;
void (*HardwareTimer::callback)()=nullptr;
static bool ready=false;
static bool owned=false;
static unsigned ticks=0,tocks=0;
bool cuberacerRadioTimerReady() { return ready; }
extern "C" int cuberacerTimerClockAllowed(uintptr_t timer) { return owned && timer==0x40010000U; }
static void tick() { ticks++; }
static void tock() { tocks++; }
int main() {
    assert(HardwareTimer::writes==0 && !hwTimer::initialized());
    hwTimer::init(tick,tock);hwTimer::stop();hwTimer::resume();
    assert(HardwareTimer::writes==0 && !hwTimer::running);
    ready=owned=true;hwTimer::init(tick,tock);assert(HardwareTimer::setups==1 && hwTimer::initialized());
    assert(!hwTimer::running);hwTimer::updateInterval(2000);hwTimer::resume();
    assert(hwTimer::running && HardwareTimer::active && tocks==1 && ticks==0);
    HardwareTimer::callback();assert(ticks==1 && tocks==1);
    hwTimer::phaseShift(100);HardwareTimer::callback();assert(tocks==2 && HardwareTimer::ticks==1100);
    HardwareTimer::callback();assert(ticks==2 && HardwareTimer::ticks==1000);
    ready=false;HardwareTimer::callback();assert(!hwTimer::running && !HardwareTimer::active);
    assert(ticks==2 && tocks==2);hwTimer::resume();assert(!HardwareTimer::active);
    ready=true;hwTimer::init(tick,tock);assert(HardwareTimer::setups==1 && hwTimer::initialized());
    hwTimer::resume();assert(tocks==3);hwTimer::stop();assert(!HardwareTimer::active);
    ready=owned=false; // Grant released after quiescence: late calls must not touch TIM1.
    const unsigned writes=HardwareTimer::writes;
    hwTimer::stop();hwTimer::resume();HardwareTimer::callback();hwTimer::init(tick,tock);
    assert(HardwareTimer::writes==writes && !hwTimer::running && !hwTimer::initialized());
    puts("CubeRacer real hwTimer grant, callback and fault-stop tests passed");
}
