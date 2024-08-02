#!/usr/bin/env python3

import subprocess
from pathlib import Path
import shutil

# get latest commit hash
result = subprocess.run("git rev-parse --short HEAD", capture_output=True, text=True)
version = result.stdout.strip()

# check if the workspace is dirty
result = subprocess.run("git diff --quiet", capture_output=True)
if result.returncode > 0:
    # workspace is dirty
    version += "*"

text = """
#ifndef GIT_SHA
#define GIT_SHA "{v}"
#endif
"""

Path("lib/git_sha").mkdir(parents=True, exist_ok=True)

with open("lib/git_sha/git_sha.h", "w+") as fp:
    fp.write(text.format(v=version))

print(version)

shutil.copyfile("include/lv_conf", ".pio/libdeps/default/lvgl/")
# Path("include/lv_conf.h").cp
