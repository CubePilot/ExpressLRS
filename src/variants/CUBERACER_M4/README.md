# CubeRacer M4 receiver — build checkpoint

Build `pio run -d src -e Unified_CubeRacer_2400_RX_M4` with PlatformIO 6.1.18.
Exact framework and compiler revisions are recorded in dependencies.json.
The environment emits ELF, BIN and HEX in its PlatformIO build directory.
Use Betaflight's `tools/cuberacer/check_images.py --core m4 firmware.elf`
before deployment. This target deliberately has no standalone uploader.

The existing STM32duino CubeNode port and SX1280 driver compile for Cortex-M4
with hard-float FPv4-SP. The checked-in ST startup assembly comes from the
pinned framework; its CPU directive is changed to cortex-m4. The build excludes
the framework's original startup object. The M4 vector table admits only EXTI4,
TIM1 update/compare and HSEM2 peripheral handlers; all other peripheral vectors
are reserved or point to the local default trap. Betaflight's ELF validator
checks this ownership policy, including Thumb handler addresses.
Linker wrapping replaces SystemInit
and ExitRun0Mode before data initialization and constructors. Arduino init only
reads inherited clock configuration and initializes the local SysTick.

Flash is 0x08180000–0x081fffff; private SRAM1 is 0x30000000–0x3001ffff,
including separate 16 KiB minimum heap and stack budgets. SRAM4 is excluded.
M7 must establish the shared clocks and release M4 at this image's vector.

Set CUBEFRAMEWORK_ROOT to the matching CubeFramework checkout (currently the
impl/cuberacer-m4 branch). Install the pinned tools in requirements.txt. The PRE
build script generates protobuf C files and compiles the same shared sources as
Betaflight. Source dependency publication is the final integration step.

At this checkpoint setup initializes the CubeFramework endpoint and loop polls
its bounded handshake before waiting for interrupts.
No radio/UART/GPIO startup runs. PB0/PA5 timing probes are disabled and TIM1
initialization is deferred out of static constructors. Legacy EEPROM entry
points trap on M4; the acknowledged configuration adapter sends durable changes
to Betaflight and never saves locally. User/super defines and appended configuration
are excluded so the image cannot inherit a standalone binding identity.

Host startup test: `python3 src/variants/CUBERACER_M4/tests/test_startup.py`.
The dual-core handshake has run on hardware with radio disabled. Normal RF startup
remains gated by the SX1281 BUSY investigation. The M4 lifecycle now handles
current grants, RF readiness, settings reconfirmation and checked quiescence; see
`src/lib/CubeRacerRadio/README.md`. Both core adapters run together in host tests
with fake peripheral hooks. No RF-inclusive image has been flashed.
Task 13 adds lost-ACK and armed-apply regression tests; full cross-layer tests and
the active-image ownership audit are recorded in Betaflight's
`docs/development/cuberacer-receiver-validation.md`.
