# 04 — Linux / Steam Deck

**Goal:** a native Linux build tuned for the Steam Deck (AMD APU, 1280×800, gamepad, Vulkan), cross-compiled from Windows.

Branch: `linux`. Checkout: `F:\UE5 Projects\CCA_Linux`.

## Status (2026-11-15)

- Branch has the shared fixes (`package fixes`, gamepad support, gamepad prompts) and **no Linux-specific config**: no `Config/Linux/`, no Linux device profile. Its `Config/` and `.uproject` match `windows` (without `Config/Windows/`).
- Not packaged yet. Toolchain install not confirmed (`LINUX_MULTIARCH_ROOT` isn't set in the agent's shell; VERIFY on the build machine).
- Gamepad support is in and was written with the Deck and Steam Input in mind (section 5).

**Alternatives on the Deck** if the native build isn't ready: the Windows build through Proton, or the web build in the Deck's browser (gamepad works there; the touch overlay hides while the gamepad is used). Record the choice with the build records (README).

---

## 1. Prerequisites

- [ ] Linux cross-compile toolchain for UE 5.9: **`v26_clang-20.1.8-rockylinux8`** (from `Engine/Config/Linux/Linux_SDK.json`; min = max = this version).
- [ ] `LINUX_MULTIARCH_ROOT` set by the toolchain installer. Restart the editor afterwards.
- [ ] Linux shows in Platforms → Linux. The installed engine ships Linux binaries (`Engine/Binaries/Linux`).
- [ ] A Steam Deck for testing.

---

## 2. Target RHI

Currently in `Config/DefaultEngine.ini` (shared):

```ini
[/Script/LinuxTargetPlatform.LinuxTargetSettings]
-TargetedRHIs=SF_VULKAN_SM5
+TargetedRHIs=SF_VULKAN_SM6
```

So Linux targets **Vulkan SM6** only. If SM6 fails or runs poorly on the Deck, create `Config/Linux/LinuxEngine.ini` on the `linux` branch (don't change the Default file):

```ini
[/Script/LinuxTargetPlatform.LinuxTargetSettings]
-TargetedRHIs=SF_VULKAN_SM6
+TargetedRHIs=SF_VULKAN_SM5
```

VERIFY on the Deck which one runs better.

---

## 3. Renderer

Project defaults Linux inherits today: software Lumen GI + reflections, VSM on, Nanite off, HW RT off, and **`r.AntiAliasingMethod=0` (no AA)**. Windows fixes the AA in its own ini; Linux doesn't yet. Proposed `Config/Linux/LinuxEngine.ini`:

```ini
[/Script/Engine.RendererSettings]
r.AntiAliasingMethod=4          ; TSR (the default is none)
; Software Lumen. Switch to 0 (none) if the Deck can't hold the frame rate.
r.DynamicGlobalIlluminationMethod=1
r.ReflectionMethod=1
r.Shadow.Virtual.Enable=0       ; cheaper shadow maps on the Deck
```

An FSR plugin is an option; ask the user before adding any plugin.

---

## 4. Device profile (Deck-tuned) — not done

`Config/Linux/LinuxDeviceProfiles.ini` (per-platform file, same pattern as `Config/Windows/WindowsDeviceProfiles.ini`):

```ini
[Linux DeviceProfile]
+CVars=sg.ShadowQuality=1
+CVars=sg.GlobalIlluminationQuality=1
+CVars=sg.ReflectionQuality=1
+CVars=sg.PostProcessQuality=2
+CVars=sg.EffectsQuality=1
+CVars=sg.TextureQuality=2
+CVars=r.ScreenPercentage=75
+CVars=t.MaxFPS=40
+TextureLODGroups=(Group=TEXTUREGROUP_World,MinLODSize=1,MaxLODSize=2048,LODBias=0,MinMagFilter=aniso,MipFilter=point,MipGenSettings=TMGS_SimpleAverage)
+TextureLODGroups=(Group=TEXTUREGROUP_Character,MinLODSize=1,MaxLODSize=2048,LODBias=0,MinMagFilter=aniso,MipFilter=point,MipGenSettings=TMGS_SimpleAverage)
```

Default resolution: `Config/Linux/LinuxGameUserSettings.ini` (**VERIFY** that this override is read; otherwise set it in code on first launch, Linux only):

```ini
[/Script/Engine.GameUserSettings]
ResolutionSizeX=1280
ResolutionSizeY=800
FullscreenMode=1
```

---

## 5. Input and UI — done (shared code)

`ULTTInputDeviceSubsystem` (`Source/CleanCutAction/UI/LTTInputDevice.h`) tracks the last device (keyboard/mouse, gamepad, touch) with a Slate input pre-processor. HUD legends, the interact prompt, station instructions and menus all follow it.

- **Steam Input:** the trackpads move the mouse while the rest is a gamepad, so mouse movement alone never switches away from the gamepad. Mouse clicks and keys only switch after **1.5 s with no gamepad input** (Steam Input sends trackpad clicks and some buttons as mouse/keyboard).
- **Prompts:** Xbox / Steam Deck labels (A, B, X, Y, LB, RB, LT, RT, L3, R3, D-pad, Menu, View).
- **Menus:** briefing, pause, end screens, leaderboard and the name keyboard all take gamepad focus (`EnsureGamepadFocus`). B backs out. A on the name box opens the on-screen keyboard.
- **Touch overlay** hides while a gamepad drives a touch device.

| Action | Gamepad | Keyboard / mouse |
|---|---|---|
| Move / Look | Left stick / Right stick | WASD / Mouse |
| Pick up / Place | X | Left click / E |
| Throw (breaks it!) | RT | Right click |
| Jump | A | Space |
| Crouch | R3 | C / Ctrl |
| Hint | Y | F |
| Dance | D-pad Up | G |
| Pause / Unstuck | Menu | P / Esc |
| Briefing: Time Trial / Just Clean | A or Menu / Y | Enter or Space / N |
| Briefing: pages / leaderboard | LB, RB or D-pad / View | Arrows or A, D / L |
| Station action / leave | RT / B | Left click / E or right click |
| Station choice (light gels) | LB, RB or D-pad | 1-3 or mouse wheel |

To check on the Deck:

- [ ] UI text readable at 1280×800 on a 7" screen (check in the editor with a 1280×800 viewport).
- [ ] No mouse-only interactions left (station mini-games accept RT / sticks / bumpers).

---

## 6. Plugins and code

- No DLSS/Streamline (Windows branch only). `AllToolsets` and `NNEDenoiser` off.
- Leaderboard uses `FHttpModule` on Linux (the `HTTP` module is linked); the player profile is a save slot.
- Linux-only C++ goes in `#if PLATFORM_LINUX`.

---

## 7. Build and deploy

```
Docs\PackagingGuide\Build_Platform.bat Linux
```

Output: `Packaged\Linux\`. On the Deck:

1. Desktop Mode → copy the folder over (USB drive or network share).
2. `chmod +x CleanCutAction.sh`
3. Steam → "Add a Non-Steam Game" → pick the `.sh`. Launch from Gaming Mode with the default gamepad layout.

---

## 8. Checklist

- [ ] Toolchain `v26_clang-20.1.8-rockylinux8` installed; `LINUX_MULTIARCH_ROOT` set
- [ ] RHI decided (SM6 now; SM5 in a Linux ini if needed)
- [ ] TSR on for Linux (default AA is none)
- [ ] HW RT off (already); Lumen software or off
- [ ] Linux device profile (75% screen percentage, 40 fps cap)
- [ ] Launches at 1280×800 fullscreen on the Deck
- [x] Gamepad layout, prompts and menu focus (shared code)
- [ ] Fully playable with Deck controls through Steam Input, menus included
- [ ] Holds the frame cap in the heaviest scene (record FPS)
- [ ] No DLSS/Streamline files in the package
- [ ] Common verification in README done
- [ ] If the native build was abandoned: Proton or browser fallback tested and recorded
