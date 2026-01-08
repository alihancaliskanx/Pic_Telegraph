#!/bin/bash

# Önce temizlik
rm -rf build-linux build-windows bin_output
mkdir -p build-linux build-windows bin_output

echo "--- 1. Linux Sürümü Derleniyor ---"
cd build-linux
cmake ..
make -j$(nproc)
cp TelgrafApp ../bin_output/TelgrafApp_Linux
cd ..

echo "--- 2. Windows (.exe) Sürümü Derleniyor ---"
cd build-windows
# Arch'taki mingw paketi bu özel komutu sağlar, otomatik olarak her şeyi Windows'a göre ayarlar
x86_64-w64-mingw32-cmake ..
make -j$(nproc)
cp TelgrafApp.exe ../bin_output/TelgrafApp.exe
cd ..

echo "--- İŞLEM TAMAM ---"
echo "Çıktılar 'bin_output' klasöründe:"
ls -lh bin_output/