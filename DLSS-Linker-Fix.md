# Debug x64 Linker Fix (DLSS/NGX)

## Issue
The project failed to link in **Debug x64** with:

- `LNK1181: cannot open input file ...\Plugins\DLSS\Source\ThirdParty\NGX\Lib\x64\nvsdk_ngx_d.lib`

The NGX import libraries were not present in the workspace plugin copy.

## Changes Applied

### 1) `CleanCutAction.uproject`
Disabled NVIDIA plugin entries that require missing NGX libs:

- `DLSS`
- `StreamlineCore`
- `StreamlineNGXCommon`
- `StreamlineDLSSG`
- `StreamlineReflex`

### 2) `Source/CleanCutAction/CleanCutAction.Build.cs`
Added a runtime check for NGX import library folder presence before adding DLSS/Streamline module dependencies on Win64.

- If `Plugins/DLSS/Source/ThirdParty/NGX/Lib` exists, DLSS deps are added.
- If it does not exist, `WITH_LTT_DLSS=0` is set and those deps are skipped.

## Result
`Intermediate/ProjectFiles/CleanCutAction.vcxproj` builds successfully after the change.

## Notes
Full solution build may still include unrelated Unreal Engine program-rule errors from the engine installation, but the game project linker error is resolved.
