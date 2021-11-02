@ECHO OFF

RMDIR /Q /S libs 
MKDIR libs
ECHO "[INFO] Downloading argtable3 ..."
curl -L https://github.com/argtable/argtable3/releases/download/v3.2.1.52f24e5/argtable-v3.2.1.52f24e5-amalgamation.tar.gz --output libs\argtable3.tar.gz  
MKDIR libs\argtable3
tar -xvf libs\argtable3.tar.gz --directory libs\argtable3


RMDIR /Q /S build
MKDIR build
CD build

SET filename=cmake.exe
FOR /R C:\ %%a IN (\) DO (
   IF EXIST "%%a\%filename%" (

      SET cmake_path=%%a%filename%
      GOTO break
   )
)
:break

@REM FOR /F %%i IN ('%cmake_path% --version') DO set cmake_vers=%%i
@REM for /f "delims=$ tokens=1*" %%i in ('%cmake_path% --version 2^>^&1 1^>nul ') do echo %%i

ECHO ============================
ECHO Building ...
ECHO ============================
ECHO CMake path   : %cmake_path%
ECHO CMake version: 
'%cmake_path% --version'

reg Query "HKLM\Hardware\Description\System\CentralProcessor\0" | find /i "x86" > NUL && set OS=32BIT || set OS=64BIT

if %OS%==32BIT echo This is a 32bit operating system
if %OS%==64BIT echo This is a 64bit operating system

if %OS%==32BIT "%cmake_path%" -DCMAKE_INSTALL_PREFIX=%USERPROFILE%\AppData\Local .. -T host=x86 -A Win32
if %OS%==64BIT "%cmake_path%" -DCMAKE_INSTALL_PREFIX=%USERPROFILE%\AppData\Local .. -T host=x64 -A x64

"%cmake_path%" --build . --config Debug

ECHO ============================
ECHO Installing ...
ECHO ============================
"%cmake_path%" --build . --target INSTALL --config MinSizeRel

CD ..