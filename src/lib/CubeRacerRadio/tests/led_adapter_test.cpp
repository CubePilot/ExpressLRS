#include "common.h"
#include "devLED.h"
#include "board_profile.h"
#include <cassert>
#include <cstdio>
unsigned genericWrites=0;
bool connectionHasModelMatch=true,teamraceHasModelMatch=true,InBindingMode=false;
connectionState_e connectionState=disconnected;
static bool high=false;
static unsigned writes=0;
void digitalWrite(int pin,int value) {
    assert(pin==CubeRacer::LED_PIN);
    high=value;writes++;
}
int main() {
    assert(LED_device.initialize());LED_device.start();LED_device.timeout();
    assert(genericWrites==0 && writes>0);
    assert(LED_device.initialize());assert(!high);
    assert(LED_device.event()==500 && high);assert(LED_device.timeout()==500 && !high);
    connectionState=connected;LED_device.event();assert(high);
    InBindingMode=true;assert(LED_device.event()==100 && high);
    assert(LED_device.timeout()==100 && !high);
    InBindingMode=false;connectionState=radioFailed;
    assert(LED_device.event()==200 && high);assert(LED_device.timeout()==1000 && !high);
    assert(genericWrites==0);
    puts("CubeRacer real LED device ownership and patterns tests passed");
}
