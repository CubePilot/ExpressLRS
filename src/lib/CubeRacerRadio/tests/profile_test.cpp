#include "hardware.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>

int main()
{
    // Arduino indices from the pinned STM32H757 generic digital-pin table.
    assert(hardware_pin(HARDWARE_radio_nss) == 20);
    assert(hardware_pin(HARDWARE_radio_busy) == 67);
    assert(hardware_pin(HARDWARE_radio_dio1) == 4);
    assert(hardware_pin(HARDWARE_radio_rst) == 83);
    assert(hardware_pin(HARDWARE_radio_sck) == 64);
    assert(hardware_pin(HARDWARE_radio_miso) == 75);
    assert(hardware_pin(HARDWARE_radio_mosi) == 76);
    assert(hardware_pin(HARDWARE_ant_ctrl) == 68);
    assert(hardware_pin(HARDWARE_button) == 81 && hardware_pin(HARDWARE_led) == 91);
    for (auto pin : {HARDWARE_serial_rx, HARDWARE_serial_tx, HARDWARE_serial1_rx, HARDWARE_serial1_tx,
                     HARDWARE_power_enable, HARDWARE_radio_nss_2, HARDWARE_radio_busy_2, HARDWARE_radio_dio1_2,
                     HARDWARE_radio_rst_2, HARDWARE_power_txen, HARDWARE_power_rxen, HARDWARE_power_apc2,
                     HARDWARE_led_rgb, HARDWARE_i2c_sda, HARDWARE_vtx_nss, HARDWARE_misc_fan_pwm})
        assert(hardware_pin(pin) == -1);
    assert(hardware_flag(HARDWARE_radio_dcdc));
    assert(!hardware_flag(HARDWARE_led_red_invert));
    assert(hardware_int(HARDWARE_power_min) == 0 && hardware_int(HARDWARE_power_max) == 0);
    assert(hardware_int(HARDWARE_power_default) == 0 && hardware_int(HARDWARE_power_control) == 0);
    assert(hardware_int(HARDWARE_power_values_count) == 1);
    assert(hardware_i16_array(HARDWARE_power_values)[0] == 10);
    assert(hardware_i16_array(HARDWARE_power_values2) == nullptr);
    assert(hardware_i16_array(HARDWARE_pwm_outputs) == nullptr && hardware_int(HARDWARE_pwm_outputs_count) == 0);
    assert(hardware_int(HARDWARE_power_values_dual_count) == 0);
    puts("CubeRacer fixed hardware profile tests passed");
}
