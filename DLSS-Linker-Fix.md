# DLSS/NGX Linker Fix

## Issue
Every configuration (Development and Debug, editor and game) failed to link the DLSS plugin modules with:

- `LNK1181: cannot open input file ...\Plugins\DLSS\Source\ThirdParty\NGX\Lib\x64\nvsdk_ngx_d.lib`

`.gitignore` ignores `*.lib`, so the NGX import libraries in `Plugins/DLSS/Source/ThirdParty/NGX/Lib/` were never committed. A fresh clone had no `Lib` folder. The DLSS plugins are enabled in `CleanCutAction.uproject`, so UBT builds them and the link fails.

## Fix

1. Copied `Plugins/DLSS/Source/ThirdParty/NGX/Lib/` (x64 and arm64 `nvsdk_ngx_*.lib`) from the NVIDIA package `UE5.8_DLSS4.5Plugin_v8.9.0`. Only the missing files were copied; the 5.9 patches in the plugin (`bValidateFormatStrings = false`, `EngineVersion 5.9.0`) are untouched.
2. Added a `.gitignore` exception so the NGX import libraries are committed:
   `!Plugins/DLSS/Source/ThirdParty/NGX/Lib/**/*.lib`

The DLSS plugins stay enabled for Win64 in the `.uproject`.

## `CleanCutAction.Build.cs` guard
`CleanCutAction.Build.cs` adds the DLSS/Streamline dependencies and sets `WITH_LTT_DLSS=1` only if `Plugins/DLSS/Source/ThirdParty/NGX/Lib` exists. With the libraries committed, the folder is present and DLSS is compiled in. The guard only affects the game module; it does not stop the plugins themselves from building.

## Runtime DLLs (still not committed)
`Plugins/**/Binaries/*` is ignored, so the runtime DLLs must be copied from the NVIDIA package on a fresh clone. The game builds without them, but DLSS, Frame Generation and Reflex will not run:

- `Plugins/DLSS/Binaries/ThirdParty/Win64/` (`nvngx_dlss.dll`, `nvngx_dlssd.dll`, plus `Development/`)
- `Plugins/StreamlineCore/Binaries/ThirdParty/Win64/` (`sl.*.dll`, `nvngx_dlssg.dll`, `nvngx_deepdvc.dll`, plus `development/`)

Copy only missing files (for example `robocopy <src> <dst> /E /XC /XN /XO`) so the 5.9-patched plugin files are not replaced.

## Result
`CleanCutActionEditor Win64 Development` builds successfully (2026-11-15).
