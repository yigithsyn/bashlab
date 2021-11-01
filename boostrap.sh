#!/bin/bash

if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" ]]; then
  echo "[INFO] Platform: WIN32"
  # git submodule add https://github.com/microsoft/vcpkg.git libs/vcpkg
else
  echo "[INFO] Platform: UNIX"
fi

mkdir libs
echo "[INFO] Downloading argtable3 ..."
curl -L https://github.com/argtable/argtable3/releases/download/v3.2.1.52f24e5/argtable-v3.2.1.52f24e5-amalgamation.tar.gz --output libs/argtable3.tar.gz  
mkdir libs/argtable3
tar -xvf libs/argtable3.tar.gz --directory libs/argtable3