# CubeRacer radio integration

CubeRacer uses the existing SX1280 command driver, SPIEx and STM32duino SPI/GPIO
implementation. `CubeRacerRadio.cpp` supplies grant/revoke validation, M4 interrupt
setup, latched SPI faults, and shutdown/readback before M7 removes radio power.

The shared `SX1280Hal::WaitOnBusy()` reads BUSY with Arduino `digitalRead()`.
BUSY timeouts are advisory: command transfers proceed, as requested for this
bring-up. They do not latch a fault or prevent initialization by themselves.
Invalid firmware revisions and actual SPI transfer errors still fail startup.

SPIEx uses `spi_transfer()` directly so its result is available to the caller.
The existing build middleware compiles an adapted STM32duino `spi_com.c` only
for M4: shared GPIO/RCC writes are omitted, TX/RX/EOT waits have a 1 ms deadline
and an iteration limit, and every transfer exit disables SPI. SPIEx releases
NSS and restores the previous interrupt mask on errors as well as success.
The installed framework is unchanged; other targets retain its original source.

M7 configures the radio pins/clocks, EXTI routing and PE10 power before granting
M4 access. Output changes use `digitalWrite()`; STM32H757's LL implementation
writes BSRR, so different pins on the same port do not require a cross-core
lock. Pin mode/AF/pull configuration remains M7-owned. Local interrupt exclusion
protects complete SPI transactions against DIO1/TIM1 re-entry on M4.

Validation:

```sh
bash src/lib/CubeRacerRadio/tests/run.sh
STM32_CORE_ROOT=/path/to/framework-arduinoststm32 python3 src/variants/CUBERACER_M4/tests/test_spi_transport.py
```

On this Mac, use `CXX=/opt/homebrew/bin/g++-15` for the first command. The HAL
test holds BUSY high and checks that commands and initialization proceed, then
injects SPI failures. The transport test runs the adapted Arduino transfer and
SPIEx transaction against fake I/O, including frozen-timebase and cleanup cases.

The sections below retain details of the other board integration hooks.

## TIM1 clock ownership

The M4 build compiles a local copy of the pinned STM32duino `timer.c`, replacing
only `enableTimerClock` and `disableTimerClock` with a grant/clock check. M7's RCC
bits remain unchanged on both paths. The rest of the vendor timing and interrupt
code is preserved, including dynamic prescaling and tick-based frequency trims.
The framework installation is not modified. Missing/duplicate hooks or remaining
RCC helper calls cause the adapter to reject the source at build time. Both base
and OC/IC MSP callbacks therefore use the checked hooks, including calls that a
compiler could inline within `timer.c`; a linker wrapper alone would not cover
those reliably.

`hwTimer` performs no peripheral operations before a grant. Initialization,
resume and callbacks also require a healthy radio and an enabled TIM1 clock.
A radio fault stops timer callbacks. Shutdown retains the ownership check even
after a radio fault, and `cuberacerRadioEnd` stops TIM1 before releasing the grant. Subsequent stop/resume/callback calls
cannot write TIM1 after the grant is released.
The radio-disabled build still has no grant caller. The grant/READY protocol
and audit of the remaining full device setup sequence are pending.

The normal radio test script now includes the real `hwTimer` adapter fixture.
To test the pinned vendor clock-hook adaptation:

```sh
STM32_CORE_ROOT=/path/to/framework-arduinoststm32 \
  python3 src/variants/CUBERACER_M4/tests/test_timer_ownership.py
```

These tests compile the real hook bodies with fake RCC operations, reject an
unowned timer/missing grant/missing clock, and verify the source outside the hooks
is unchanged. They supplement the Cortex-M4 build and linked disassembly audit.


## Button and LED ownership

M7 sets PF3 to input with a pull-up and establishes PF13's initial low output
level before granting the radio. On power-off it removes pulls and isolates the
pins before disabling PE10. M4 validates PF3/PF13 modes and pulls, PF13 push-pull
configuration, and the TIM1 clock before accepting a grant.

The existing Button debounce logic reads only PF3 through the adapter. It performs
no generic pin initialization. A missing grant or radio fault suppresses button
actions and clears partial press history, preventing a stale release from
triggering an action in the next session. Binding still uses M7's configuration
authority. The existing LED device uses digitalWrite directly; its initialization
and updates run within the receiver lifecycle. M7 still configures its pin.
The fixed digital indices 81/91 avoid the variant's analog pin aliases.

Reset and antenna selection use the common SX1280/receiver GPIO paths. Reset pin
mode setup happens in generic HAL initialization, while M7 configures it for M4.
RFAMP uses its common implementation; all external PA/TX/RX enable pins are
undefined in this board profile, so it performs no GPIO operations.

The radio tests exercise the common reset and RFAMP code, button debounce and
LED patterns, as well as advisory BUSY handling and SPI failures. They do not
replace physical signal measurements.

## Fixed profile and power ceiling

`OPTIONS/cuberacer_hardware.cpp` provides immutable hardware getters for this
board. It uses the pinned generic variant's digital indices (PA4 NSS is index 4,
PF3 button is 81 and PF13 LED is 91), a single SX1281, SPI4 and switched antennas.
The planned DC-DC network is enabled. PE10 remains M7-only; all M4 serial,
second-radio, PWM, I2C, external PA and optional-device pins are undefined.

CubeRacer `options_init` initializes RAM-only options and names without reading
appended flash data, parsing user hardware JSON or loading a standalone UID.
It runs before the CubeFramework endpoint even in the radio-disabled build.
Betaflight subsequently supplies all receiver settings. The compiled domain
selects ISM/EU-CE behavior; Wi-Fi auto-start and Airport are disabled.

The table contains only PWR_10mW (nominal +10 dBm), with min/max/default at that
level. MatchTX is clamped through the existing power manager. In addition,
SX1280Driver::SetOutputPower caps every CubeRacer request at +10 dBm, including
calibration/direct adjustments. The host driver test commits requested powers
from -30 to +30 dBm through a simulated TX-done interrupt and checks the actual
encoded SetTxParams command. This does not measure conducted RF power.

CubeFramework now supplies radio-control messages and M7's ownership policy;
Betaflight consumes those messages and gates RC/telemetry on RF READY. M4's radio
lifecycle consumer implements start/reconfigure/quiesce hooks. The local radio
build enables startup; current hardware results are in the Betaflight handover.

## M4 lifecycle integration

`CubeFramework/receiver_radio.h` validates M7 session/revision/grant tokens and
owns the main-loop start/reconfigure/stop actions. `receiver_link.cpp` gates RC,
CRSF and RF-ready status separately from settings readiness. It services control
before releasing staged RC and rechecks heartbeat age after a blocking start.
Radio messages are retried behind hello/config acknowledgments; they never renew
the heartbeat. Initial startup requires internal selection and disarmed state.
A running receiver can continue armed. Binding/settings transactions retain the
grant with RC inhibited; a current grant reconfirms RF when settings are ready.

The `rx_main.cpp` hooks use `radio_runtime.h`: grant, one-time device/router
registration, SX1281 Begin, RF settings, timer initialization, RX and a final BUSY
completion check. Settings reconfirmation omits Begin, registration and power
cycling. Only M7 writes PE10. Options and the shared endpoint initialize once at
boot, including in the disabled image.

Shutdown uses no SX1281 commands. It masks/clears the M4 TIM1 and DIO1 interrupts,
stops TIM1/SPI4, deasserts NSS, asserts reset and clears antenna/LED outputs before
ending local ownership. Readback checks those registers before QUIESCENT is
allowed. Failed readback leaves shutdown unacknowledged (no timeout-based power
off or automatic retry). Button press history and OTA/RC assemblies are cleared.
A new challenged session may revoke an old grant even after local quiescence.
Faults prevent another start in the failed session.

The production `CUBERACER_M4_RADIO_DISABLED` define remains enabled and the M7
startup gate remains absent. The separate RF-inclusive image used for compile
verification is not a deployment image. BUSY, rail/TCXO, RF link, conducted power
and live telemetry still require bench validation.

Native lifecycle/runtime fixtures and real M7+M4 adapter tests run from
Betaflight's `src/test/cubefw_integration` CMake suite with matching ELRS/CF roots.
The adapter fixtures replace peripheral/runtime hooks, not the shared transport,
settings client, M7 settings/persistence logic or M4 lifecycle policy. They do not
execute STM32 register operations or prove on-air behavior.
