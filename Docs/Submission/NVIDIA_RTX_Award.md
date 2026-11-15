# Render Royalty: NVIDIA RTX Award (Windows build)

This file has three parts: what the project uses today, the plan for the `windows` branch, and the form answers. Fill in the form only with features that are in the packaged Windows build and checked on an RTX GPU.

---

## 1. Where the project stands today (checked 2026-11-15)

| NVIDIA feature | In the project now? | Can we add it on UE 5.9 MegaJam Preview? |
|---|---|---|
| **NVIDIA DLSS** (Super Resolution, Frame Generation, Ray Reconstruction) | No. The DLSS/Streamline plugin isn't in `Plugins/` or the engine. | **Yes, most likely.** NVIDIA's DLSS 4.5 UE plugin ships with source, so it can be built against 5.9 in our C++ project. Biggest risk: preview API changes break the compile. |
| RTX Mega Geometry | No | **No.** Needs the NvRTX engine branch, and there is no NvRTX build of 5.9 MegaJam Preview. |
| RTX Direct Illumination (RTXDI) | No | **No.** NvRTX only. UE's MegaLights is Epic's feature, not RTXDI. |
| RTX Indirect Illumination (ReSTIR GI) | No | **No.** NvRTX only. Lumen with hardware ray tracing is Epic's feature and doesn't count. |
| RTX Path Tracing (ReSTIR PT) | No | **No.** NvRTX only. UE's built-in Path Tracer is Epic's feature. |
| LSS Hair | No | **No.** NvRTX only, and the game has no strand hair. |
| NVIDIA ACE | No | Not in a jam timeframe (it would need cloud/NIM setup plus API keys, and keys can't ship in the build). |

Also note:

- The **Web (Wasm/WebGPU) build can't use any of these**. Only the Windows package can enter the award, so it has to be uploaded to itch.io as a downloadable next to the browser build.
- The Reflex plugin in the engine (`Engine/Plugins/Runtime/Nvidia/Reflex`) isn't on the award list. It's a nice extra but doesn't count.
- **Only tick "NVIDIA DLSS"**, and only after DLSS is verified in the packaged build. Ticking a feature the judges can't find hurts the entry.

---

## 2. Plan for the `windows` branch

Follows `Docs/PackagingGuide/01_Windows.md`. The steps marked ⚠ need your OK first, because they trigger a full shader recompile (for Windows only) or change plugins.

1. **Already done:** DX12 + SM6 are set (`DefaultEngine.ini`), Lumen GI and reflections are on, Virtual Shadow Maps are on.
2. **Done in this pass:** created `Config/Windows/WindowsEngine.ini`, which turns on TSR anti-aliasing for Windows only. The project had `r.AntiAliasingMethod=0` (no AA) from the template. That matters for "best visuals", and DLSS replaces TSR as the upscaler anyway.
3. **Done (2026-11-15):** "Go Big" graphics for Windows only.
   - `Config/Windows/WindowsEngine.ini`: hardware ray tracing on, Lumen GI and reflections on hardware ray tracing, hit lighting for reflections (`LightingMode=2`), Lumen reflections on translucent front layers (glass), default bloom on. VSM stays the shadow method. Ray Reconstruction (DLSS-RR) needs this. GPUs without ray tracing fall back to software Lumen.
   - `Config/Windows/WindowsDeviceProfiles.ini`: Cinematic GI, reflections, shadows, post-process and shading; Epic for the rest; full-resolution textures.
4. **Done:** DLSS 4.5 plugin package v8.9.0 (for UE 5.8, dated 2026-10-16) downloaded by the user.
5. **Done:** `DLSS`, `StreamlineCore`, `StreamlineNGXCommon`, `StreamlineDLSSG` and `StreamlineReflex` copied into `Plugins/` (without the 5.8 editor DLLs, `.pdb` files and `Intermediate/`), enabled in the `.uproject` with `"PlatformAllowList": ["Win64"]`. Left out: NIS, RTX Dynamic Vibrance, Movie Render Queue support and the deprecated `Streamline` stub.
   - **5.9 fix:** 5.9 checks printf format strings at compile time and the plugin passes enums to `%d`/`%u`. Each plugin `*.Build.cs` now sets `bValidateFormatStrings = false;` (marked `NOTE(5.9)`), plus a few casts in the log lines. No engine changes.
   - The Win64 Development game target compiles and links with no errors or warnings.
6. **Done:** `Source/CleanCutAction/UI/LTTGraphicsSettings.*` turns on DLSS-SR Quality, Frame Generation (Auto: 2x on RTX 40, multi-frame on RTX 50, with Reflex on and VSync off) and Ray Reconstruction at game start, only where the GPU supports each. Compiled only on Win64 (`WITH_LTT_DLSS`).
7. **Done:** the pause menu adds three buttons before Quit, shown only when DLSS is supported: **NVIDIA DLSS** (cycles Off (TSR) / DLAA / Quality / Balanced / Performance), **Frame Generation** On/Off, **Ray Reconstruction** On/Off.
8. Verify in the package: the plugin's on-screen indicator or `stat unit` before and after, on an RTX GPU and on a non-RTX GPU (it should fall back to TSR cleanly). Take a before/after screenshot pair for the submission.

If step 5 fails, the Windows build still makes a better **Render Royalty** entry than the web build (full-resolution textures, TSR, Lumen, VSM). Just leave the NVIDIA award blank.

---

## 3. Form answers

### Optional: Are you entering the UE Render Royalty NVIDIA RTX Award?

Check only what is verified in the packaged build:

- [ ] RTX Mega Geometry
- [ ] RTX Direct Illumination
- [ ] RTX Indirect Illumination
- [ ] RTX Path Tracing
- [ ] LSS Hair
- [x] **NVIDIA DLSS** (once steps 4–8 pass)
- [ ] NVIDIA ACE

### Optional: How did you use the NVIDIA features you checked above

**Draft. Delete any sentence for a feature that didn't make it into the build:**

> **NVIDIA DLSS 4.5 (Windows build):** DLSS Super Resolution (Quality by default) upscales the two-floor New York cafe set, so the Windows build can run hardware ray-traced Lumen global illumination and reflections, with hit lighting for reflections, at Cinematic settings while holding a smooth frame rate during the Phase 2 shoot, when the player sprints between effect stations while rain, steam and thunder-flash effects run. DLSS Frame Generation is on by default on RTX 40/50 GPUs (Auto: 2x on RTX 40, Multi Frame Generation on RTX 50), with NVIDIA Reflex on to keep input responsive. DLSS Ray Reconstruction replaces the denoiser for the ray-traced Lumen reflections on the cafe's windows and glossy surfaces. All three can be toggled live in the pause menu (**NVIDIA DLSS**: Off (TSR) / DLAA / Quality / Balanced / Performance, **Frame Generation** On/Off, **Ray Reconstruction** On/Off), so judges can compare against TSR. The buttons only appear on GPUs that support each feature; on other GPUs the game falls back to TSR.
