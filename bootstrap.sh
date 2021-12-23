#!/bin/bash

if [ "$1" != "build" ]; then
  ################################################################################
  # dependencies
  ################################################################################
  echo ============================
  echo "[INFO] Dependencies ..."
  echo ============================
  rm -rf libs && mkdir libs

  echo "Argtable3: A single-file, ANSI C, command-line parsing library that parses GNU-style command-line options"
  curl -L https://github.com/argtable/argtable3/releases/download/v3.2.1.52f24e5/argtable-v3.2.1.52f24e5.tar.gz --output libs/argtable-v3.2.1.52f24e5.tar.gz --silent 
  tar -xf libs/argtable-v3.2.1.52f24e5.tar.gz --directory libs/
  cd libs/argtable-v3.2.1.52f24e5
  mkdir build
  cd build
  cmake ..
  cmake --build . --config Release
  cd ../../..

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

  echo "algo: Simple algorithms"
  git clone https://github.com/ntessore/algo.git libs/algo

  echo ============================
  echo "[INFO] Dependecies: Civetweb: BuEmbedded C/C++ web server"
  echo ============================
  curl -L https://github.com/civetweb/civetweb/archive/refs/tags/v1.15.tar.gz --output libs/civetweb-1.15.tar.gz --silent
  tar -xvf libs/civetweb-1.15.tar.gz --directory libs/
  cd libs/civetweb-1.15
  mkdir build_dir
  cd build_dir
  cmake -DCIVETWEB_ENABLE_SSL=OFF -DCIVETWEB_ENABLE_SERVER_EXECUTABLE=OFF -DCIVETWEB_ENABLE_DEBUG_TOOLS=OFF -DCIVETWEB_BUILD_TESTING=OFF -DBUILD_TESTING=OFF -DCIVETWEB_ENABLE_WEBSOCKETS=ON -DBUILD_SHARED_LIBS=ON ..
  cmake --build . --config Release
  cd ../../..

fi

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


