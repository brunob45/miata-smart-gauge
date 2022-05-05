#!/usr/bin/env python3

import subprocess


def run_cmd(cmd):
    return subprocess.run(cmd, capture_output=True, shell=True, text=True)


# get latest commit hash
result = run_cmd('git rev-parse --short HEAD')
version = result.stdout.strip()

# check if the workspace is dirty
result = run_cmd('git diff --quiet')
if result.returncode > 0:
    #workspace is dirty
    version += '*'

text = """
#ifndef GIT_SHA
#define GIT_SHA "{v}"
#endif
"""

with open('include/git_sha.h', 'w+') as fp:
    fp.write(text.format(v=version))

print(version)
