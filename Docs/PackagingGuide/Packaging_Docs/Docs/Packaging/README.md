# CleanCutAction — Packaging Guide (Index)

This folder tells an agent (Claude Code / Cowork) or a teammate exactly how to set up and package CleanCutAction for four targets from **one project**.

| Doc | Target | Renderer | Priority |
|---|---|---|---|
| `01_Windows.md` | Windows "Go Big" (RTX showcase, DLSS) | Desktop, DX12 SM6, HW ray tracing | 1 — prize-relevant |
| `02_Web_WASM.md` | Browser (desktop + mobile), WebAssembly/WebGPU | Mobile renderer | 2 — riskiest, current target |
| `03_Android.md` | Android APK (sideload) | Mobile renderer, Vulkan + ES3.1 | 3 |
| `04_Linux_SteamDeck.md` | Linux / Steam Deck | Desktop, Vulkan SM5 | 4 |

Scripts live in `Scripts/`. Record every build result in `BUILD_LOG.md` (create it if missing).

---

## Engine context

- Engine: **Unreal Engine 5.9 MegaJam Preview, source build** (required for the Web target).
- Project: C++ project (required for the Web target and for compiling the DLSS plugin).
- Project root: `F:\UE5 Projects\CleanCutAction\` (verify the `.uproject` name before scripting).
- The preview is experimental. Expect rough edges in the Web platform especially.

---

## How an agent should use these docs

1. Read this README fully, then read **one** platform doc.
2. Work on a dedicated git branch: `packaging/windows`, `packaging/web`, `packaging/android`, `packaging/linux`.
3. Implement the platform doc **section by section, in order**. Commit after each section with a message naming the section.
4. Tick the platform doc's checklist as you go and report which items are done, skipped or blocked.
5. When anything in a doc is marked **VERIFY**, look it up in the engine source/config or ask the user. Never guess.

---

## Non-negotiable rules (all platforms)

1. **Never modify, downscale or re-import source assets for a platform.** Texture size and quality limits go in device profiles or per-platform overrides only (see "Texture policy" below).
2. **`Config/Default*.ini` is only for settings valid on ALL platforms.** Platform-specific settings go in `Config/<Platform>/<Platform>Engine.ini`, `<Platform>Game.ini`, etc., or in platform sections of `DefaultDeviceProfiles.ini`.
3. **NVIDIA plugins (DLSS, Streamline) are Windows-only.** They must never be compiled or cooked into Linux, Android or Web builds.
4. **Never put API keys or secrets** in the project, config, Blueprints or source code.
5. **Stop and ask the user before:**
   - modifying engine source
   - installing, upgrading or removing plugins
   - deleting or moving assets
   - changing a project-wide rendering default in `DefaultEngine.ini`
   - any change that forces a full shader recompile for every platform (warn about the time cost first)
6. Prefer the smallest, reversible change. If a setting can be done per-platform, do it per-platform.

---

## Texture policy (read before touching textures)

Earlier web work may have optimized textures. Before packaging any platform, check:

1. Open an affected texture. Compare **Imported** resolution vs **Displayed / Max In-Game** resolution.
   - Imported still original (e.g. 4096) → only properties changed. Revert `Maximum Texture Size`, `LOD Bias`, `Compression Settings`, `LOD Group` to their originals. No reimport needed.
   - Imported itself is small → source data was replaced. Restore the `.uasset` from the pre-web branch (step 2).
2. List what changed and restore from git rather than re-importing:
   ```
   git diff --name-only <pre-web-branch>..HEAD -- Content/ Config/
   git checkout <pre-web-branch> -- Content/Path/To/Textures/
   git lfs pull
   ```
3. Check the `Config/` part of that diff for global texture caps in `DefaultEngine.ini`, `DefaultDeviceProfiles.ini` or `DefaultScalability.ini`. Move any platform-specific caps to that platform's device profile.

Per-platform limits are then applied at cook time:
- Device profiles: `+TextureLODGroups=(Group=TEXTUREGROUP_World,MaxLODSize=1024,LODBias=0)` style entries inside the platform's profile.
- Per-asset: the per-platform override arrow next to `Maximum Texture Size` / `LOD Bias` in the texture details.

---

## Shared packaging settings

Set once in **Project Settings → Packaging** (written to `Config/DefaultGame.ini`):

```ini
[/Script/UnrealEd.ProjectPackagingSettings]
BuildConfiguration=PPBC_Shipping
UsePakFile=True
bUseIoStore=True
bShareMaterialShaderCode=True
bSharedMaterialNativeLibraries=True
IncludePrerequisites=True
; Only the maps the game actually needs. Keeps every build (especially Web) small.
+MapsToCook=(FilePath="/Game/Maps/VERIFY_MainMap")
```

**VERIFY:** list the real map paths with the user before writing `MapsToCook`.

Use `Full Rebuild` only for final submission builds.

---

## Build output layout

```
F:\UE5 Projects\CleanCutAction\Builds\
    Windows\
    Web\
    Android\
    SteamDeck\
```

Add `Builds/` to `.gitignore`. Use `Scripts/Build_Platform.bat <Win64|Web|Android|Linux>` to produce each package.

---

## Common verification (every platform)

- [ ] Package completes with no errors. Read the UAT log for cook warnings about missing assets or shaders.
- [ ] Build launches and reaches gameplay on the target device.
- [ ] Input works with that platform's primary input (mouse/keyboard, gamepad, touch).
- [ ] Package size recorded in `BUILD_LOG.md`, with date, branch and commit hash.
- [ ] No NVIDIA plugin files in non-Windows packages (search the staged folder for `DLSS`, `Streamline`, `nvngx`).
