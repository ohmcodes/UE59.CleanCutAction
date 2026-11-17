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

REM Android SDK / NDK / JDK for this run only, when SetupAndroid.bat hasn't set them for the user.
if not defined ANDROID_HOME set "ANDROID_HOME=%LOCALAPPDATA%\Android\Sdk"
if not defined NDKROOT set "NDKROOT=%ANDROID_HOME%\ndk\27.2.12479018"
if not defined NDK_ROOT set "NDK_ROOT=%NDKROOT%"
REM JDK 21: Gradle 8.13 (UE 5.9) can't run on Android Studio's bundled Java 25 ("Unsupported class file major version 69").
if not defined JAVA_HOME set "JAVA_HOME=C:\Program Files\Android\openjdk\jdk-21.0.8"

REM -nocompileeditor / -skipbuildeditor: works with the editor open (Live Coding blocks editor builds). The cook uses
REM the editor build already on disk; the game code itself is compiled fresh for Android.
call "%UAT%" BuildCookRun -project="%PROJECT%" -platform=Android -cookflavor=ASTC -clientconfig=%CONFIG% ^
    -build -cook -stage -pak -package -archive -archivedirectory="%OUT%" -noP4 -utf8output -unattended ^
    -nocompileeditor -skipbuildeditor
REM Not "if errorlevel 1": crash exit codes are negative (e.g. -1073741818) and that test misses them.
if not "%ERRORLEVEL%"=="0" (
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
