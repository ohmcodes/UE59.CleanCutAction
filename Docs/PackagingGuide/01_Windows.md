# 01 — Windows "Go Big" (RTX Showcase)

**Goal:** the highest-fidelity build of CleanCutAction: hardware ray tracing, Lumen HW RT, Virtual Shadow Maps, and (for the NVIDIA RTX Award) **NVIDIA DLSS**. Every Windows-only setting stays out of the other platforms.

Branch: `windows`. Checkout: `F:\UE5 Projects\CCA_Windows`. Award plan and form answers: `Docs/Submission/NVIDIA_RTX_Award.md`.

## Status (2026-11-15)

| Item | State |
|---|---|
| DX12 + SM6 | **Done**, but in `Config/DefaultEngine.ini` (it only affects Windows, since it's the Windows target section) |
| TSR anti-aliasing | **Done**: `Config/Windows/WindowsEngine.ini` (`r.AntiAliasingMethod=4`; the default is 0 = none) |
| HW ray tracing, Lumen HW RT, hit-lighting reflections, translucent front-layer reflections, bloom | **Done**: `Config/Windows/WindowsEngine.ini` |
| Device profile (Epic/Cinematic) | **Done**: `Config/Windows/WindowsDeviceProfiles.ini` |
| `Config/Windows/` committed | **No**: the folder is untracked |
| Nanite | **Off** project-wide (`r.Nanite.ProjectEnabled=False`). Not turned on for Windows |
| DLSS / Streamline plugin | **Not installed**. No `Plugins/` folder in the project; nothing in the engine |
| NVIDIA Reflex | Engine plugin exists (`Engine/Plugins/Runtime/Nvidia/Reflex`, Win64-only, off by default). **Not enabled** |
| In-game graphics menu | **None**. The pause menu has no graphics settings |
| Windows package | **Not built yet** |

---

## 1. Prerequisites

- [ ] Visual Studio with the C++ game development workload (the editor module already builds on this machine: `Binaries/Win64/UnrealEditor-CleanCutAction.dll`).
- [ ] An NVIDIA RTX GPU on the test machine (needed to verify ray tracing and DLSS).
- [ ] Latest NVIDIA driver.
- [ ] DLSS plugin downloaded by the user (section 1.5), only if the DLSS award is still the plan.

---

## 1.5 Download the NVIDIA DLSS plugin (user step)

The agent must not download this. It needs NVIDIA's license accepted by the user.

1. Go to **https://developer.nvidia.com/rtx/dlss** and find **"DLSS 4.5 Plugin for Unreal Engine"** (free NVIDIA Developer account).
2. Accept the license and take the newest UE version listed (5.9 Preview won't be listed; the plugin ships with source and recompiles against 5.9 in our C++ project).
3. Unzip. Keep the Quick Start Guide PDFs: the agent reads them for the cvar and Blueprint names (section 5).
4. Note the package version in the build records (README).
5. Copy the unzipped folder into `CCA_Windows\ThirdParty\NVIDIA_DLSS_Download\` and tell the agent to continue with section 4.

**Git note:** the plugin has large `.dll` files. Track `*.dll`, `*.lib`, `*.pdb` with Git LFS and keep the repo private (NVIDIA's license governs redistribution).

---

## 2. Windows RHI and shader model — done

Already in `Config/DefaultEngine.ini`:

```ini
[/Script/WindowsTargetPlatform.WindowsTargetSettings]
DefaultGraphicsRHI=DefaultGraphicsRHI_DX12
-D3D12TargetedShaderFormats=PCD3D_SM5
+D3D12TargetedShaderFormats=PCD3D_SM6
```

(`DefaultGraphicsRHI` is listed twice there; harmless.) No need to repeat it in `WindowsEngine.ini`.

---

## 3. Ray tracing, Lumen, shadows — done (Windows ini)

Project defaults in `DefaultEngine.ini` (all platforms): Lumen GI and reflections (`r.DynamicGlobalIlluminationMethod=1`, `r.ReflectionMethod=1`) in **software** mode, `r.RayTracing=False`, VSM on, `r.Nanite.ProjectEnabled=False`, `r.SkinCache.CompileShaders=1`, MegaLights off, Substrate on, `r.AntiAliasingMethod=0`, bloom off by default, `TargetedHardwareClass=Mobile` / `Scalable`.

`Config/Windows/WindowsEngine.ini` overrides for Windows only:

```ini
[/Script/Engine.RendererSettings]
r.AntiAliasingMethod=4                                   ; TSR
r.RayTracing=True                                        ; non-RT GPUs fall back to software Lumen
r.RayTracing.Shadows=False                               ; VSM stays the shadow method
r.Lumen.HardwareRayTracing=True
r.Lumen.HardwareRayTracing.LightingMode=2                ; hit lighting for reflections
r.Lumen.TranslucencyReflections.FrontLayer.EnableForProject=True
r.DefaultFeature.Bloom=True
```

- Changing `r.RayTracing` or `r.Lumen.HardwareRayTracing` recompiles Windows shaders and needs an editor restart.
- **Nanite is not enabled.** Turning on `r.Nanite.ProjectEnabled` would also need meshes converted to Nanite (asset change). Ask the user before doing either.

---

## 4. DLSS plugin install — not started (ask the user first)

1. Copy the plugin folders from `ThirdParty\NVIDIA_DLSS_Download\` (DLSS, Streamline and their dependencies, as shipped) into `CCA_Windows/Plugins/` (create it). Follow the Quick Start PDF if it differs.
2. Regenerate project files and build the editor target.
3. If it fails to compile against 5.9 Preview: report the first errors; don't patch plugin or engine source without permission; time-box (about one hour) before dropping DLSS.
4. Each added `.uplugin` module must have `"PlatformAllowList": [ "Win64" ]`. Ask before adding it if missing.
5. Enable the plugins (Edit → Plugins), restart.
6. NVIDIA recommends the Reflex bundled with Streamline over the engine's Reflex plugin. The engine one is off, so leave it off.
7. With Frame Generation on, disable VSync (`r.vsync 0`).

**Branch note:** `Plugins/` must exist only on the `windows` branch. Don't merge it into `wasm`, `linux` or `android`.

---

## 5. DLSS runtime setup — not started

**VERIFY:** cvar and Blueprint names change between plugin versions. Use the names in the plugin's PDF; these are a starting point only.

```ini
; Config/Windows/WindowsEngine.ini
[SystemSettings]
r.NGX.DLSS.Enable=1
r.Streamline.DLSSG.Enable=1   ; Frame Generation, DX12 only
```

- Wrap all C++ that touches DLSS in `#if PLATFORM_WINDOWS`.
- Add the DLSS module to `Source/CleanCutAction/CleanCutAction.Build.cs` only for Win64:
  ```csharp
  if (Target.Platform == UnrealTargetPlatform.Win64)
  {
      PrivateDependencyModuleNames.Add("VERIFY_DLSSBlueprintModuleName");
  }
  ```

---

## 6. In-game graphics menu — not started

The game has no graphics settings yet (pause menu: Resume, Leaderboard, Unstuck, Restart, Music, Quit). For the award, add a Windows-only section to the pause menu (`Source/CleanCutAction/UI/LTTPauseMenu.*`):

- **DLSS Super Resolution:** Off / DLAA / Quality / Balanced / Performance (modes the plugin reports as supported).
- **Frame Generation:** On / Off.
- **Ray Reconstruction:** On / Off (needs HW ray tracing, which is now on).
- Hide the section when DLSS isn't supported (non-RTX GPU); TSR is then the upscaler.
- Must be reachable with a gamepad (use `ULTTInputDeviceSubsystem::EnsureGamepadFocus`, like the rest of the pause menu).

---

## 7. Extras (optional)

- **NVIDIA Reflex:** engine plugin, off. Not on the award list.
- **Photo mode with the Path Tracer:** not done. `r.PathTracing=False` project-wide, and `NNEDenoiser` is disabled to keep the package small, so this would need both turned back on for Windows only.

---

## 8. Device profile — done

`Config/Windows/WindowsDeviceProfiles.ini`:

```ini
[Windows DeviceProfile]
+CVars=sg.ViewDistanceQuality=3
+CVars=sg.AntiAliasingQuality=4
+CVars=sg.ShadowQuality=4
+CVars=sg.GlobalIlluminationQuality=4
+CVars=sg.ReflectionQuality=4
+CVars=sg.PostProcessQuality=4
+CVars=sg.TextureQuality=3
+CVars=sg.EffectsQuality=3
+CVars=sg.FoliageQuality=3
+CVars=sg.ShadingQuality=4
```

No texture caps: Windows cooks full resolution. Device profile cvars override the scalability level; fine while there is no graphics menu.

---

## 9. Input

- Keyboard/mouse and gamepad (XInput; Xbox-layout prompts). The prompts follow the last device used (`Source/CleanCutAction/UI/LTTInputDevice.h`). Mouse clicks or keys only switch away from the gamepad after 1.5 s with no gamepad input.
- Touch controls only appear on touch devices (`ShouldUseTouchControls`), not on a desktop.
- Gamepad layout: see `04_Linux_SteamDeck.md` section 5.

---

## 10. Online leaderboard

Desktop builds post runs to `https://cca.ohmcodes.com` through `FHttpModule` (the `HTTP` module is in `CleanCutAction.Build.cs`). The player id and name are kept in a save slot (`ULTTPlayerProfileSave`). Override the server with `-LeaderboardUrl=...` or `BaseUrl` in `DefaultGame.ini`. Firewalls may prompt on first run.

---

## 11. Build

```
Docs\PackagingGuide\Build_Platform.bat Win64
```

Output: `Packaged\Windows\`. Ship it to itch.io as a **downloadable** (zip) next to the browser build; only the Windows build can enter the RTX award.

---

## 12. Checklist

- [x] DX12 + SM6 set (`DefaultEngine.ini`, Windows target section)
- [x] TSR, HW ray tracing, Lumen HW RT, VSM on (Windows ini)
- [ ] Nanite (off; ask before enabling)
- [x] Windows device profile (Epic/Cinematic, full-res textures)
- [ ] `Config/Windows/` committed
- [ ] DLSS plugin downloaded by the user, version recorded
- [ ] DLSS plugin compiles; `.uplugin` modules restricted to Win64
- [ ] DLSS SR verified in-game (`stat unit` before/after, or the plugin's indicator)
- [ ] Frame Generation verified on an RTX 40/50 GPU, or noted as untested
- [ ] Graphics menu shows DLSS options only when supported, and works with a gamepad
- [ ] Packaged build launches on an RTX machine and a non-RTX machine (falls back to software Lumen + TSR)
- [ ] Common verification in README done
- [ ] Submission note lists exactly which DLSS features are active and where the toggles are
