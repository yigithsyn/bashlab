#!/bin/bash

################################################################################
# dependencies
################################################################################
echo "[INFO] Downloading argtable3 ..."
rm -rf libs && mkdir libs
curl -L https://github.com/argtable/argtable3/releases/download/v3.2.1.52f24e5/argtable-v3.2.1.52f24e5-amalgamation.tar.gz --output libs/argtable3.tar.gz  
mkdir libs/argtable3
tar -xvf libs/argtable3.tar.gz --directory libs/argtable3

################################################################################
# build and install
################################################################################
echo "[INFO] Building"
rm -rf build && mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
echo "[INFO] Installing"
make install --prefix=/usr/local 





