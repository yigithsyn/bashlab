#!/bin/bash

################################################################################
# dependencies
################################################################################
echo ============================
echo "[INFO] Dependencies ..."
echo ============================
echo "Downloading argtable3 ..."
rm -rf libs && mkdir libs
curl -L https://github.com/argtable/argtable3/releases/download/v3.2.1.52f24e5/argtable-v3.2.1.52f24e5-amalgamation.tar.gz --output libs/argtable3.tar.gz --silent 
echo "Extracting argtable3 ..."
mkdir libs/argtable3
tar -xf libs/argtable3.tar.gz --directory libs/argtable3

################################################################################
# build and install
################################################################################
echo ============================
echo "[INFO] Building ..."
echo ============================
rm -rf build && mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_BUILD_TYPE=Release ..
make

echo ============================
echo "[INFO] Installing ..." 
echo ============================
make install 

cd ..


