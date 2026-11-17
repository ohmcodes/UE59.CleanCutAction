@echo off
rem Clean! Cut! Action! VR - runs the game outside the editor in VR (OpenXR: Meta XR Simulator or Link / Air Link).
rem Much faster than VR Preview in the editor, so the XR Simulator doesn't flag the app as unresponsive and drop its input.
rem
rem Usage:  Scripts\RunVR_Standalone.bat          launch only
rem         Scripts\RunVR_Standalone.bat build    build the editor module first (close the Unreal Editor first;
rem                                               a standalone run doesn't include Live Coding patches)

set ENGINE=D:\UE5 Engines\UE_5.9
set PROJECT=%~dp0..\CleanCutAction.uproject

if /I "%~1"=="build" (
  call "%ENGINE%\Engine\Build\BatchFiles\Build.bat" CleanCutActionEditor Win64 Development "-Project=%PROJECT%" -WaitMutex
  if errorlevel 1 (
    echo Build failed. Close the Unreal Editor and try again.
    exit /b 1
  )
)

rem -ModelContextProtocolPort keeps port 8000 free for the editor's unreal-mcp server.
start "" "%ENGINE%\Engine\Binaries\Win64\UnrealEditor.exe" "%PROJECT%" -game -vr -windowed -ResX=1280 -ResY=720 -log=VRStandalone.log -ModelContextProtocolPort=8011
