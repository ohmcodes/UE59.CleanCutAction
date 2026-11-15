@echo off
REM ============================================================
REM  CleanCutAction - per-platform package script
REM  Usage: Build_Platform.bat Win64 ^| Web ^| Android ^| Linux
REM  Fill in the TODO values before first use.
REM ============================================================

REM TODO: path to your UE 5.9 source build root (folder containing Engine\)
set "UE_ROOT=F:\TODO\UnrealEngine"
REM TODO: verify the .uproject file name
set "PROJECT=F:\UE5 Projects\CleanCutAction\CleanCutAction.uproject"
REM TODO: UAT platform name for Web (see 02_Web_WASM.md section 2)
set "WEB_PLATFORM=TODO_WebPlatformName"

set "OUT=F:\UE5 Projects\CleanCutAction\Builds"
set "UAT=%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat"
set "COMMON=-project="%PROJECT%" -clientconfig=Shipping -build -cook -stage -pak -archive -noP4 -utf8output"

if "%~1"=="" goto usage

if /I "%~1"=="Win64" (
    call "%UAT%" BuildCookRun %COMMON% -platform=Win64 -prereqs -archivedirectory="%OUT%\Windows"
    goto done
)
if /I "%~1"=="Web" (
    if "%WEB_PLATFORM%"=="TODO_WebPlatformName" (
        echo Set WEB_PLATFORM in this script first. See 02_Web_WASM.md section 2.
        exit /b 1
    )
    call "%UAT%" BuildCookRun %COMMON% -platform=%WEB_PLATFORM% -archivedirectory="%OUT%\Web"
    goto done
)
if /I "%~1"=="Android" (
    call "%UAT%" BuildCookRun %COMMON% -platform=Android -cookflavor=ASTC -package -archivedirectory="%OUT%\Android"
    goto done
)
if /I "%~1"=="Linux" (
    call "%UAT%" BuildCookRun %COMMON% -platform=Linux -archivedirectory="%OUT%\SteamDeck"
    goto done
)

:usage
echo Usage: Build_Platform.bat Win64 ^| Web ^| Android ^| Linux
exit /b 1

:done
if errorlevel 1 (
    echo BUILD FAILED for %~1. Check the UAT log above.
    exit /b 1
)
echo BUILD SUCCEEDED for %~1. Record the result in Docs\Packaging\BUILD_LOG.md
