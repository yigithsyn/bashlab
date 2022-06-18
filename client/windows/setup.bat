@ECHO OFF

@REM https://visualstudio.microsoft.com/tr/downloads/
@REM https://docs.microsoft.com/tr-tr/visualstudio/install/workload-component-id-vs-build-tools?view=vs-2022

RMDIR /Q /S setup > nul 2>&1 
MKDIR setup

curl -L https://aka.ms/vs/17/release/vs_BuildTools.exe --output setup\vs_buildtools.exe
setup\vs_buildtools.exe^
 --add Microsoft.VisualStudio.Workload.MSBuildTools^
 --add Microsoft.VisualStudio.Workload.VCTools^
 --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64^
 --add Microsoft.VisualStudio.Component.VC.CMake.Project^
 --add Microsoft.VisualStudio.Component.VC.ASAN^
 --passive

RMDIR /Q /S setup > nul 2>&1 
