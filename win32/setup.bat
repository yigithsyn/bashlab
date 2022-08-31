@ECHO OFF

IF [%1]==[] (
  SET JQ_BINARY=jq-win32.exe
  SET PYTHON_INSTALLER=python-3.8.10.exe
  SET PYTHON_INSTALL_DIR=%LocalAppData%\Programs\Python3.8-32
  SET PYTHON_UNINSTALL_NAME="Python 3.8.10 (32-bit)"
)
IF "%1"=="x64" (
  SET JQ_BINARY=jq-win64.exe
  SET PYTHON_INSTALLER=python-3.8.10-amd64.exe
  SET PYTHON_INSTALL_DIR=%LocalAppData%\Programs\Python3.8-64
  SET PYTHON_UNINSTALL_NAME="Python 3.8.10 (64-bit)"
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
winget uninstall %PYTHON_UNINSTALL_NAME%
curl -L https://www.python.org/ftp/python/3.8.10/%PYTHON_INSTALLER% --output requirements\%PYTHON_INSTALLER%
requirements\%PYTHON_INSTALLER% /quiet InstallAllUsers=0 PrependPath=0 DefaultJustForMeTargetDir=%PYTHON_INSTALL_DIR%

ECHO -------------------------------------------------------------------------
ECHO [INFO] Requirements: Jq: Lightweight and flexible command-line JSON processor
ECHO -------------------------------------------------------------------------
DEL /F /Q %LocalAppData%\bin\jq.exe
curl -L https://github.com/stedolan/jq/releases/download/jq-1.6/%JQ_BINARY% --output %LocalAppData%\bin\jq.exe

RMDIR /Q /S requirements > nul 2>&1