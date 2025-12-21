#!/bin/bash

mkdir -p build-linux
mkdir -p bin_output

cd build-linux
cmake ..
make
cp TelgrafApp ../bin_output/
cd ..

echo "Code compiled successfully"