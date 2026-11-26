@echo off
rem ScreenSaver v0.3.2 - Build-Skript (Doppelklick).
rem Baut build\ScreenSaver.scr, legt es in fertig\ ab und baut danach den Installer (Inno Setup), falls installiert.
rem Jede Version bleibt in Versionen\ archiviert. Alle Meldungen stehen auch in build\build-log.txt.
rem Hinweis: keine Klammerbloecke verwenden, sonst bricht die Batch-Datei bei Klammern in Meldungen ab.
setlocal
cd /d "%~dp0"
if not exist build mkdir build
set "LOG=%~dp0build\build-log.txt"
call :main > "%LOG%" 2>&1
set "RC=%errorlevel%"
type "%LOG%"
echo.
if not "%RC%"=="0" echo *** FEHLER: Bitte die Meldungen oben an Claude schicken, sie stehen auch in build\build-log.txt
if "%RC%"=="0" echo *** FERTIG
pause
exit /b %RC%

:main
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto :novs
set "VSDIR="
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSDIR=%%i"
if not defined VSDIR goto :novs
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul

set "VER=0.0.0"
set "BLD=0"
for /f "tokens=3" %%a in ('findstr /C:"#define BSL_VERSION " Source\Version.h') do set "VER=%%~a"
for /f "tokens=3" %%a in ('findstr /C:"#define BSL_BUILD " Source\Version.h') do set "BLD=%%~a"
echo Version %VER% Build %BLD%

rem Logo ist optional: liegt Resources\Logo.png vor, wird es eingebettet
set "LOGODEF="
if exist "Resources\Logo.png" set "LOGODEF=/DHAS_LOGO"
echo Logo: %LOGODEF%

echo --- Ressourcen
rc /nologo %LOGODEF% /fo build\app.res Source\app.rc
if errorlevel 1 goto :rcfail

echo --- Kompilieren
cl /nologo /std:c++17 /O2 /EHsc /W3 /utf-8 /DUNICODE /D_UNICODE %LOGODEF% Source\main.cpp build\app.res /Fo:build\ /Fe:build\ScreenSaver.scr /link /SUBSYSTEM:WINDOWS
if errorlevel 1 goto :clfail

if not exist fertig mkdir fertig
if not exist Versionen mkdir Versionen
del /q "fertig\*.scr" >nul 2>&1
del /q "fertig\*Setup*.exe" >nul 2>&1
copy /y "build\ScreenSaver.scr" "fertig\ScreenSaver.scr" >nul
copy /y "build\ScreenSaver.scr" "Versionen\ScreenSaver_v%VER%.scr" >nul
echo Fertig: fertig\ScreenSaver.scr

echo --- Installer
set "ISCC="
if exist "%ProgramFiles(x86)%\Inno Setup 7\ISCC.exe" set "ISCC=%ProgramFiles(x86)%\Inno Setup 7\ISCC.exe"
if exist "%ProgramFiles%\Inno Setup 7\ISCC.exe" set "ISCC=%ProgramFiles%\Inno Setup 7\ISCC.exe"
if exist "%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe" set "ISCC=%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
if exist "%ProgramFiles%\Inno Setup 6\ISCC.exe" set "ISCC=%ProgramFiles%\Inno Setup 6\ISCC.exe"
if not defined ISCC goto :noiscc
"%ISCC%" /Qp /DAppVersion=%VER% /DAppBuild=%BLD% installer\ScreenSaver.iss
if errorlevel 1 goto :isfail
copy /y "installer\Output\ScreenSaver-Setup-v%VER%-Build%BLD%.exe" "fertig\" >nul
copy /y "installer\Output\ScreenSaver-Setup-v%VER%-Build%BLD%.exe" "Versionen\" >nul
echo Installer: fertig\ScreenSaver-Setup-v%VER%-Build%BLD%.exe
exit /b 0

:noiscc
echo Inno Setup wurde nicht gefunden, der Installer wurde nicht gebaut.
exit /b 0

:novs
echo Visual Studio mit den C++-Werkzeugen wurde nicht gefunden.
exit /b 1

:rcfail
echo FEHLER bei den Ressourcen.
exit /b 1

:clfail
echo FEHLER beim Kompilieren.
exit /b 1

:isfail
echo FEHLER beim Installer.
exit /b 1
