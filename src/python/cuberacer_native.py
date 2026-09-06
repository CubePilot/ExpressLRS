"""Expose the pinned shared native types to CubeRacer host tests."""
Import("env")
import os
from pathlib import Path
root = Path(os.environ.get("CUBEFRAMEWORK_ROOT", str(Path(env["PROJECT_DIR"]).parent.parent / "CubeFramework")))
if (root / "CubePilotFW/ReceiverTypes.h").is_file():
    env.Append(CPPPATH=[str(root)])
