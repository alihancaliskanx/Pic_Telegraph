#!/bin/bash

cd Pic_Telegraph/ui/ > /dev/null 2>&1

rm -rf build install
mkdir build install

set -e

echo "🐧 Linux verison is compiling..."
cmake --preset linux-native
cmake --build --preset linux
cmake --install build/linux

echo "🪟 Windows verison is compiling (Cross-Compile)..."
cmake --preset windows-cross
cmake --build --preset windows-x
cmake --install build/windows

echo "✅ Process done! you can look at 'install' directory."

sleep 5

clear