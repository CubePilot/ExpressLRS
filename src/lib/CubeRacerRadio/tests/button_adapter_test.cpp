#include <cstdint>
#include <cassert>
#include <cstdio>
#define INPUT 0
#define INPUT_PULLUP 2
#define DBGVLN(...) ((void)0)
static uint32_t now;
static unsigned modeWrites, shortPresses, longPresses;
static bool owned=false, high=true;
static unsigned long millis() { return now; }
static void pinMode(uint8_t,int) { modeWrites++; }
static int digitalRead(uint8_t) { return high; }
bool cuberacerRadioButtonRead(uint8_t pin,bool &level) {
    if(!owned || pin!=81)return false;
    level=high;return true;
}
#include "button.h"
static void step(Button &button,bool level,unsigned ms=25) { high=level;now+=ms;button.update(); }
int main() {
    Button button;
    button.OnShortPress=[](){shortPresses++;};button.OnLongPress=[](){longPresses++;};
    button.init(81);assert(modeWrites==0);
    step(button,false,600);step(button,false,600);step(button,false,600);
    assert(shortPresses==0 && longPresses==0);
    owned=true;
    step(button,false);step(button,false);step(button,false);
    step(button,true);step(button,true);assert(shortPresses==1);
    step(button,false);step(button,false);step(button,false,501);assert(longPresses==1);
    owned=false;step(button,false,600);step(button,true);assert(longPresses==1 && shortPresses==1);
    owned=true;step(button,true);step(button,true);assert(shortPresses==1); // No stale release.
    // No device polls happen while the runtime is stopped. Explicit shutdown
    // reset must prevent a partial old press leaking into the next grant.
    step(button,false);step(button,false);step(button,false);
    button.reset();step(button,true,2000);step(button,true);assert(shortPresses==1);
    button.init(80);step(button,false);step(button,false);step(button,false,600);
    assert(longPresses==1 && modeWrites==0);
    puts("CubeRacer real Button ownership and debounce tests passed");
}
