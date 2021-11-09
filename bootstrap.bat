@ECHO OFF

ECHO ============================
ECHO [INFO] Dependencies ...
ECHO ============================
RMDIR /Q /S libs 
MKDIR libs
ECHO "Downloading argtable3 ..."
curl -L https://github.com/argtable/argtable3/releases/download/v3.2.1.52f24e5/argtable-v3.2.1.52f24e5-amalgamation.tar.gz --output libs\argtable3.tar.gz --silent 
MKDIR libs\argtable3
tar -xf libs\argtable3.tar.gz --directory libs\argtable3

ECHO ============================
ECHO [INFO] Building ...
ECHO ============================
RMDIR /Q /S build > nul 2>&1
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

"%cmake_path%" -DCMAKE_INSTALL_PREFIX=%USERPROFILE%\AppData\Local ..

ECHO ============================
ECHO Installing ...
ECHO ============================
"%cmake_path%" --build . --target INSTALL --config Release

CD ..