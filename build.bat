@echo off
setlocal EnableDelayedExpansion
cd /d "%~dp0"

if exist "C:\mingw64\bin\gcc.exe" set "PATH=C:\mingw64\bin;%PATH%"
for /d %%D in ("%LOCALAPPDATA%\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT*") do (
  if exist "%%D\mingw64\bin\gcc.exe" set "PATH=%%D\mingw64\bin;%PATH%"
)

rem Every .c under src\ is part of the build; new files need no change here.
rem File names must be unique across folders (MSVC puts all .obj in one folder).
set "SRC="
for /r "src" %%F in (*.c) do set "SRC=!SRC! "%%F""
if not defined SRC (
  echo No source files under src\.
  exit /b 1
)

where gcc >nul 2>&1
if %errorlevel%==0 goto gcc
where cl >nul 2>&1
if %errorlevel%==0 goto msvc

echo No gcc or cl on PATH.
echo Install WinLibs, then open a new terminal:
echo   winget install -e --id BrechtSanders.WinLibs.POSIX.UCRT --proxy http://127.0.0.1:10808
echo If --proxy is rejected, enable it once from an admin terminal:
echo   winget settings --enable ProxyCommandLineArgument
echo Check: gcc --version
echo Check: windres --version
exit /b 1

:gcc
set "RES="
where windres >nul 2>&1
if errorlevel 1 goto gcc_nometa
windres -O coff -i dk4save.rc -o dk4save_res.o
if errorlevel 1 exit /b 1
set "RES=dk4save_res.o"
goto gcc_link
:gcc_nometa
echo windres not found. Building without the visual-styles manifest.
:gcc_link
set "SPECS="
if not "%RES%"=="" set "SPECS=-specs=mingw-endfile.specs"
gcc -O2 -std=c11 -Wall -Wextra -mwindows -static-libgcc -finput-charset=UTF-8 -fexec-charset=UTF-8 %SPECS% -Isrc -o dk4save.exe !SRC! %RES% -lcomctl32 -lcomdlg32 -lgdi32 -luser32
set "ERR=%errorlevel%"
if exist dk4save_res.o del dk4save_res.o
if not "%ERR%"=="0" exit /b %ERR%
goto done

:msvc
set "RES="
where rc >nul 2>&1
if errorlevel 1 goto msvc_nometa
rc /nologo /fo dk4save.res dk4save.rc
if errorlevel 1 exit /b 1
set "RES=dk4save.res"
goto msvc_link
:msvc_nometa
echo rc not found. Building without the visual-styles manifest.
:msvc_link
if not exist obj mkdir obj
cl /nologo /O2 /W3 /utf-8 /DUNICODE /D_UNICODE /Isrc /Foobj\ /Fe:dk4save.exe !SRC! %RES% /link /SUBSYSTEM:WINDOWS comctl32.lib comdlg32.lib gdi32.lib user32.lib
set "ERR=%errorlevel%"
if exist dk4save.res del dk4save.res
if exist obj rmdir /s /q obj
if not "%ERR%"=="0" exit /b %ERR%

:done
if exist dk4save.exe echo Built %cd%\dk4save.exe
