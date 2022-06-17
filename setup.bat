@ECHO OFF

RMDIR /Q /S setup > nul 2>&1 
RMDIR /Q /S msvc > nul 2>&1 
MKDIR setup
MKDIR msvc

@REM winget uninstall "Microsoft Visual Studio Installer"

curl -L https://aka.ms/vs/16/release/vs_buildtools.exe --output setup\vs_buildtools.exe
setup\vs_buildtools.exe --installPath %CD%\msvc^
 --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64^
 --add Microsoft.VisualStudio.Component.VC.CMake.Project^
 --add Microsoft.VisualStudio.Component.VC.ASAN^
 --passive

@REM  --add Microsoft.VisualStudio.Workload.MSBuildTools^
@REM  --add Microsoft.VisualStudio.Workload.VCTools^
RMDIR /Q /S setup > nul 2>&1 
