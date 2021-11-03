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
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" ]]; then
  type cmake > /dev/null 2>&1
  if [[ $? == 0 ]]; then
    echo "CMake found in path."
  else
    echo "CMake not found in path. Checking through filesystem ..."
    CMAKE=$(find "/c" -name "cmake.exe"  2>&1 | grep -v "Permission denied" | head -n 1)
    if [[ $? == 0 ]]; then
      echo "CMake found at: $CMAKE"
      "$CMAKE" -DCMAKE_INSTALL_PREFIX=$USERPROFILE/AppData/Local ..
    else
      cd ..
      exit 1
    fi
  fi
else
  cmake -DCMAKE_BUILD_TYPE=Release ..
  make
  echo "[INFO] Installing"
fi

echo ============================
echo "[INFO] Installing ..." 
echo ============================
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" ]]; then
  "$CMAKE" --build . --target INSTALL --config MinSizeRel
else
  make install --prefix=/usr/local 
fi

cd ..


