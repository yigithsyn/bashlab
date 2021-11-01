@ECHO OFF
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
"%fullpath%" -DCMAKE_INSTALL_PREFIX=%USERPROFILE%\AppData\Local ..
"%fullpath%" --build . --target INSTALL --config Release 
