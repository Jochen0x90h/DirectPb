# Install the project to ~/.local/bin
#
# Usage:
# 1. python configure.py
# 2. python install.py
#

import subprocess


# configure
subprocess.run(f"cmake --preset x64-windows-static", shell=True)

# build and install to ~/.local/bin
subprocess.run(f"cmake --build build/x64-windows-static --config Debug --target install", shell=True)
