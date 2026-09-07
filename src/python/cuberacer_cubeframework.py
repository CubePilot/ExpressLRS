"""Compile the same CubeFramework sources and bounded protos used by Betaflight."""
Import("env")
from pathlib import Path
import os
import subprocess
import sys

root=Path(os.environ.get("CUBEFRAMEWORK_ROOT", str(Path(env["PROJECT_DIR"]).parent.parent / "CubeFramework"))).resolve()
if not (root / "CubePilotFW/ReceiverEndpoint.cpp").is_file():
    raise RuntimeError("Set CUBEFRAMEWORK_ROOT to the matching CubeFramework checkout")
generated=Path(env.subst("$BUILD_DIR")) / "cubefw-proto"
generated.mkdir(parents=True,exist_ok=True)
nanopb=root / "third_party/nanopb"
subprocess.run([sys.executable,str(nanopb / "generator/nanopb_generator.py"),
    "-I",str(root / "proto"),"-I",str(nanopb / "generator/proto"),"-D",str(generated),
    *[str(path) for path in sorted((root / "proto").glob("*.proto"))]],check=True)
env.Append(CPPPATH=[str(root),str(nanopb),str(generated)])
env.Append(CPPDEFINES=["CF_PLATFORM_STM32DUINO"])
env.BuildSources("$BUILD_DIR/cubefw-core",str(root / "CubePilotFW"),src_filter=[
    "+<Receiver*.cpp>","+<ShMemTransport.cpp>","+<pal/HSEM.cpp>"])
env.BuildSources("$BUILD_DIR/cubefw-nanopb",str(nanopb),src_filter=[
    "+<pb_common.c>","+<pb_encode.c>","+<pb_decode.c>"])
env.BuildSources("$BUILD_DIR/cubefw-generated",str(generated),src_filter=["+<*.pb.c>"])
