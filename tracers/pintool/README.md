# Villoc Pintool
This is a pintool which aims to create an ltrace compatible output for heap management functions
which can be visualised by villoc.
# Build and Run
To Build run:
```shell
make PIN_ROOT=~/code/pin-2.14-71313-gcc.4.4.7-linux
```
Where PIN_ROOT is the path to your pin directory.
Then to run:
```shell
~/code/pin-2.14-71313-gcc.4.4.7-linux/pin.sh -t obj-intel64/pintool.so -- $cmd
```
Add `-injection child` if the operating system configuration prevents Pin from using the
default (parent) injection mode.

Where "~/code/pin-2.14-71313-gcc.4.4.7-linux/pin" should be replaced with your path to pin.
By default the trace will be written to the 'trace' file in the current directory,
the -o flag can be used to change this.

