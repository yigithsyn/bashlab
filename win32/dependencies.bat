@ECHO OFF

ECHO =========================================================================
ECHO [INFO] Dependencies ...
ECHO =========================================================================

RMDIR /Q /S dependencies > nul 2>&1 
MKDIR dependencies

IF [%1]==[] (
  SET BASHLAB_ARCH=32
  SET BASHLAB_CMAKE_ARCH=-T host=x86 -A Win32
)
IF "%1"=="x64" (
  SET BASHLAB_ARCH=64
  SET BASHLAB_CMAKE_ARCH=''
)


SET BASHLAB_INSTALL_DIR=%LocalAppData%\Programs\bashlab-%BASHLAB_ARCH%
SET BASHLAB_INCLUDE_DIR=%BASHLAB_INSTALL_DIR%\include
SET BASHLAB_LIBRARY_DIR=%BASHLAB_INSTALL_DIR%\lib
SET BASHLAB_BINARY_DIR=%BASHLAB_INSTALL_DIR%\bin

RMDIR /Q /S %BASHLAB_INSTALL_DIR% > nul 2>&1

MKDIR %BASHLAB_INSTALL_DIR%
MKDIR %BASHLAB_INCLUDE_DIR%
MKDIR %BASHLAB_LIBRARY_DIR%
MKDIR %BASHLAB_BINARY_DIR%

@REM C/C++
ECHO algo: Simple algorithms
git clone https://github.com/ntessore/algo.git dependencies\algo
COPY /Y dependencies\algo\linspace.c %BASHLAB_INCLUDE_DIR%\linspace.h
RMDIR /Q /S dependencies\algo

ECHO Argtable3: A single-file, ANSI C, command-line parsing library that parses GNU-style command-line options.
curl -L https://github.com/argtable/argtable3/releases/download/v3.2.1.52f24e5/argtable-v3.2.1.52f24e5.tar.gz --output dependencies\argtable-v3.2.1.52f24e5.tar.gz --silent 
tar -xf dependencies\argtable-v3.2.1.52f24e5.tar.gz --directory dependencies\
CD dependencies\argtable-v3.2.1.52f24e5
MKDIR build
CD build
cmake.exe -DBUILD_SHARED_LIBS=ON -DARGTABLE3_ENABLE_TESTS=OFF -DCMAKE_INSTALL_PREFIX=%BASHLAB_INSTALL_DIR% %BASHLAB_CMAKE_ARCH% ..
cmake.exe --build . --target INSTALL --config Release
CD ../../..
RMDIR /Q /S dependencies\argtable-v3.2.1.52f24e5

ECHO Jansson: C library for encoding, decoding and manipulating JSON data
curl -L https://github.com/akheron/jansson/releases/download/v2.14/jansson-2.14.tar.gz --output dependencies\jansson-2.14.tar.gz --silent
tar -xvf dependencies\jansson-2.14.tar.gz --directory dependencies\
CD dependencies\jansson-2.14
MKDIR build
CD build
cmake.exe -DJANSSON_BUILD_DOCS=OFF -DJANSSON_BUILD_SHARED_LIBS=ON -DJANSSON_WITHOUT_TESTS=ON -DCMAKE_INSTALL_PREFIX=%BASHLAB_INSTALL_DIR% %BASHLAB_CMAKE_ARCH% ..
cmake.exe --build . --target INSTALL --config Release
CD ../../..
RMDIR /Q /S dependencies\jansson-2.14

@REM ECHO Civetweb: Embedded C/C++ web server
@REM curl -L https://github.com/civetweb/civetweb/archive/refs/tags/v1.15.tar.gz --output dependencies\civetweb-1.15.tar.gz --silent
@REM tar -xvf dependencies\civetweb-1.15.tar.gz --directory dependencies\
@REM CD dependencies\civetweb-1.15
@REM MKDIR build_dir
@REM CD build_dir
@REM cmake.exe -DCIVETWEB_ENABLE_SSL=OFF -DCIVETWEB_ENABLE_SERVER_EXECUTABLE=OFF -DCIVETWEB_ENABLE_DEBUG_TOOLS=OFF -DCIVETWEB_BUILD_TESTING=OFF -DBUILD_TESTING=OFF -DCIVETWEB_ENABLE_WEBSOCKETS=ON -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=%USERPROFILE%\AppData\Local ..
@REM cmake.exe --build . --target INSTALL --config Release
@REM CD ../../..
@REM RMDIR /Q /S dependencies\civetweb-1.15

ECHO libmongoc: A high-performance MongoDB driver for C
curl -L https://github.com/mongodb/mongo-c-driver/releases/download/1.21.1/mongo-c-driver-1.21.1.tar.gz --output dependencies\mongo-c-driver-1.21.1.tar.gz --silent
tar -xvf dependencies\mongo-c-driver-1.21.1.tar.gz --directory dependencies\
CD dependencies\mongo-c-driver-1.21.1
MKDIR build_dir
CD build_dir
@REM For x86, ZLIB ve ICU should be switched off. Otherwise it will not compile properly.
cmake.exe -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DENABLE_ZLIB=OFF -DENABLE_ICU=OFF -DENABLE_MONGODB_AWS_AUTH=OFF -DENABLE_EXAMPLES=OFF -DENABLE_TESTS=OFF -DENABLE_STATIC=OFF -DCMAKE_INSTALL_PREFIX=%BASHLAB_INSTALL_DIR% %BASHLAB_CMAKE_ARCH% ..
cmake.exe --build . --target INSTALL --config Release
CD ../../..
RMDIR /Q /S dependencies\mongo-c-driver-1.21.1

ECHO libserialport: Minimal, cross-platform shared library written in C
curl -L http://sigrok.org/gitweb/?p=libserialport.git;a=snapshot;h=6f9b03e597ea7200eb616a4e410add3dd1690cb1;sf=zip --output dependencies\libserialport-6f9b03e.zip --silent
powershell.exe -NoP -NonI -Command "Expand-Archive '.\dependencies\libserialport-6f9b03e.zip' '.\dependencies\'"
CD dependencies\libserialport-6f9b03e
msbuild.exe libserialport.sln -t:Rebuild -p:Configuration=Release /p:Platform=x%BASHLAB_ARCH%
COPY /Y libserialport.h %BASHLAB_INCLUDE_DIR%\
COPY /Y x64\Release\libserialport.lib %BASHLAB_LIBRARY_DIR%\
COPY /Y x64\Release\libserialport.dll %BASHLAB_BINARY_DIR%\
CD ../..
@REM RMDIR /Q /S dependencies\libserialport-6f9b03e

ECHO libantenna: Antennas and propagation tookit library for C
git clone https://github.com/yigithsyn/libantenna dependencies/libantenna
CD dependencies/libantenna
MKDIR build
CD build
cmake.exe -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=%BASHLAB_INSTALL_DIR% %BASHLAB_CMAKE_ARCH% ..
cmake.exe --build . --target INSTALL --config Release
CD ../../..
RMDIR /Q /S dependencies\libantenna


@REM Python
pip install --no-warn-script-location --upgrade pip --user
pip install --no-warn-script-location pymongo==4.1.0 
pip install --no-warn-script-location numpy==1.22.3 
pip install --no-warn-script-location pyinstaller==4.10 

RMDIR /Q /S dependencies\