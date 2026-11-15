# CleanCutAction — Packaging Guide (Index)

How to set up and package Clean! Cut! Action! for four targets. Each target has its own branch and its own checkout folder; content and most config are shared.

| Doc | Target | Branch / folder | Renderer | State (2026-11-15) |
|---|---|---|---|---|
| `02_Web_WASM.md` | Browser (desktop + mobile), WebAssembly/WebGPU | `wasm` / `F:\UE5 Projects\CCA_WASM` | Mobile deferred tier (WebGPU) | **Shipped build exists** (2026-11-15 01:14). itch.io target. |
| `01_Windows.md` | Windows "Go Big" (RTX showcase) | `windows` / `F:\UE5 Projects\CCA_Windows` | Desktop, DX12 SM6 | Config in progress (HW ray tracing + device profile added, uncommitted). No DLSS. Not packaged yet. |
| `04_Linux_SteamDeck.md` | Linux / Steam Deck | `linux` / `F:\UE5 Projects\CCA_Linux` | Desktop, Vulkan SM6 | No platform config yet. Not packaged yet. |
| `03_Android.md` | Android APK (sideload) | `android` / `F:\UE5 Projects\CCA_Android` | Mobile, Vulkan + ES3.1 | No platform config yet. Not packaged yet. |

`main` (`F:\UE5 Projects\CleanCutAction`) is the editor/MCP branch: it keeps `AllToolsets` on and is not used for packaging.

Scripts: `Build_Platform.bat` in this folder. Record every build in `BUILD_LOG.md` (not created yet; this guide holds the only size records so far).

---

## Engine context

- Engine: **UE 5.9.0 Preview** at `F:\UE5 Engines\UE_5.9`. It is an **installed build** (`Engine/Build/InstalledBuild.txt`), not a source build, and ships Win64, Linux, LinuxArm64, Android and Wasm binaries.
- Project: C++ (`CleanCutAction.uproject`, one runtime module `CleanCutAction`). The Web target needs a C++ project.
- Project root for this checkout: `F:\UE5 Projects\CCA_Windows`. Other branches use the folders in the table above.
- Wasm platform docs: `F:\UE5 Engines\UE_5.9\Engine\Platforms\Wasm\Docs\` (GettingStarted, Deploy, Performance, Troubleshooting).
- UAT logs: `%APPDATA%\Unreal Engine\AutomationTool\Logs\F+UE5+Engines+UE_5.9\Log.txt`.

---

## How an agent should use these docs

1. Read this README fully, then **one** platform doc.
2. Work in that platform's checkout folder and branch (table above).
3. Go section by section. Tick the platform doc's checklist and report which items are done, skipped or blocked.
4. When something is marked **VERIFY**, look it up in the engine source/config or ask the user. Never guess.

---

## Non-negotiable rules (all platforms)

1. **Never modify, downscale or re-import source assets for a platform.** Texture caps go in device profiles only (see "Texture policy").
2. **`Config/Default*.ini` is for settings valid on ALL platforms.** Platform settings go in `Config/<Platform>/<Platform>*.ini` (Windows, Linux, Android) or `Platforms/Wasm/Config/Wasm*.ini` (Web).
3. **NVIDIA plugins (DLSS, Streamline) are Windows-only.** Never compile or cook them into Linux, Android or Web builds.
4. **Never put API keys or secrets** in the project, config, Blueprints or source.
5. **No level travel may point at an uncooked map.** Only the maps listed below are cooked.
6. **Never set `bSkipEditorContent=True`.** The Landmass plugin's startup materials reference `/Engine/EditorMaterials` and `/Engine/EditorResources`; the cook fails ("Content is missing from cook", UAT ExitCode=25). Tried on wasm 2026-11-14 and reverted.
7. **Stop and ask the user before:** modifying engine source; installing, upgrading or removing plugins; deleting or moving assets; changing a project-wide rendering default in `DefaultEngine.ini`; any change that recompiles every platform's shaders (warn about the time first).
8. Prefer the smallest, reversible change, per platform where possible.

---

## Pre-package check (every branch)

These fixes were applied to all five checkouts on 2026-11-14. Verify them at the start of every packaging session:

| Check | Expected | Where |
|---|---|---|
| `AllToolsets` plugin | `"Enabled": false` (its game feature plugins fail the cook with a GameFeatureData asset-manager error). Only `main` keeps it on. | `CleanCutAction.uproject` |
| `NNEDenoiser` plugin | `"Enabled": false`, and no `+DirectoriesToAlwaysCook=(Path="/NNEDenoiser")` (54 MB of path-tracer models) | `.uproject`, `DefaultGame.ini` |
| "Back to Cleanup" target | `LTTSceneDirector.CleanupLevelName` = `L_NYC_Cafe_Michael`, not the uncooked `L_NYC_Cafe` | `Source/CleanCutAction/Shoot/LTTSceneDirector.h` |
| Compression level | `PackageCompressionLevel_TestShipping=7` | `DefaultGame.ini` |
| `bSkipEditorContent` | `False` | `DefaultGame.ini` |

After editing the `.uproject` or ini files by hand, restart the editor before saving Project Settings, or it writes the old values back.

---

## Texture policy

- Source textures stay at their imported size. The StylizedNewYorkCity textures are 2K and were the bulk of the web build.
- Caps are applied at cook time in the platform's device profile, for example:
  `+TextureLODGroups=(Group=TEXTUREGROUP_World,MinLODSize=1,MaxLODSize=1024,LODBias=0,...)`
- Current caps: Wasm only (`Platforms/Wasm/Config/WasmDeviceProfiles.ini`, see `02_Web_WASM.md`). Windows cooks full resolution. Linux and Android have no caps yet.
- There is no `Config/DefaultDeviceProfiles.ini` and no global texture cap in `DefaultEngine.ini` or `DefaultScalability.ini`.

---

## Shared packaging settings (current `Config/DefaultGame.ini`)

`[/Script/UnrealEd.ProjectPackagingSettings]`, shared by every branch:

| Setting | Value |
|---|---|
| Build configuration | `PPBC_Shipping`, `ForDistribution=False`, `IncludeDebugFiles=False`, `IncludeCrashReporter=False` |
| Containers | `UsePakFile=True`, `bUseIoStore=True`, `bUseZenStore=True` |
| Chunks | `bGenerateChunks=True`, `MaxChunkSize=0` (the web build gets `pakchunk0` + `pakchunk99`) |
| Compression | `bCompressed=True`, Oodle **Kraken**, level 4 (Debug/Development), **7** (Test/Shipping and Distribution) |
| Shader code | `bShareMaterialShaderCode=True`, `bSharedMaterialNativeLibraries=False` |
| Prerequisites | `IncludePrerequisites=True` (Windows installer for the VC++ runtime) |
| Cultures | English only (`+CulturesToStage=en`) |
| Cook scope | `bCookAll=False`, `bCookMapsOnly=False`, `bSkipEditorContent=False` |

**Maps.** `GameDefaultMap` (in `DefaultEngine.ini`) is `/Game/StylizedNewYorkCity/Level/L_NYC_Cafe_Michael` (Phase 1). `MapsToCook` adds:

- `/Game/StylizedNewYorkCity/Level/L_NYC_Cafe_Phase2_Michael` (Phase 2)
- `/Game/ParisStreet/Levels/SampleMap` (the level instance both cafes use)

**Assets loaded only by path** are also in `MapsToCook`, so the cook picks them up: `GC_Generic_Shatter`, `A_Emote_Rookie`, `IA_Emote`, `EmoteMusic/paroparo`, the materials `M_GhostSilhouette`, `M_BrokenProp`, `M_FrameCone`, `M_WaterPuddle`, `Mirror/M_Mirror`, `Mirror/M_MirrorFrame`, the SFX `SW_LightClick`, `SW_MopSwish`, `SW_PuddleSplash`, `SW_PuddleClean`, `SFX/Props/DA_PropSounds`, `VFX/NS_MopSplash`, three UI icons, the widgets `WBP_VirtualKeyboard`, `WBP_MusicToggle`, `WBP_EndingOverlay`, `WBP_ShootHUD`, `WBP_ShootEndScreen`, and the engine basic shapes `Cube`, `Cylinder`, `Plane`, `BasicShapeMaterial`. **Add any new path-loaded asset there.**

Use Full Rebuild only for final submission builds.

---

## Build output layout

`Build_Platform.bat` archives into the project's own `Packaged\` folder (git-ignored), one sub-folder per platform as UAT names it:

```
<checkout>\Packaged\
    Wasm\        (web: upload this folder, zipped)
    Windows\
    Linux\
    Android\     (or Android_ASTC\, depending on cook flavor)
```

---

## Common verification (every platform)

- [ ] Pre-package check above passes.
- [ ] Package completes with no errors. Read the UAT log for "missing from cook" and shader warnings.
- [ ] Build launches into `L_NYC_Cafe_Michael`, Phase 1 → "Shoot the Scene!" loads Phase 2, and "Back to Cleanup" returns to Phase 1.
- [ ] Keyboard/mouse, gamepad and (where relevant) touch all work; on-screen prompts follow the last-used device.
- [ ] Online leaderboard (`https://cca.ohmcodes.com`) submits a run and shows the board.
- [ ] Package size recorded with date, branch and commit.
- [ ] No NVIDIA plugin files in non-Windows packages (search the staged folder for `DLSS`, `Streamline`, `nvngx`).

---

## Build records

| Date | Platform | Branch / commit | Size | Notes |
|---|---|---|---|---|
| 2026-11-15 01:14 | Wasm (Shipping) | `wasm` `d1471cd` + uncommitted 512 world-texture cap | `pakchunk0-Wasm.ucas` 146.5 MB, `.wasm` 141.2 MB (25.9 MB brotli), staged folder 330 MB, `CCA_2ms_WASM_v1.0.0.zip` 238.7 MB | `F:\UE5 Projects\CCA_WASM\Packaged\`. Every file under 200 MB. |
| — | Windows | — | — | Not packaged yet |
| — | Linux | — | — | Not packaged yet |
| — | Android | — | — | Not packaged yet |
