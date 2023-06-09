#!/bin/bash
set -x

rm -rf extracted
mkdir -p extracted
cd extracted
ndstool -x ../rom.nds -9 arm9.bin -7 arm7.bin -y9 y9.bin -y7 y7.bin -d data -y overlay -t banner.bin -h header.bin
cd ..
