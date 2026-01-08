#!/bin/bash

mkdir -p build-windows
mkdir -p bin_output

echo "Windows derlemesi basliyor..."
cd build-windows

cmake -G "MinGW Makefiles" ..

mingw32-make 

cp TelgrafApp.exe ../bin_output/

cd ..

echo "Windows derlemesi tamamlandi. Dosya bin_output klasorunde."