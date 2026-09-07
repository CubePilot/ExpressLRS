#include "options.h"
#include "board_profile.h"
#include <cassert>
#include <cstring>
#include <cstdio>
int main() {
    firmware_options_t options;
    memset(&options,0xa5,sizeof(options));
    CubeRacer::bootOptions(options);
    assert(!options.hasUID && !options.is_airport && !options.flash_discriminator);
    for(auto b:options.uid)assert(!b);
    assert(!options.home_wifi_ssid[0] && !options.home_wifi_password[0]);
    assert(options.wifi_auto_on_interval==-1 && options.uart_baud==420000);
#ifdef Regulatory_Domain_EU_CE_2400
    assert(options.domain==1);
#else
    assert(options.domain==0);
#endif
    puts("CubeRacer boot options clear standalone settings and use compiled domain");
}
