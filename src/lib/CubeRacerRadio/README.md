# CubeRacer radio integration

M7 configures radio GPIO modes/pulls/AFs, shared clocks, EXTI routing and PE10
power before releasing M4. M4 owns SPI4, TIM1 and its radio GPIOs until reset.
There are no runtime hardware requests, grants, revocations or power-off ACKs.

`CubeRacerRadio.cpp` supplies SPI initialization/error handling, M4 interrupt
setup and local fault shutdown. SPIEx and STM32duino perform SPI/GPIO access;
the existing build middleware omits shared configuration writes from the M4
SPI/timer implementations. SPIEx performs transfers directly, with bounded
polling and a latched error exposed through `getLastError()`. Each SPI flag wait
has an iteration limit; it does not use the interrupt-driven Arduino timebase
while interrupts are masked. The radio lifecycle
maps that status to its existing fault reports; SPIEx has no radio dependency.
The transfer preserves STM32duino's final-clock settling delay before disabling SPI.
The common SX1280 BUSY wait uses digitalRead; timeout is advisory and transfers
continue. Reset, antenna, LED and RFAMP paths use their common implementations.

M4 starts the receiver after authoritative settings arrive. A new session or
settings revision refreshes the running receiver without repeating hardware
initialization. Link loss, receiver deselection and pending settings inhibit
READY/RC/CRSF data; they do not revoke hardware or cut power. Runtime/init faults
stop the radio locally and hold reset. PE10 stays on until the next MCU reset.

M4 sends sequenced READY/FAULT reports for the current session/revision. M7
requires fresh configuration, link and RF status before consuming data. The
legacy protobuf grant field is reserved as zero; old lease messages are rejected.
See CubeFramework's `docs/receiver-radio-control.md` for the transport contract.

## Board profile

`OPTIONS/cuberacer_hardware.cpp` supplies the fixed profile from `board_profile.h`:
PB4 NSS, PE2 SCK, PE13 MISO, PE14 MOSI, PF5 RESET, PE5 BUSY, PA4 DIO1,
PE6 antenna, PF3 button and PF13 LED. Canonical digital indices avoid the
variant's analog aliases. PE10 remains M7-owned. External PA/TX/RX pins and
optional devices are undefined; the common RFAMP implementation does no GPIO
work for this board. PE6 low selects the onboard chip antenna (index 0);
high selects the external U.FL connector (index 1). Mode 2 enables automatic
switched diversity. The SX1281 telemetry ceiling remains +10 dBm.

M7 owns pinMode and other shared-register configuration. Arduino digitalWrite
uses BSRR for individual pin changes. SPI transaction interrupt masking protects
against DIO1/TIM1 re-entry on M4, independently of inter-core GPIO access.
Button debounce resets on local radio failure; binding and settings changes
continue to use the M7 configuration authority and existing armed-state rules.

## Validation

```sh
bash src/lib/CubeRacerRadio/tests/run.sh
STM32_CORE_ROOT=/path/to/framework-arduinoststm32 python3 src/variants/CUBERACER_M4/tests/test_spi_transport.py
STM32_CORE_ROOT=/path/to/framework-arduinoststm32 python3 src/variants/CUBERACER_M4/tests/test_timer_ownership.py
```

On this Mac, set `CXX=/opt/homebrew/bin/g++-15` for the first command. The tests
exercise advisory BUSY, real common reset/RFAMP code, SPI failures, timer fault
handling, button debounce and LED patterns. Betaflight's
`src/test/cubefw_integration` runs both adapters with configuration, RF-state,
restart and control-loss scenarios. Deploy matching M7 and M4 images together.
Current hardware evidence and build commands are in the Betaflight handover.
