# 01 — Windows "Go Big" (RTX Showcase)

**Goal:** the highest-fidelity build of CleanCutAction. Hardware ray tracing, Lumen HW RT, Nanite, Virtual Shadow Maps, and **NVIDIA DLSS** (for the NVIDIA RTX Award). Every Windows-only feature must stay out of the other platforms.

Branch: `packaging/windows`

---

## 1. Prerequisites

- [ ] Visual Studio with the C++ game development workload (versions per the 5.9 source build's requirements).
- [ ] An NVIDIA RTX GPU on the test machine (needed to verify DLSS).
- [ ] Latest NVIDIA driver.
- [ ] NVIDIA DLSS Unreal Engine plugin downloaded (see section 1.5; the user does this, not the agent).

---

## 1.5 Download the NVIDIA DLSS plugin (user step)

The agent must not download this. It requires accepting NVIDIA's license agreement, which only the user can do.

1. Go to **https://developer.nvidia.com/rtx/dlss** and find **"DLSS 4.5 Plugin for Unreal Engine"**. NVIDIA may ask you to sign in with a free NVIDIA Developer account.
2. Accept the license agreement and pick the download for your engine version.
   - UE 5.9 MegaJam Preview is days old and probably not listed. Download the **newest version listed** (5.8 or 5.7). The plugin ships with source code, so it recompiles against 5.9 inside our C++ project (section 4).
   - Older versions are on NVIDIA's "DLSS: Unreal Engine Plugin Archive" page, linked from the same site.
3. Unzip. Expect:
   - plugin folders (DLSS / Super Resolution, Streamline for Frame Generation + Reflex, and their dependencies)
   - a `Samples/` folder with a test project
   - Quick Start Guide PDFs. **Keep these. The agent reads them for the correct cvar and Blueprint names (section 5).**
4. Write down the package version in `BUILD_LOG.md`. The October 2026 package lists Streamline 2.14.16 / NGX 310.9.16; yours may be newer.
5. Copy the unzipped folder into `CleanCutAction\ThirdParty\NVIDIA_DLSS_Download\` so the agent can find the plugins and PDFs, then tell the agent to continue with section 4.

**Git note:** the plugin contains large `.dll` files. Track them with Git LFS (`*.dll`, `*.lib`, `*.pdb` in `.gitattributes`) and keep the repo private, since redistribution is governed by NVIDIA's license.

---

## 2. Windows RHI and shader model

Write to `Config/Windows/WindowsEngine.ini` (create if missing):

```ini
[/Script/WindowsTargetPlatform.WindowsTargetSettings]
DefaultGraphicsRHI=DefaultGraphicsRHI_DX12
-D3D12TargetedShaderFormats=PCD3D_SM5
+D3D12TargetedShaderFormats=PCD3D_SM6
```

---

## 3. Ray tracing, Lumen, Nanite, shadows

These are renderer settings. Put them in `Config/Windows/WindowsEngine.ini` so they apply only to Windows.

```ini
[/Script/Engine.RendererSettings]
r.RayTracing=True
r.SkinCache.CompileShaders=True
r.DynamicGlobalIlluminationMethod=1
r.ReflectionMethod=1
r.Lumen.HardwareRayTracing=True
r.Shadow.Virtual.Enable=1
r.Nanite.ProjectEnabled=True
```

- `r.RayTracing` and `r.SkinCache.CompileShaders` need an editor restart and shader recompile. **Warn the user before applying.**
- **VERIFY:** if any of these keys already exist in `Config/DefaultEngine.ini` with different values, do not edit the Default file. The Windows file overrides it.

---

## 4. DLSS plugin install (ask the user before starting)

1. Copy the plugin folders from `ThirdParty\NVIDIA_DLSS_Download\` (DLSS, Streamline, and their dependencies, exactly as shipped) into `CleanCutAction/Plugins/`. Follow the Quick Start Guide PDF if its steps differ from these.
2. Regenerate project files and build the editor target from Visual Studio.
3. If it fails to compile against 5.9 Preview:
   - Report the first errors to the user.
   - Do not patch plugin or engine source without permission.
   - Agree on a time-box (suggested: one hour) before abandoning DLSS.
4. Open every `.uplugin` that was added and confirm each module has:
   ```json
   "PlatformAllowList": [ "Win64" ]
   ```
   If missing, ask the user before adding it.
5. Enable the plugins in the editor (Edit → Plugins). Restart.
6. NVIDIA recommends using the Reflex bundled with the Streamline plugin instead of UE's built-in Reflex plugin. If both are present, **ask the user** before disabling the built-in one.
7. When Frame Generation is on, disable VSync (`r.vsync 0`). NVIDIA notes VSync can misbehave with Frame Generation active.

---

## 5. DLSS runtime setup

**VERIFY:** cvar and Blueprint function names change between plugin versions. Read the documentation PDF bundled with the plugin and use the names it gives. The names below are a starting point only.

Default-on settings in `Config/Windows/WindowsEngine.ini`:

```ini
[SystemSettings]
r.NGX.DLSS.Enable=1
; Frame Generation via Streamline (DX12 only)
r.Streamline.DLSSG.Enable=1
```

Gameplay code:

- All C++ that references DLSS types or headers must be wrapped:
  ```cpp
  #if PLATFORM_WINDOWS
  // DLSS calls here
  #endif
  ```
- Add the DLSS module to `CleanCutAction.Build.cs` only for Win64:
  ```csharp
  if (Target.Platform == UnrealTargetPlatform.Win64)
  {
      PrivateDependencyModuleNames.Add("VERIFY_DLSSBlueprintModuleName");
  }
  ```
- In Blueprint, branch on `Get Platform Name == "Windows"` before calling DLSS nodes.

---

## 6. In-game graphics menu (makes the feature visible to judges)

Add a Windows-only section in the graphics settings UI:

- **DLSS Super Resolution mode:** Off / Quality / Balanced / Performance / DLAA (use the modes the plugin reports as supported).
- **Frame Generation:** On / Off.
- **Ray Reconstruction:** On / Off (needs HW ray tracing).
- Hide the whole section when the plugin reports DLSS is not supported (non-RTX GPU). TSR is then the upscaler.

---

## 7. Extras (optional, cheap)

- **NVIDIA Reflex:** built into UE5. Enable it in Edit → Plugins if available and expose a toggle.
- **Photo mode with the Path Tracer:** a key that switches the viewport to path tracing for screenshots.

---

## 8. Device profile

In `Config/DefaultDeviceProfiles.ini`:

```ini
[Windows DeviceProfile]
DeviceType=Windows
BaseProfileName=
+CVars=sg.ShadowQuality=3
+CVars=sg.GlobalIlluminationQuality=3
+CVars=sg.ReflectionQuality=3
+CVars=sg.TextureQuality=3
```

No texture caps here. Windows cooks at full resolution.

---

## 9. Build

```
Scripts\Build_Platform.bat Win64
```

Output: `Builds\Windows\`

---

## 10. Checklist

- [ ] DX12 + SM6 set in the Windows ini only
- [ ] HW ray tracing, Lumen HW RT, VSM, Nanite on (Windows ini)
- [ ] DLSS plugin downloaded by the user, version recorded in BUILD_LOG.md
- [ ] DLSS plugin compiles; `.uplugin` modules restricted to Win64
- [ ] DLSS SR verified in-game (`stat unit` before/after, or the plugin's on-screen indicator)
- [ ] Frame Generation verified on an RTX 40/50 GPU, or noted as untested
- [ ] Graphics menu shows DLSS options only when supported
- [ ] Packaged build launches on an RTX machine and a non-RTX machine (falls back cleanly)
- [ ] Common verification in README done
- [ ] Note for submission: list exactly which DLSS features are active and where the toggles are
