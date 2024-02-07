#!/bin/bash

export MAKEFLAGS=-j$(nproc)

git clone https://github.com/ultra0000/PS3-SDL2 /sdl2

cd /sdl2
chmod +x ./configure
./configure --host=powerpc64-ps3-elf --enable-static --disable-shared 
make

cp -r include/ $PSL1GHT/portlibs/ppu/include/SDL2
cp build/.libs/* $PSL1GHT/portlibs/ppu/lib

cd /RefresherPS3

make clean
make
make pkg