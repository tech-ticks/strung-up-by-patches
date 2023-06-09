#!/bin/bash
set -x

cd extracted
ndstool -c ../rom.nds -9 arm9.bin -7 arm7.bin -y9 y9.bin -y7 y7.bin -d data -y overlay -t banner.bin -h header.bin
cd ..
