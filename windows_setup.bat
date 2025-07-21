@echo off
setlocal

REM set directory to location of this file. Needed when drag n dropping across directories.
cd /d %~dp0

if not exist "C:/devkitPro/devkitPPC" (
    echo ERROR: devkitPro not found at "C:/devkitPro/devkitPPC"
    echo Please install devkitPro with the GameCube package
    goto end
) else (
    echo found devkitPro
    set "PATH=%PATH%;C:\devkitPro\devkitPPC\bin"
)

C:\devkitPro\msys2\msys2_shell.cmd -use-full-path -defterm -no-start -msys2 -here

:end

REM pause if not run from command line
echo %CMDCMDLINE% | findstr /C:"/c">nul && pause
