#!/bin/sh

rm -rf build

mkdir build
cd build

if [ $# -eq 1 ] && [ $1 -eq 32 ]
then
    CFLAGS="-W -Wall -Wextra -std=gnu99 -nostdlib -fno-builtin -O3 -m32 " CXXFLAGS=-m32 cmake -DDynamoRIO_DIR=$DYNAMORIO_HOME/cmake ../ && make && cp libvilloc_tracer.so ../villoc_tracer
else
    CFLAGS="-W -Wall -Wextra -std=gnu99 -nostdlib -fno-builtin -O3 " cmake -DDynamoRIO_DIR=$DYNAMORIO_HOME/cmake ../ && make && cp libvilloc_tracer.so ../villoc_tracer
fi
