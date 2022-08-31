@ECHO OFF

IF [%1]==[] (
  SET JQ_BINARY=jq-win32.exe
  SET PYTHON_INSTALLER=python-3.8.10.exe
  SET PYTHON_INSTALL_DIR=%LocalAppData%\Programs\Python32
)
IF "%1"=="x64" (
  SET JQ_BINARY=jq-win64.exe
  SET PYTHON_INSTALLER=python-3.8.10-win64.exe
  SET PYTHON_INSTALL_DIR=%LocalAppData%\Programs\Python64
)

  
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
curl -L https://www.python.org/ftp/python/3.8.10/%PYTHON_INSTALLER% --output requirements\%PYTHON_INSTALLER%
requirements\%PYTHON_INSTALLER% /quiet InstallAllUsers=0 PrependPath=1 DefaultJustForMeTargetDir=%PYTHON_INSTALL_DIR%
RMDIR /Q /S requirements > nul 2>&1

ECHO -------------------------------------------------------------------------
ECHO [INFO] Requirements: Jq: Lightweight and flexible command-line JSON processor
ECHO -------------------------------------------------------------------------
DEL /F /Q %USERPROFILE%\AppData\Local\bin\jq.exe
curl -L https://github.com/stedolan/jq/releases/download/jq-1.6/JQ_BINARY --output %USERPROFILE%\AppData\Local\bin\jq.exe