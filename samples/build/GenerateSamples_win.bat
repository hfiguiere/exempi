:: =================================================================================================
:: Copyright 2013 Adobe Systems Incorporated
:: All Rights Reserved.
::
:: NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the terms
:: of the Adobe license agreement accompanying it.
:: =================================================================================================

echo OFF
cls
set CMAKE=..\..\tools\cmake\bin\cmake.exe
set workingDir=%~dp0%
set CMAKE=%workingDir%%CMAKE%

if NOT exist %CMAKE% ( ECHO Cmake tool not present at %CMAKE%, cannot proceed 
pause
exit /B 0)


ECHO Enter your choice:
ECHO 1. Clean
ECHO 2. VC2010, 32 Bit Project
ECHO 3. VC2010, 64 Bit Project
ECHO 4. Build All


ECHO
set /P choice=Enter your choice:

IF "%choice%"=="1" GOTO CleanCMake
IF "%choice%"=="2" GOTO 32VC2010
IF "%choice%"=="3" GOTO 64VC2010
IF "%choice%"=="4" GOTO 32VC2010


ECHO Invalid Choice, Exiting
pause
exit /B 0

:CleanCmake
echo "Cleaning..."
if exist cmake\CustomSchema\build rmdir /S /Q cmake\CustomSchema\build
if exist cmake\CustomSchema\build_x64 rmdir /S /Q cmake\CustomSchema\build_x64
if exist cmake\DumpFile\build rmdir /S /Q cmake\DumpFile\build
if exist cmake\DumpFile\build_x64 rmdir /S /Q cmake\DumpFile\build_x64
if exist cmake\DumpMainXMP\build rmdir /S /Q cmake\DumpMainXMP\build
if exist cmake\DumpMainXMP\build_x64 rmdir /S /Q cmake\DumpMainXMP\build_x64
if exist cmake\DumpScannedXMP\build rmdir /S /Q cmake\DumpScannedXMP\build
if exist cmake\DumpScannedXMP\build_x64 rmdir /S /Q cmake\DumpScannedXMP\build_x64
if exist cmake\ModifyingXMP\build rmdir /S /Q cmake\ModifyingXMP\build
if exist cmake\ModifyingXMP\build rmdir /S /Q cmake\ModifyingXMP\build
if exist cmake\ReadingXMP\build_x64 rmdir /S /Q cmake\ReadingXMP\build_x64
if exist cmake\ReadingXMP\build rmdir /S /Q cmake\ReadingXMP\build
if exist cmake\XMPCommand\build_x64 rmdir /S /Q cmake\XMPCommand\build_x64
if exist cmake\XMPCommand\build rmdir /S /Q cmake\XMPCommand\build
if exist cmake\XMPCoreCoverage\build_x64 rmdir /S /Q cmake\XMPCoreCoverage\build_x64
if exist cmake\XMPCoreCoverage\build rmdir /S /Q cmake\XMPCoreCoverage\build
if exist cmake\XMPFilesCoverage\build_x64 rmdir /S /Q cmake\XMPFilesCoverage\build_x64
if exist cmake\XMPFilesCoverage\build rmdir /S /Q cmake\XMPFilesCoverage\build
if exist cmake\XMPIterations\build_x64 rmdir /S /Q cmake\XMPIterations\build_x64
if exist cmake\XMPIterations\build rmdir /S /Q cmake\XMPIterations\build

if exist vc10 rmdir /S /Q vc10
if exist ..\target\windows rmdir /S /Q ..\target\windows
if exist ..\target\windows_x64 rmdir /S /Q ..\target\windows_x64

echo "Done"
pause
exit /B 0


:32VC2010
set GENERATOR=Visual Studio 10
set DIR=vc10\windows
set bit64=0
GOTO GenerateNow

:64VC2010
set GENERATOR=Visual Studio 10 Win64
set DIR=vc10\windows_x64
set bit64=1
GOTO GenerateNow



:GenerateNow

IF NOT exist %DIR% mkdir %DIR%
cd %DIR%
if errorlevel 1 ( ECHO Cannot create folder %DIR% for generating project
goto error)
ECHO "%CMAKE%"  ..\..\cmake\ -G"%GENERATOR%" -DCMAKE_CL_64=%bit64%
"%CMAKE%"  ..\..\cmake\ -G"%GENERATOR%" -DCMAKE_CL_64=%bit64% 
if errorlevel 1 ( cd ..\..\
goto error)
IF "%choice%"=="4" ( set choice="0"
cd ..\..\
goto 64VC2010
)
goto ok


:error
echo CMake Build Failed.
pause
exit /B 1


:ok
cd ..\..\
echo CMake Build Success.
pause
exit /B 0
