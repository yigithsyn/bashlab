@ECHO OFF

MKDIR test > nul 2>&1

IF "%1"=="requirements" (
  ECHO =========================================================================
  ECHO [INFO] Requirements ...
  ECHO =========================================================================
  RMDIR /Q /S requirements > nul 2>&1 
  MKDIR requirements

  @REM C/C++
  @REM Install Visual Studio Community Edition and C++ Desktop Toolset

  ECHO -------------------------------------------------------------------------
  ECHO [INFO] Requirements: Python 3.8
  ECHO -------------------------------------------------------------------------
  winget uninstall Python.Python.3
  curl -L https://www.python.org/ftp/python/3.8.10/python-3.8.10-amd64.exe --output requirements\python-3.8.10-amd64.exe
  requirements\python-3.8.10-amd64.exe /quiet InstallAllUsers=0 PrependPath=1
  RMDIR /Q /S requirements > nul 2>&1

  ECHO -------------------------------------------------------------------------
  ECHO [INFO] Requirements: Jq: Lightweight and flexible command-line JSON processor
  ECHO -------------------------------------------------------------------------
  DEL /F /Q %USERPROFILE%\AppData\Local\bin\jq.exe
  curl -L https://github.com/stedolan/jq/releases/download/jq-1.6/jq-win64.exe --output %USERPROFILE%\AppData\Local\bin\jq.exe
  
  EXIT /B 0
)

IF "%1"=="dependencies" (
  ECHO =========================================================================
  ECHO [INFO] Dependencies ...
  ECHO =========================================================================

  vcvars64.bat

  RMDIR /Q /S dependencies > nul 2>&1 
  MKDIR dependencies

  RMDIR /Q /S %USERPROFILE%\AppData\Local\bin
  RMDIR /Q /S %USERPROFILE%\AppData\Local\include
  RMDIR /Q /S %USERPROFILE%\AppData\Local\lib

  MKDIR %USERPROFILE%\AppData\Local\bin
  MKDIR %USERPROFILE%\AppData\Local\include
  MKDIR %USERPROFILE%\AppData\Local\lib

  @REM C/C++
  ECHO algo: Simple algorithms
  git clone https://github.com/ntessore/algo.git dependencies\algo
  COPY /Y dependencies\algo\linspace.c %USERPROFILE%\AppData\Local\include\linspace.h
  RMDIR /Q /S dependencies\algo

  ECHO Argtable3: A single-file, ANSI C, command-line parsing library that parses GNU-style command-line options.
  curl -L https://github.com/argtable/argtable3/releases/download/v3.2.1.52f24e5/argtable-v3.2.1.52f24e5.tar.gz --output dependencies\argtable-v3.2.1.52f24e5.tar.gz --silent 
  tar -xf dependencies\argtable-v3.2.1.52f24e5.tar.gz --directory dependencies\
  CD dependencies\argtable-v3.2.1.52f24e5
  MKDIR build
  CD build
  cmake.exe -DBUILD_SHARED_LIBS=ON -DARGTABLE3_ENABLE_TESTS=OFF -DCMAKE_INSTALL_PREFIX=%USERPROFILE%\AppData\Local ..
  cmake.exe --build . --target INSTALL --config Release
  CD ../../..
  RMDIR /Q /S dependencies\argtable-v3.2.1.52f24e5

  ECHO Jansson: C library for encoding, decoding and manipulating JSON data
  curl -L https://github.com/akheron/jansson/releases/download/v2.14/jansson-2.14.tar.gz --output dependencies\jansson-2.14.tar.gz --silent
  tar -xvf dependencies\jansson-2.14.tar.gz --directory dependencies\
  CD dependencies\jansson-2.14
  MKDIR build
  CD build
  cmake.exe -DJANSSON_BUILD_DOCS=OFF -DJANSSON_BUILD_SHARED_LIBS=ON -DJANSSON_WITHOUT_TESTS=ON -DCMAKE_INSTALL_PREFIX=%USERPROFILE%\AppData\Local ..
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
  cmake.exe -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DENABLE_ZLIB=OFF -DENABLE_ICU=OFF -DENABLE_MONGODB_AWS_AUTH=OFF -DENABLE_EXAMPLES=OFF -DENABLE_TESTS=OFF -DENABLE_STATIC=OFF -DCMAKE_INSTALL_PREFIX=%USERPROFILE%\AppData\Local ..
  cmake.exe --build . --target INSTALL --config Release
  CD ../../..
  RMDIR /Q /S dependencies\mongo-c-driver-1.21.1

  ECHO libserialport: Minimal, cross-platform shared library written in C
  curl -L http://sigrok.org/gitweb/?p=libserialport.git;a=snapshot;h=6f9b03e597ea7200eb616a4e410add3dd1690cb1;sf=zip --output dependencies\libserialport-6f9b03e.zip --silent
  powershell.exe -NoP -NonI -Command "Expand-Archive '.\dependencies\libserialport-6f9b03e.zip' '.\dependencies\'"
  CD dependencies\libserialport-6f9b03e
  msbuild.exe libserialport.sln -t:Rebuild -p:Configuration=Release /p:Platform=x64
  COPY /Y libserialport.h %USERPROFILE%\AppData\Local\include\
  COPY /Y x64\Release\libserialport.lib %USERPROFILE%\AppData\Local\lib\
  COPY /Y x64\Release\libserialport.dll %USERPROFILE%\AppData\Local\bin\
  CD ../..
  @REM RMDIR /Q /S dependencies\libserialport-6f9b03e

  ECHO libantenna: Antennas and propagation tookit library for C
  git clone https://github.com/yigithsyn/libantenna dependencies/libantenna
  CD dependencies/libantenna
  MKDIR build
  CD build
  cmake.exe -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=%USERPROFILE%\AppData\Local ..
  cmake.exe --build . --target INSTALL --config Release
  CD ../../..
  RMDIR /Q /S dependencies\libantenna
 

  @REM Python
  %USERPROFILE%\AppData\Local\Programs\Python\Python38\Scripts\pip install --upgrade pip --user
  %USERPROFILE%\AppData\Local\Programs\Python\Python38\Scripts\pip install pymongo==4.1.0 
  %USERPROFILE%\AppData\Local\Programs\Python\Python38\Scripts\pip install numpy==1.22.3 
  %USERPROFILE%\AppData\Local\Programs\Python\Python38\Scripts\pip install pyinstaller==4.10 

  RMDIR /Q /S dependencies\
)



:build
ECHO ============================
ECHO [INFO] Building ...
ECHO ============================
TYPE list.json | jq -c ".[] | select(.osOnly == false)" > list2.json
mongo bashlab --eval "db.programs.drop()"
mongoimport --db bashlab --collection programs --file list2.json
mongo bashlab --eval "db.programs.find().length()"
DEL list2.json

RMDIR /Q /S dist > nul 2>&1 
MKDIR dist
RMDIR /Q /S build > nul 2>&1
MKDIR build

@REM C/C++
CD build
cmake.exe -DCMAKE_INSTALL_PREFIX=%USERPROFILE%\AppData\Local ..
CD ..

@REM Python
%USERPROFILE%\AppData\Local\Programs\Python\Python38\Scripts\pyinstaller --onefile main\ams2nsi.py
DEL ams2nsi.spec
RMDIR /Q /S main\__pycache__ > nul 2>&1 

:install
ECHO ============================
ECHO Installing ...
ECHO ============================
@REM C/C++
CD build
cmake.exe --build . --target INSTALL --config Release
RMDIR /Q /S build > nul 2>&1
CD ..

@REM Python
COPY /Y dist\*.exe %USERPROFILE%\AppData\Local\bin\

RMDIR /Q /S dist > nul 2>&1 
RMDIR /Q /S build > nul 2>&1 