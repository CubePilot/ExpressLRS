# CubeRacer radio transport

The SX1280-family command driver is shared with other ExpressLRS targets. This
board adapter owns SPI4 runtime registers, PA4 NSS, PF5 reset, PE6 antenna and
PB4's CPU2 interrupt mask/pending flag. BUSY is PE5. M7 owns PE10 power, RCC,
SYSCFG, shared EXTI edge registers and GPIO mode/AF/pull/speed configuration.

`cuberacerRadioGrant(session)` is a future coordinated-startup boundary, not an
inference from valid settings. There is currently **no caller**, and the normal
`CUBERACER_M4_RADIO_DISABLED` build refuses every grant. Removing that define alone
does not supply a grant or a complete startup sequence. The caller must wait for
M7's explicit session-matched power/pin grant after the TCXO settle interval.
M4 readbacks reject incompatible pins, EXTI routing/edges and clocks. The clock
contract is SPI45SEL=PCLK2 at 120 MHz; SPI runs at /8 =15 MHz, MSB-first/mode0.
Other clocks are rejected. The board radio is SX1281 with an assumed fitted
52 MHz TCXO; no GPIO selects TCXO operation or controls its supply separately.

Constructor/init attempts without a grant perform no peripheral writes. After
grant, only the listed peripheral and BSRR registers are written. No Arduino SPI,
pinMode, generic attachInterrupt or RFAMP power hooks are used for these paths.
M7 configures PB4 rising-edge EXTI before releasing M4. The M4 vector references
`CUBERACER_DIO1_IRQHandler`, which touches only PB4's CPU2 pending bit and callback.
Antenna selection uses BSRR, including the ordinary ELRS diversity path.

Transfers are synchronous, byte-wide, bounded to259 bytes, with one NSS assertion
for the entire command and EOT before release. BUSY is checked before any command.
Local PRIMASK nesting protects against TIM1/DIO1 SPI re-entry. Both BUSY and SPI
polling have a1ms elapsed deadline and an iteration ceiling for a stalled clock.
This ceiling is a termination guard, not a guaranteed real-time bound if the clock
has stopped. Normal operation must still be measured against the RF timer budget.
Any timeout/SPI error latches a fault, suppresses later commands and prevents
`SX1280Driver::Begin()` from reporting success, including failure after its final
command. Failed HAL reads return zero bytes. SPI failure releases NSS and disables
SPI while preserving SSI. `end()` holds reset low and relinquishes ownership; it
never powers off PE10 or changes shared configuration. M7 may isolate/power off
only after the future quiescence handshake. Reinitialization cannot clear a fault;
an explicit end followed by a fresh coordinated grant is required.

Run host transport and real SX1280 HAL/Begin tests:

```sh
bash src/lib/CubeRacerRadio/tests/run.sh
```

The fake register/time boundary checks absent ownership, clock rejection,259-byte
framing, BUSY/SPI timeouts, clock wrap/freeze, latched errors, owned output bits and
nested interrupt restoration. HAL tests exercise all command classes on BUSY
failure, normal firmware-register command framing and failure at every command in
Begin. The Cortex-M4 firmware build checks the real CMSIS register bindings.
The disabled build compiles out grant readbacks; compile that source once without
the disabled define as an additional audit, without flashing/enabling startup.

Remaining gates: coordinated grant/revoke and explicit RF READY/fault status;
remaining device initialization paths; conservative SX1281 power-profile
enforcement; physical BUSY diagnosis and SWD bench validation. Passing host tests does not validate RF timing,
board wiring, power/TCXO signal integrity or transmitter interoperability.

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
authority. The existing LED device preserves its patterns and inversion handling
but writes only PF13 through BSRR; no generic GPIO setup/write path runs on M4.
LED fault indication remains available until the grant ends. Both pin adapters
accept the canonical digital indices 81/91 from the fixed board profile.
STM32duino PF3/PF13 macros are analog aliases and must not be passed to the
signed 8-bit LED device; the fixed indices also correct the earlier adapter check.

The radio test script includes real Button and LED-device fixtures as well as
11 transport tests. They cover pre-grant inactivity, granted operation, fault
behavior, revocation, short/long presses and connected/disconnected/binding/fault
LED patterns. These are host tests, not a physical button or LED validation.


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
lifecycle consumer and start/reconfigure/quiesce hooks are still pending. Both
production RF gates remain disabled, and no new firmware has been flashed.

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
