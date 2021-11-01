#!/bin/bash

################################################################################
# dependencies
################################################################################
rm -rf libs && mkdir libs
echo "[INFO] Downloading argtable3 ..."
curl -L https://github.com/argtable/argtable3/releases/download/v3.2.1.52f24e5/argtable-v3.2.1.52f24e5-amalgamation.tar.gz --output libs/argtable3.tar.gz  
mkdir libs/argtable3
tar -xvf libs/argtable3.tar.gz --directory libs/argtable3

################################################################################
# build and install
################################################################################
# if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" ]]; then
#   echo "[INFO] Platform: WIN32"
#   rm -rf build && mkdir build && cd build
#   rm -rf CMakeCache.txt
#   "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -G "Visual Studio 16 2019" -T host=x86 -A win32 ..
#   "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build . --target INSTALL --config Release
# else
#   echo "[INFO] Platform: UNIX"
#   cmake -DCMAKE_BUILD_TYPE=Release ..
#   make
#   make install 
# fi





