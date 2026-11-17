@echo off
REM ============================================================
REM  Clean! Cut! Action! VR - package the Meta Quest APK
REM  Copyright (c) 2026 Ohmcodes. All Rights Reserved.
REM
REM  Usage:
REM    Scripts\Package_Quest.bat              Shipping APK (what gets submitted)
REM    Scripts\Package_Quest.bat dev          Development APK (logcat logs, stat commands)
REM    Scripts\Package_Quest.bat install      Shipping APK, then install it on the connected headset
REM
REM  Needs: Android Studio + Engine\Extras\Android\SetupAndroid.bat run once (SDK / NDK / licenses).
REM  The editor can stay open. Output: Builds\Quest\Android_ASTC\  (APK + Install_*.bat)
REM  Settings that shape the APK: Config\DefaultEngine.ini [AndroidRuntimeSettings], Config\Android\
REM  AndroidDeviceProfiles.ini [Android_OpenXR]. See Docs\10_VR_Meta_Quest.md section 9.
REM ============================================================
setlocal

set "UE_ROOT=D:\UE5 Engines\UE_5.9"
set "PROJECT=%~dp0..\CleanCutAction.uproject"
set "OUT=%~dp0..\Builds\Quest"
set "UAT=%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat"

set "CONFIG=Shipping"
if /I "%~1"=="dev" set "CONFIG=Development"

call "%UAT%" BuildCookRun -project="%PROJECT%" -platform=Android -cookflavor=ASTC -clientconfig=%CONFIG% ^
    -build -cook -stage -pak -package -archive -archivedirectory="%OUT%" -noP4 -utf8output -unattended
if errorlevel 1 (
    echo.
    echo QUEST PACKAGE FAILED. Check the UAT log above ^(and Saved\Logs^).
    exit /b 1
)

echo.
echo QUEST PACKAGE SUCCEEDED: %OUT%\Android_ASTC
dir /b "%OUT%\Android_ASTC\*.apk"

if /I "%~1"=="install" (
    for %%F in ("%OUT%\Android_ASTC\Install_*.bat") do call "%%F"
)
endlocal
