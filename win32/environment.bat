@ECHO OFF

IF [%1]==[] (
  SET JQ_DOWNLOAD_BINARY=jq-win32.exe
  SET PYTHON_INSTALLER=python-3.8.10.exe
  SET PYTHON_INSTALL_DIR=%LocalAppData%\Programs\Python3.8-32
  SET PYTHON_UNINSTALL_NAME="Python 3.8.10 (32-bit)"
  SET VCVARS="vcvars32.bat"
)
IF "%1"=="x64" (
  SET JQ_DOWNLOAD_BINARY=jq-win64.exe
  SET PYTHON_INSTALLER=python-3.8.10-amd64.exe
  SET PYTHON_INSTALL_DIR=%LocalAppData%\Programs\Python3.8-64
  SET PYTHON_UNINSTALL_NAME="Python 3.8.10 (64-bit)"
  SET VCVARS="vcvars64.bat"
)

SET JQ_INSTALL_DIR=%LocalAppData%\Programs\jq
SET JQ_PATH=%JQ_INSTALL_DIR%\jq.exe
SET PYTHON_PATH=%PYTHON_INSTALL_DIR%\python.exe
SET VS_TOOLSET_PATH="C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build"

SET PATH=%JQ_INSTALL_DIR%;%PYTHON_INSTALL_DIR%;%PYTHON_INSTALL_DIR%\Scripts;%PATH%

%VS_TOOLSET_PATH%\%VCVARS%