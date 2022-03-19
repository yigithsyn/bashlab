@ECHO OFF

RMDIR /Q /S libs > nul 2>&1 && MKDIR libs
RMDIR /Q /S build > nul 2>&1 && MKDIR build
MKDIR test

ECHO ============================
ECHO [INFO] Dependencies ...
ECHO ============================
:algo
ECHO algo: Simple algorithms
git clone https://github.com/ntessore/algo.git libs/algo
IF "%1"=="algo" EXIT /B 0

IF "%1"=="jansson" GOTO jansson
IF "%1"=="argtable3" GOTO argtable3
IF "%1"=="civetweb" GOTO civetweb
IF "%1"=="libmongoc" GOTO libmongoc
IF "%1"=="serialport" GOTO serialport
IF "%1"=="libantenna" GOTO libantenna
GOTO build
@REM for arguements with quotes
@REM IF "%~1"=="build" GOTO build

:argtable3
ECHO Argtable3: A single-file, ANSI C, command-line parsing library that parses GNU-style command-line options.
curl -L https://github.com/argtable/argtable3/releases/download/v3.2.1.52f24e5/argtable-v3.2.1.52f24e5.tar.gz --output libs\argtable-v3.2.1.52f24e5.tar.gz --silent 
tar -xf libs\argtable-v3.2.1.52f24e5.tar.gz --directory libs\
CD libs\argtable-v3.2.1.52f24e5
MKDIR build
CD build
cmake.exe -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=%USERPROFILE%\AppData\Local -T host=x86 -A win32 ..
cmake.exe --build . --target INSTALL --config Release
CD ../../..
RMDIR /Q /S libs\argtable-v3.2.1.52f24e5
IF "%1"=="argtable3" EXIT /B 0

:jansson
ECHO Jansson: C library for encoding, decoding and manipulating JSON data
curl -L https://github.com/akheron/jansson/releases/download/v2.14/jansson-2.14.tar.gz --output libs\jansson-2.14.tar.gz --silent
tar -xvf libs\jansson-2.14.tar.gz --directory libs\
CD libs\jansson-2.14
MKDIR build
CD build
cmake.exe -DJANSSON_BUILD_DOCS=OFF -DJANSSON_BUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=%USERPROFILE%\AppData\Local -T host=x86 -A win32 ..
cmake.exe --build . --target INSTALL --config Release
CD ../../..
RMDIR /Q /S libs\jansson-2.14
IF "%1"=="jansson" EXIT /B 0



:civetweb
ECHO Civetweb: Embedded C/C++ web server
curl -L https://github.com/civetweb/civetweb/archive/refs/tags/v1.15.tar.gz --output libs\civetweb-1.15.tar.gz --silent
tar -xvf libs\civetweb-1.15.tar.gz --directory libs\
CD libs\civetweb-1.15
MKDIR build_dir
CD build_dir
cmake.exe -DCIVETWEB_ENABLE_SSL=OFF -DCIVETWEB_ENABLE_SERVER_EXECUTABLE=OFF -DCIVETWEB_ENABLE_DEBUG_TOOLS=OFF -DCIVETWEB_BUILD_TESTING=OFF -DBUILD_TESTING=OFF -DCIVETWEB_ENABLE_WEBSOCKETS=ON -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=%USERPROFILE%\AppData\Local -T host=x86 -A win32 ..
cmake.exe --build . --target INSTALL --config Release
CD ../../..
RMDIR /Q /S libs\civetweb-1.15
IF "%1"=="civetweb" EXIT /B 0

:libmongoc
ECHO libmongoc: A high-performance MongoDB driver for C
curl -L https://github.com/mongodb/mongo-c-driver/releases/download/1.21.1/mongo-c-driver-1.21.1.tar.gz --output libs\mongo-c-driver-1.21.1.tar.gz --silent
tar -xvf libs\mongo-c-driver-1.21.1.tar.gz --directory libs\
CD libs\mongo-c-driver-1.21.1
MKDIR build_dir
CD build_dir
@REM For x86, ZLIB ve ICU should be switched off. Otherwise it will not compile properly.
cmake.exe -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DENABLE_ZLIB=OFF -DENABLE_ICU=OFF -DENABLE_MONGODB_AWS_AUTH=OFF -DENABLE_EXAMPLES=OFF -DENABLE_TESTS=OFF -DENABLE_STATIC=OFF -DCMAKE_INSTALL_PREFIX=%USERPROFILE%\AppData\Local -T host=x86 -A Win32 ..
cmake.exe --build . --target INSTALL --config Release
CD ../../..
RMDIR /Q /S libs\mongo-c-driver-1.21.1
IF "%1"=="libmongoc" EXIT /B 0

:serialport
ECHO libserialport: Minimal, cross-platform shared library written in C
curl -L http://sigrok.org/gitweb/?p=libserialport.git;a=snapshot;h=6f9b03e597ea7200eb616a4e410add3dd1690cb1;sf=zip --output libs\libserialport-6f9b03e.zip --silent
powershell.exe -NoP -NonI -Command "Expand-Archive '.\libs\libserialport-6f9b03e.zip' '.\libs\'"
CD libs\libserialport-6f9b03e
msbuild libserialport.sln -t:Rebuild -p:Configuration=Release /p:Platform=x86
copy /Y libserialport.h %USERPROFILE%\AppData\Local\include\
copy /Y Release\libserialport.lib %USERPROFILE%\AppData\Local\lib\
copy /Y Release\libserialport.dll %USERPROFILE%\AppData\Local\bin\
CD ../..
RMDIR /Q /S libs\libserialport-6f9b03e
IF "%1"=="serialport" EXIT /B 0

:libantenna
ECHO libantenna: Antennas and propagation tookit library for C
git clone https://github.com/yigithsyn/libantenna libs/libantenna
CD libs/libantenna
MKDIR build
CD build
cmake.exe -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=%USERPROFILE%\AppData\Local -T host=x86 -A Win32 ..
cmake.exe --build . --target INSTALL --config Release
CD ../../..
RMDIR /Q /S libs\libantenna
IF "%1"=="libantenna" EXIT /B 0

:build
ECHO ============================
ECHO [INFO] Building ...
ECHO ============================
CD build

cmake.exe -DCMAKE_INSTALL_PREFIX=%USERPROFILE%\AppData\Local -T host=x86 -A win32 ..

:install
ECHO ============================
ECHO Installing ...
ECHO ============================
cmake.exe --build . --target INSTALL --config Release

CD ..