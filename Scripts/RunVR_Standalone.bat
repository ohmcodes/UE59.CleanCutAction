@echo off
rem Clean! Cut! Action! VR - runs the game outside the editor in VR (OpenXR: Meta XR Simulator or Link / Air Link).
rem Much faster than VR Preview in the editor, so the XR Simulator doesn't flag the app as unresponsive and drop its input.
rem
rem Usage:  Scripts\RunVR_Standalone.bat               launch on the active OpenXR runtime
rem         Scripts\RunVR_Standalone.bat link          launch on the Quest headset through Meta Horizon Link / Air Link
rem                                                    (for this run only; the system's active runtime is left alone)
rem         Scripts\RunVR_Standalone.bat sim           launch in the Meta XR Simulator (for this run only)
rem         Scripts\RunVR_Standalone.bat build [link]  build the editor module first (close the Unreal Editor first;
rem                                                    a standalone run doesn't include Live Coding patches)

set ENGINE=D:\UE5 Engines\UE_5.9
set PROJECT=%~dp0..\CleanCutAction.uproject
set LINK_RUNTIME=C:\Program Files\Meta Horizon\Support\oculus-runtime\oculus_openxr_64.json
set SIM_RUNTIME=C:\Program Files\MetaXRSimulator\v207.1\meta_openxr_simulator.json
set DO_BUILD=

:args
if "%~1"=="" goto run
if /I "%~1"=="build" set DO_BUILD=1
if /I "%~1"=="link" set "XR_RUNTIME_JSON=%LINK_RUNTIME%"
if /I "%~1"=="sim" set "XR_RUNTIME_JSON=%SIM_RUNTIME%"
shift
goto args

:run
if defined DO_BUILD (
  call "%ENGINE%\Engine\Build\BatchFiles\Build.bat" CleanCutActionEditor Win64 Development "-Project=%PROJECT%" -WaitMutex
  if errorlevel 1 (
    echo Build failed. Close the Unreal Editor and try again.
    exit /b 1
  )
)

if defined XR_RUNTIME_JSON echo OpenXR runtime for this run: %XR_RUNTIME_JSON%

rem -ModelContextProtocolPort keeps port 8000 free for the editor's unreal-mcp server.
start "" "%ENGINE%\Engine\Binaries\Win64\UnrealEditor.exe" "%PROJECT%" -game -vr -windowed -ResX=1280 -ResY=720 -log=VRStandalone.log -ModelContextProtocolPort=8011
