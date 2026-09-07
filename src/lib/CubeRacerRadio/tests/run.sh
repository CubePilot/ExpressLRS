#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../../.."
build_dir=$(mktemp -d /tmp/cuberacer-radio-tests.XXXXXX)
trap 'rm -rf "$build_dir"' EXIT
gc_sections=-Wl,--gc-sections
[[ $(uname -s) != Darwin ]] || gc_sections=-Wl,-dead_strip
incs=(-Ilib/CubeRacerRadio/tests/include -Iinclude -Ilib/CubeRacerRadio)
for dir in lib/*; do [[ ! -d "$dir" ]] || incs+=("-I$dir"); done
"${CXX:-g++}" -std=gnu++11 -g -ffunction-sections -fdata-sections "$gc_sections" \
    -include lib/CubeRacerRadio/tests/include/native_debug.h -DUNIT_TEST -DCUBERACER_RADIO_HAL_TEST -DCUBERACER_M4 -DTARGET_NATIVE -DPROGMEM= \
    "${incs[@]}" lib/CubeRacerRadio/tests/radio_hal_test.cpp lib/SX1280Driver/SX1280_hal.cpp \
    lib/SX1280Driver/SX1280.cpp lib/SPIEx/SPIEx.cpp lib/RFAMP/RFAMP_hal.cpp -o "$build_dir/hal"
"$build_dir/hal"
"${CXX:-g++}" -std=gnu++11 -DUNIT_TEST -DCUBERACER_M4 -DTARGET_NATIVE -DPLATFORM_STM32 -DTARGET_RX -DPROGMEM= \
    -include lib/CubeRacerRadio/tests/include/timer_fake.h "${incs[@]}" \
    lib/CubeRacerRadio/tests/timer_adapter_test.cpp lib/HWTIMER/STM32_hwTimer.cpp -o "$build_dir/timer"
"$build_dir/timer"
"${CXX:-g++}" -std=gnu++11 -DCUBERACER_M4 -Ilib/BUTTON -Ilib/CubeRacerRadio \
    lib/CubeRacerRadio/tests/button_adapter_test.cpp -o "$build_dir/button"
"$build_dir/button"
"${CXX:-g++}" -std=gnu++11 -DUNIT_TEST -DCUBERACER_M4 -DTARGET_NATIVE -DTARGET_RX -DPROGMEM= \
    "${incs[@]}" -include lib/CubeRacerRadio/tests/include/led_fake.h \
    lib/CubeRacerRadio/tests/led_adapter_test.cpp lib/LED/devLED.cpp -o "$build_dir/led"
"$build_dir/led"
"${CXX:-g++}" -std=c++11 -DCUBERACER_M4 -Iinclude -Ilib/CubeRacerRadio \
    lib/CubeRacerRadio/tests/profile_test.cpp lib/OPTIONS/cuberacer_hardware.cpp -o "$build_dir/profile"
"$build_dir/profile"
for domain in "" "-DRegulatory_Domain_EU_CE_2400"; do
    "${CXX:-g++}" -std=c++11 -DUNIT_TEST -DTARGET_NATIVE $domain "${incs[@]}" \
        lib/CubeRacerRadio/tests/options_test.cpp -o "$build_dir/options"
    "$build_dir/options"
done
