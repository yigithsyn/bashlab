#!/bin/bash

################################################################################
# dependencies
################################################################################
echo ============================
echo "[INFO] Dependencies ..."
echo ============================
rm -rf libs && mkdir libs

echo "Argtable3: A single-file, ANSI C, command-line parsing library that parses GNU-style command-line options"
curl -L https://github.com/argtable/argtable3/releases/download/v3.2.1.52f24e5/argtable-v3.2.1.52f24e5-amalgamation.tar.gz --output libs/argtable3.tar.gz --silent 
echo "Extracting argtable3 ..."
mkdir libs/argtable3
tar -xf libs/argtable3.tar.gz --directory libs/argtable3

echo "Jansson: C library for encoding, decoding and manipulating JSON data"
curl -L https://github.com/akheron/jansson/releases/download/v2.14/jansson-2.14.tar.gz --output libs/jansson-2.14.tar.gz --silent
tar -xf libs/jansson-2.14.tar.gz --directory libs/
cd libs/jansson-2.14
mkdir build
cd build
cmake -DJANSSON_BUILD_DOCS=OFF ..
# make
# make check
cmake --build . --config Release
cd ../../..

rm -rf build && mkdir build && cd build
################################################################################
# build and install
################################################################################
echo ============================
echo "[INFO] Building ..."
echo ============================
cmake -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_BUILD_TYPE=Release ..
make

echo ============================
echo "[INFO] Installing ..." 
echo ============================
make install 

cd ..


