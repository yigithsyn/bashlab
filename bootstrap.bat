@ECHO OFF

ECHO ============================
ECHO [INFO] Search for CMake ...
ECHO ============================

SET filename=cmake.exe
FOR /R C:\ %%a IN (\) DO (
   IF EXIST "%%a\%filename%" (
      SET cmake_path=%%a%filename%
      GOTO break
   )
)
:break
ECHO CMake found at: %cmake_path%

@REM FOR /F %%i IN ('%cmake_path% --version') DO set cmake_vers=%%i
@REM for /f "delims=$ tokens=1*" %%i in ('%cmake_path% --version 2^>^&1 1^>nul ') do echo %%i

ECHO ============================
ECHO [INFO] Dependencies ...
ECHO ============================
RMDIR /Q /S libs 
MKDIR libs
ECHO Argtable3: A single-file, ANSI C, command-line parsing library that parses GNU-style command-line options.
curl -L https://github.com/argtable/argtable3/releases/download/v3.2.1.52f24e5/argtable-v3.2.1.52f24e5-amalgamation.tar.gz --output libs\argtable3.tar.gz --silent 
MKDIR libs\argtable3
tar -xf libs\argtable3.tar.gz --directory libs\argtable3

ECHO Jansson: C library for encoding, decoding and manipulating JSON data
curl -L https://github.com/akheron/jansson/releases/download/v2.14/jansson-2.14.tar.gz --output libs\jansson-2.14.tar.gz --silent
tar -xvf libs\jansson-2.14.tar.gz --directory libs\
CD libs\jansson-2.14
MKDIR build
CD build
"%cmake_path%" -DJANSSON_BUILD_SHARED_LIBS=1 -DJANSSON_BUILD_DOCS=OFF ..
"%cmake_path%" --build . --config Release
CD ../../..


ECHO ============================
ECHO [INFO] Building ...
ECHO ============================
RMDIR /Q /S build > nul 2>&1
MKDIR build
CD build

"%cmake_path%" -DCMAKE_INSTALL_PREFIX=%USERPROFILE%\AppData\Local ..


ECHO ============================
ECHO Installing ...
ECHO ============================
"%cmake_path%" --build . --target INSTALL --config Release

CD ..