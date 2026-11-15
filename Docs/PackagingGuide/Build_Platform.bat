@echo off
REM ============================================================
REM  CleanCutAction - per-platform package script
REM  Usage: Build_Platform.bat Win64 ^| Web ^| Android ^| Linux
REM  Run it from the checkout of the matching branch (windows, wasm, android, linux).
REM  The project is found relative to this file (Docs\PackagingGuide\..\..), so it works in every checkout folder.
REM ============================================================
setlocal

REM UE 5.9 Preview (installed build). Override with the UE_ROOT environment variable if it moves.
if not defined UE_ROOT set "UE_ROOT=F:\UE5 Engines\UE_5.9"
for %%I in ("%~dp0..\..") do set "PROJECT_DIR=%%~fI"
set "PROJECT=%PROJECT_DIR%\CleanCutAction.uproject"
REM UAT archives into <OUT>\<Platform folder>: Packaged\Wasm, Packaged\Windows, Packaged\Linux, Packaged\Android_ASTC
set "OUT=%PROJECT_DIR%\Packaged"
set "UAT=%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat"
set COMMON=-project="%PROJECT%" -clientconfig=Shipping -build -cook -stage -pak -iostore -archive -archivedirectory="%OUT%" -noP4 -unattended -utf8output

if not exist "%PROJECT%" (
    echo Project not found: %PROJECT%
    exit /b 1
)
if not exist "%UAT%" (
    echo RunUAT not found: %UAT%  ^(set UE_ROOT^)
    exit /b 1
)

if "%~1"=="" goto usage

if /I "%~1"=="Win64" (
    call "%UAT%" BuildCookRun %COMMON% -platform=Win64 -prereqs
    goto done
)
if /I "%~1"=="Web" (
    call "%UAT%" BuildCookRun %COMMON% -platform=Wasm -package
    goto done
)
if /I "%~1"=="Android" (
    call "%UAT%" BuildCookRun %COMMON% -platform=Android -cookflavor=ASTC -package
    goto done
)
if /I "%~1"=="Linux" (
    call "%UAT%" BuildCookRun %COMMON% -platform=Linux
    goto done
)

:usage
echo Usage: Build_Platform.bat Win64 ^| Web ^| Android ^| Linux
exit /b 1

:done
if errorlevel 1 (
    echo BUILD FAILED for %~1. Check the UAT log: %APPDATA%\Unreal Engine\AutomationTool\Logs\F+UE5+Engines+UE_5.9\Log.txt
    exit /b 1
)
echo BUILD SUCCEEDED for %~1. Output under %OUT%. Record the size in Docs\PackagingGuide\README.md ^(Build records^).
exit /b 0
