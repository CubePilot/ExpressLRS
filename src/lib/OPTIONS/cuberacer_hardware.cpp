#ifdef CUBERACER_M4
#include "hardware.h"
#include "board_profile.h"
using namespace CubeRacer;

int hardware_pin(nameType name)
{
    switch (name)
    {
    case HARDWARE_radio_nss:
        return NSS_PIN;
    case HARDWARE_radio_busy:
        return BUSY_PIN;
    case HARDWARE_radio_dio1:
        return DIO1_PIN;
    case HARDWARE_radio_rst:
        return RESET_PIN;
    case HARDWARE_radio_sck:
        return SCK_PIN;
    case HARDWARE_radio_miso:
        return MISO_PIN;
    case HARDWARE_radio_mosi:
        return MOSI_PIN;
    case HARDWARE_ant_ctrl:
        return ANTENNA_PIN;
    case HARDWARE_button:
        return BUTTON_PIN;
    case HARDWARE_led:
        return LED_PIN;
    default:
        return -1; // M7 power, UARTs, second radio and optional devices absent.
    }
}

bool hardware_flag(nameType name)
{
    return name == HARDWARE_radio_dcdc;
}

int hardware_int(nameType name)
{
    switch (name)
    {
    case HARDWARE_power_min:
    case HARDWARE_power_max:
    case HARDWARE_power_high:
    case HARDWARE_power_default:
    case HARDWARE_power_control:
    case HARDWARE_power_lna_gain:
    case HARDWARE_power_values_dual_count:
    case HARDWARE_radio_rfsw_ctrl_count:
    case HARDWARE_pwm_outputs_count:
    case HARDWARE_ledidx_rgb_status_count:
    case HARDWARE_ledidx_rgb_vtx_count:
    case HARDWARE_ledidx_rgb_boot_count:
    case HARDWARE_misc_fan_speeds_count:
    case HARDWARE_screen_type:
        return 0;
    case HARDWARE_power_values_count:
        return 1;
    default:
        return -1;
    }
}

float hardware_float(nameType)
{
    return 0;
}

const int16_t *hardware_i16_array(nameType name)
{
    static const int16_t power[] = {MAX_OUTPUT_DBM};
    return name == HARDWARE_power_values ? power : nullptr;
}

const uint16_t *hardware_u16_array(nameType)
{
    return nullptr;
}
#endif
