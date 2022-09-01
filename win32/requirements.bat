@ECHO OFF

IF [%1]==[] (
  SET JQ_DOWNLOAD_BINARY=jq-win32.exe
  SET PYTHON_INSTALLER=python-3.8.10.exe
  SET PYTHON_INSTALL_DIR=%LocalAppData%\Programs\Python3.8-32
  SET PYTHON_UNINSTALL_NAME="Python 3.8.10 (32-bit)"
)
IF "%1"=="x64" (
  SET JQ_DOWNLOAD_BINARY=jq-win64.exe
  SET PYTHON_INSTALLER=python-3.8.10-amd64.exe
  SET PYTHON_INSTALL_DIR=%LocalAppData%\Programs\Python3.8-64
  SET PYTHON_UNINSTALL_NAME="Python 3.8.10 (64-bit)"
)

SET JQ_INSTALL_DIR=%LocalAppData%\Programs\jq
SET JQ_PATH=%JQ_INSTALL_DIR%\jq.exe
SET PYTHON_PATH=%PYTHON_INSTALL_DIR%\python.exe
  
ECHO =========================================================================
ECHO [INFO] Requirements ...
ECHO =========================================================================
RMDIR /Q /S requirements > nul 2>&1 
MKDIR requirements

ECHO -------------------------------------------------------------------------
ECHO [INFO] Requirements: Python 3.8
ECHO -------------------------------------------------------------------------
winget uninstall %PYTHON_UNINSTALL_NAME%
curl -L https://www.python.org/ftp/python/3.8.10/%PYTHON_INSTALLER% --output requirements\%PYTHON_INSTALLER%
requirements\%PYTHON_INSTALLER% /quiet InstallAllUsers=0 PrependPath=0 DefaultJustForMeTargetDir=%PYTHON_INSTALL_DIR%

ECHO -------------------------------------------------------------------------
ECHO [INFO] Requirements: Jq: Lightweight and flexible command-line JSON processor
ECHO -------------------------------------------------------------------------
RMDIR /Q /S %JQ_INSTALL_DIR% > nul 2>&1
MKDIR %JQ_INSTALL_DIR%
curl -L https://github.com/stedolan/jq/releases/download/jq-1.6/%JQ_DOWNLOAD_BINARY% --output %JQ_INSTALL_DIR%\jq.exe

RMDIR /Q /S requirements > nul 2>&1