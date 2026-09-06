"""Use the CM4 vector assembly instead of the framework's CM7 assembly."""
Import("env")

def skip_framework_startup(env, node):
    return None

env.AddBuildMiddleware(skip_framework_startup, "*/startup_stm32yyxx.S")

env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", env.VerboseAction(
    "$OBJCOPY -O ihex $TARGET $BUILD_DIR/${PROGNAME}.hex",
    "Generating CubeRacer M4 HEX"))
