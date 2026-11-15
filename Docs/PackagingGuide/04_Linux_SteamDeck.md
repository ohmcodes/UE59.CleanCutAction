# 04 — Linux / Steam Deck

**Goal:** a native Linux build tuned for the Steam Deck (AMD APU, 1280×800, gamepad, Vulkan), cross-compiled from Windows.

Branch: `packaging/linux`

**Fallback:** if the native build cannot be made stable in time, the Windows build running through Proton on the Deck is an accepted alternative. Record that decision in `BUILD_LOG.md`.

---

## 1. Prerequisites

- [ ] Linux cross-compile toolchain installed on the Windows build machine. Use the toolchain version that UE 5.9's Linux docs list for this engine (**VERIFY**; versions must match).
- [ ] Environment variable `LINUX_MULTIARCH_ROOT` set by the toolchain installer. Restart the editor afterwards.
- [ ] Linux shows as an available platform in Platforms → Linux.
- [ ] Access to a Steam Deck for testing (Desktop Mode → copy the build over, or add it as a non-Steam game).

---

## 2. Target RHI

`Config/Linux/LinuxEngine.ini`:

```ini
[/Script/LinuxTargetPlatform.LinuxTargetSettings]
-TargetedRHIs=SF_VULKAN_SM6
+TargetedRHIs=SF_VULKAN_SM5
```

**VERIFY** the available RHI enum names in the 5.9 source before writing them.

---

## 3. Renderer

Same file (`Config/Linux/LinuxEngine.ini`):

```ini
[/Script/Engine.RendererSettings]
r.RayTracing=False
r.Lumen.HardwareRayTracing=False
; Software Lumen. Switch to 0 (none) if the Deck cannot hold the frame rate.
r.DynamicGlobalIlluminationMethod=1
r.ReflectionMethod=1
r.Shadow.Virtual.Enable=0
r.AntiAliasingMethod=4
```

- `r.AntiAliasingMethod=4` = TSR. An FSR plugin is an option, but **ask the user** before adding any plugin.
- Nanite can stay on (desktop renderer). Check performance.

---

## 4. Device profile (Deck-tuned)

```ini
; Config/DefaultDeviceProfiles.ini
[Linux DeviceProfile]
DeviceType=Linux
BaseProfileName=
+CVars=sg.ShadowQuality=1
+CVars=sg.GlobalIlluminationQuality=1
+CVars=sg.ReflectionQuality=1
+CVars=sg.PostProcessQuality=2
+CVars=sg.EffectsQuality=1
+CVars=sg.TextureQuality=2
+CVars=r.ScreenPercentage=75
+CVars=t.MaxFPS=40
+TextureLODGroups=(Group=TEXTUREGROUP_World,MaxLODSize=2048,LODBias=0)
+TextureLODGroups=(Group=TEXTUREGROUP_Character,MaxLODSize=2048,LODBias=0)
```

Default resolution: `Config/Linux/LinuxGameUserSettings.ini` (**VERIFY** that this override filename is read; otherwise set the defaults in code on first launch for Linux only):

```ini
[/Script/Engine.GameUserSettings]
ResolutionSizeX=1280
ResolutionSizeY=800
FullscreenMode=1
```

---

## 5. Input and UI

- Every action reachable by gamepad, including menus (focus navigation in UMG).
- Deck button glyphs if the project shows button prompts (Xbox-style glyphs are an acceptable default).
- Minimum UI text size readable on a 7" screen at 1280×800. Check in the editor with a 1280×800 viewport.
- No mouse-only interactions.

---

## 6. Plugins and code

- DLSS / Streamline excluded (Win64-only `PlatformAllowList`).
- Any Linux-specific C++ goes in `#if PLATFORM_LINUX` blocks.

---

## 7. Build and deploy

```
Scripts\Build_Platform.bat Linux
```

Output: `Builds\SteamDeck\`

On the Deck:

1. Desktop Mode → copy the folder over (USB drive or network share).
2. `chmod +x CleanCutAction.sh`
3. In Steam, "Add a Non-Steam Game" → pick the `.sh`. Launch from Gaming Mode.

---

## 8. Checklist

- [ ] Toolchain version matches the engine; `LINUX_MULTIARCH_ROOT` set
- [ ] Vulkan SM5 target in the Linux ini only
- [ ] HW RT off; Lumen software or off; TSR
- [ ] Linux device profile tuned (75% screen percentage, 40 fps cap)
- [ ] Launches at 1280×800 fullscreen on the Deck
- [ ] Fully playable with Deck controls, menus included
- [ ] Holds the frame cap in the heaviest scene (record FPS in BUILD_LOG.md)
- [ ] No DLSS/Streamline files in the package
- [ ] Common verification in README done
- [ ] If the native build was abandoned: Proton fallback tested and recorded
