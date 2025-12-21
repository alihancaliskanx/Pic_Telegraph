cd build-linux
cmake ..
make
cp TelgrafApp ../bin_output/
cd ..

cd build-windows
x86_64-w64-mingw32-cmake ..
make
cp TelgrafApp.exe ../bin_output/
cd ..

echo "Code compiled successfully"