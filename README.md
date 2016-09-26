jj2-bridge
===
A small program that acts as stdin for a running JJ2 process. You can use it to send chat and commands.

Build dependencies
---
For cross-compiling on Arch Linux:
` # pacman -S mingw-w64-gcc`

Run `make` to build.

Usage
---
On Linux (with charset conversion):

`cat | while IFS=$'\n' read -r line ; do echo "$line" | iconv -f UTF-8 -t MS-ANSI -c ; done | wine jj2-bridge.exe`
