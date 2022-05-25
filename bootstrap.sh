#!/bin/bash


rm -rf build && mkdir build
mkdir "test"

if [ "$1" == "requirements" ]; then
  echo ============================
  echo "[INFO] Requirements ..."
  echo ============================  
  # sudo apt update

  sudo apt remove jq -y
  sudo apt install jq -y

  exit 0
fi

if [ "$1" == "dependencies" ]; then
  echo ============================
  echo "[INFO] Dependencies ..."
  echo ============================
  rm -rf dependencies
  mkdir dependencies

  # C/C++
  echo "algo: Simple algorithms"
  git clone https://github.com/ntessore/algo.git dependencies/algo
  cp dependencies/algo/linspace.c /usr/local/include/linspace.h

  echo "Argtable3: A single-file, ANSI C, command-line parsing library that parses GNU-style command-line options"
  curl -L https://github.com/argtable/argtable3/releases/download/v3.2.1.52f24e5/argtable-v3.2.1.52f24e5.tar.gz --output dependencies/argtable-v3.2.1.52f24e5.tar.gz --silent 
  tar -xf dependencies/argtable-v3.2.1.52f24e5.tar.gz --directory dependencies/
  cd dependencies/argtable-v3.2.1.52f24e5
  mkdir build
  cd build
  cmake -DBUILD_SHARED_LIBS=ON -DARGTABLE3_ENABLE_TESTS=OFF ..
  cmake --build . --config Release
  cmake --install . --prefix /usr/local
  cd ../../..
  rm -rf dependencies/argtable-v3.2.1.52f24e5

  echo "Jansson: C library for encoding, decoding and manipulating JSON data"
  curl -L https://github.com/akheron/jansson/releases/download/v2.14/jansson-2.14.tar.gz --output dependencies/jansson-2.14.tar.gz --silent
  tar -xf dependencies/jansson-2.14.tar.gz --directory dependencies/
  cd dependencies/jansson-2.14
  mkdir build
  cd build
  cmake -DJANSSON_BUILD_DOCS=OFF -DJANSSON_BUILD_SHARED_LIBS=ON -DJANSSON_WITHOUT_TESTS=ON ..
  cmake --build . --config Release
  cmake --install . --prefix /usr/local
  cd ../../..
  rm -rf dependencies/jansson-2.14

  # echo "Civetweb: Embedded C/C++ web server"
  # curl -L https://github.com/civetweb/civetweb/archive/refs/tags/v1.15.tar.gz --output dependencies/civetweb-1.15.tar.gz --silent
  # tar -xvf dependencies/civetweb-1.15.tar.gz --directory dependencies/
  # cd dependencies/civetweb-1.15
  # mkdir build_dir
  # cd build_dir
  # cmake -DCIVETWEB_ENABLE_SSL=OFF -DCIVETWEB_ENABLE_SERVER_EXECUTABLE=OFF -DCIVETWEB_ENABLE_DEBUG_TOOLS=OFF -DCIVETWEB_BUILD_TESTING=OFF -DBUILD_TESTING=OFF -DCIVETWEB_ENABLE_WEBSOCKETS=ON -DBUILD_SHARED_LIBS=ON ..
  # cmake --build . --config Release
  # cmake --install . --prefix /usr/local
  # cd ../../..
  # rm -rf dependencies/civetweb-1.15

  echo "Libmongoc: A high-performance MongoDB driver for C"
  # Dependencies
  # apt update
  # sudo apt-get install cmake libssl-dev libsasl2-dev
  curl -L https://github.com/mongodb/mongo-c-driver/releases/download/1.21.1/mongo-c-driver-1.21.1.tar.gz --output dependencies/mongo-c-driver-1.21.1.tar.gz --silent
  tar -xvf dependencies/mongo-c-driver-1.21.1.tar.gz --directory dependencies/
  cd dependencies/mongo-c-driver-1.21.1
  mkdir build_dir
  cd build_dir
  # In Windows: for x86, ZLIB ve ICU should be switched off. Otherwise it will not compile properly.
  cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DENABLE_MONGODB_AWS_AUTH=OFF -DCMAKE_BUILD_TYPE=Release -DENABLE_EXAMPLES=OFF -DENABLE_TESTS=OFF -DENABLE_STATIC=OFF ..
  cmake --build . --config Release
  cmake --install . --prefix /usr/local
  cd ../../..
  rm -rf dependencies/mongo-c-driver-1.21.1

  echo "Libserialport: Minimal, cross-platform shared library written in C"
  curl -L https://launchpad.net/ubuntu/+archive/primary/+sourcefiles/libserialport/0.1.1-4/libserialport_0.1.1.orig.tar.xz --output dependencies/libserialport-0.1.1-4.tar.xz --silent
  mkdir dependencies/libserialport-0.1.1-4/
  tar -xf dependencies/libserialport-0.1.1-4.tar.xz --directory dependencies/libserialport-0.1.1-4/
  cd dependencies/libserialport-0.1.1-4/
  ./autogen.sh
  ./configure --prefix=/usr/local
  make
  sudo make install
  cd ../../
  rm -rf dependencies/libserialport-0.1.1-4/

  echo "libantenna: Antennas and propagation tookit library for C"
  git clone https://github.com/yigithsyn/libantenna dependencies/libantenna
  cd dependencies/libantenna
  mkdir build
  cd build
  cmake -DBUILD_SHARED_LIBS=ON ..
  cmake --build . --config Release
  cmake --install . --prefix /usr/local
  cd ../../..
  rm -rf dependencies/libantenna

  # Python
  pip install --upgrade pip
  pip install pymongo==4.1.0 
  pip install numpy==1.22.3 
  pip install pyinstaller==4.10 

  ldconfig
  rm -rf dependencies
fi

rm -rf build && mkdir build && cd build
################################################################################
# build and install
################################################################################
echo ============================
echo "[INFO] Building ..."
echo ============================
# C/C++
cmake -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_BUILD_TYPE=Release ..
make
cd ..

# Python
# pyinstaller --onefile main/ams2nsi.py
# rm -rf main/__pycache__
# rm ams2nsi.spec


echo ============================
echo "[INFO] Installing ..." 
echo ============================
cat list.json | jq -c '.[] | select(.osOnly == false)' > list2.json
mongo bashlab --eval "db.programs.drop()"
mongoimport --db bashlab --collection programs --file list2.json
mongo bashlab --eval "db.programs.find().length()"
rm -rf list2.json

# C/C++
cd build
make install 
cd .. 
rm -rf build

# Python
# cp dist/ams2nsi /usr/local/bin/
# rm -rf dist