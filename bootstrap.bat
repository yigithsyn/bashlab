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

      SET fullpath=%%a%filename%
      GOTO break
   )
)
:break

ECHO %fullpath%
"%fullpath%" -version
reg Query "HKLM\Hardware\Description\System\CentralProcessor\0" | find /i "x86" > NUL && set OS=32BIT || set OS=64BIT

if %OS%==32BIT echo This is a 32bit operating system
if %OS%==64BIT echo This is a 64bit operating system

if %OS%==32BIT "%fullpath%" -DCMAKE_INSTALL_PREFIX=%USERPROFILE%\AppData\Local .. -T host=x86 -A Win32
if %OS%==64BIT "%fullpath%" -DCMAKE_INSTALL_PREFIX=%USERPROFILE%\AppData\Local .. -T host=x64 -A x64

"%fullpath%" --build . --config Debug

ECHO "[INFO] Installing ..."
"%fullpath%" --build . --target INSTALL --config MinSizeRel

CD ..