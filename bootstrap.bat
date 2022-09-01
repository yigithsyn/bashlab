@ECHO OFF

MKDIR test > nul 2>&1




:build
ECHO ============================
ECHO [INFO] Building ...
ECHO ============================
TYPE list.json | jq -c ".[] | select(.osOnly == false)" > list2.json
mongo bashlab --eval "db.programs.drop()"
mongoimport --db bashlab --collection programs --file list2.json
mongo bashlab --eval "db.programs.find().length()"
DEL list2.json

RMDIR /Q /S dist > nul 2>&1 
MKDIR dist
RMDIR /Q /S build > nul 2>&1
MKDIR build

@REM C/C++
CD build
cmake.exe -DCMAKE_INSTALL_PREFIX=%USERPROFILE%\AppData\Local ..
CD ..

@REM Python
%USERPROFILE%\AppData\Local\Programs\Python\Python38\Scripts\pyinstaller --onefile main\ams2nsi.py
DEL ams2nsi.spec
RMDIR /Q /S main\__pycache__ > nul 2>&1 

:install
ECHO ============================
ECHO Installing ...
ECHO ============================
@REM C/C++
CD build
cmake.exe --build . --target INSTALL --config Release
RMDIR /Q /S build > nul 2>&1
CD ..

@REM Python
COPY /Y dist\*.exe %USERPROFILE%\AppData\Local\bin\

RMDIR /Q /S dist > nul 2>&1 
RMDIR /Q /S build > nul 2>&1 