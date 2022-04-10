@ECHO OFF

RMDIR /Q /S dist > nul 2>&1 
MKDIR dist
RMDIR /Q /S requirements > nul 2>&1 
MKDIR requirements
RMDIR /Q /S libs > nul 2>&1 
MKDIR libs
RMDIR /Q /S build > nul 2>&1
MKDIR build
MKDIR test > nul 2>&1

ECHO ============================
ECHO [INFO] Requirements ...
ECHO ============================
IF "%1"=="requirements" (
  @REM C/C++
  @REM Install Visual Studio Community Edition and C++ Desktop Toolset

  @REM Python 3.8
  winget uninstall Python.Python.3
  curl -L https://www.python.org/ftp/python/3.8.10/python-3.8.10.exe --output requirements\python-3.8.10.exe --silent
  requirements\python-3.8.10.exe /quiet InstallAllUsers=0 PrependPath=1
  RMDIR /Q /S requirements > nul 2>&1 
)

ECHO ============================
ECHO [INFO] Dependencies ...
ECHO ============================
IF "%1"=="dependencies" (
  @REM C/C++
  ECHO algo: Simple algorithms
  git clone https://github.com/ntessore/algo.git libs/algo

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

  ECHO libserialport: Minimal, cross-platform shared library written in C
  curl -L http://sigrok.org/gitweb/?p=libserialport.git;a=snapshot;h=6f9b03e597ea7200eb616a4e410add3dd1690cb1;sf=zip --output libs\libserialport-6f9b03e.zip --silent
  powershell.exe -NoP -NonI -Command "Expand-Archive '.\libs\libserialport-6f9b03e.zip' '.\libs\'"
  CD libs\libserialport-6f9b03e
  msbuild.exe libserialport.sln -t:Rebuild -p:Configuration=Release /p:Platform=x86
  COPY /Y libserialport.h %USERPROFILE%\AppData\Local\include\
  COPY /Y Release\libserialport.lib %USERPROFILE%\AppData\Local\lib\
  COPY /Y Release\libserialport.dll %USERPROFILE%\AppData\Local\bin\
  CD ../..
  RMDIR /Q /S libs\libserialport-6f9b03e

  ECHO libantenna: Antennas and propagation tookit library for C
  git clone https://github.com/yigithsyn/libantenna libs/libantenna
  CD libs/libantenna
  MKDIR build
  CD build
  cmake.exe -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=%USERPROFILE%\AppData\Local -T host=x86 -A Win32 ..
  cmake.exe --build . --target INSTALL --config Release
  CD ../../..
  RMDIR /Q /S libs\libantenna

  @REM Python
  %USERPROFILE%\AppData\Local\Programs\Python\Python38-32\Scripts\pip install --upgrade pip --user
  %USERPROFILE%\AppData\Local\Programs\Python\Python38-32\Scripts\pip install pymongo==4.1.0 
  %USERPROFILE%\AppData\Local\Programs\Python\Python38-32\Scripts\pip install numpy==1.22.3 
  %USERPROFILE%\AppData\Local\Programs\Python\Python38-32\Scripts\pip install pyinstaller==4.10 

  EXIT /B 0

)



:build
ECHO ============================
ECHO [INFO] Building ...
ECHO ============================
@REM C/C++
CD build
cmake.exe -DCMAKE_INSTALL_PREFIX=%USERPROFILE%\AppData\Local -T host=x86 -A win32 ..
CD ..

@REM Python
%USERPROFILE%\AppData\Local\Programs\Python\Python38-32\Scripts\pyinstaller --onefile main\ams2nsi.py
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