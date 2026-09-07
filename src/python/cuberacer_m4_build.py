"""Use the CM4 vector assembly instead of the framework's CM7 assembly."""
Import("env")
from pathlib import Path
import sys
sys.path.insert(0, str(Path(env["PROJECT_DIR"]) / "python"))
from cuberacer_timer_source import adapt_timer_source
from cuberacer_spi_source import adapt_spi_source


def owned_peripheral_source(env, node):
    # Compile an adapted copy only for this environment. The pinned framework
    # installation and standalone CubeNode build remain untouched.
    source = Path(node.srcnode().get_abspath()).read_text()
    name = Path(node.srcnode().get_abspath()).stem
    adapted = (adapt_timer_source if name == "timer" else adapt_spi_source)(source)
    destination = Path(env.subst("$BUILD_DIR")) / ("cuberacer_owned_" + name + ".c")
    destination.parent.mkdir(parents=True, exist_ok=True)
    if not destination.exists() or destination.read_text() != adapted:
        destination.write_text(adapted)
    return env.File(str(destination))

env.AddBuildMiddleware(owned_peripheral_source, "*/SrcWrapper/src/stm32/timer.c")
env.AddBuildMiddleware(owned_peripheral_source, "*/SPI/src/utility/spi_com.c")

def skip_framework_startup(env, node):
    return None

env.AddBuildMiddleware(skip_framework_startup, "*/startup_stm32yyxx.S")

env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", env.VerboseAction(
    "$OBJCOPY -O ihex $TARGET $BUILD_DIR/${PROGNAME}.hex",
    "Generating CubeRacer M4 HEX"))
