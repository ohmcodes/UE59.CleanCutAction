# 10 - VR / Meta Quest Port (`vr` branch)

The `vr` branch turns *Clean! Cut! Action!* into a Meta Quest VR game for the **Meta VR Start Developer Competition 2026** (Devpost). The flat desktop, web (Wasm) and Android builds live on their own branches (`main`, `wasm`, `android`, `windows`, `linux`); this branch is Quest-only.

This file records every change made for VR, how the VR systems work, how to test without a headset, and what's left.

---

## 1. Competition summary

| | |
|---|---|
| Competition | Meta VR Start Developer Competition 2026 (`start-developer-competition-26.devpost.com`) |
| Deadline | **2026-11-18 12:00 PM PT** (2026-11-19 04:00 Manila). No changes after the deadline. |
| Track / division | Gaming / **New Experience** (first commit 2026-11-11, after the 2026-09-24 window start) |
| Hard requirement | **Hands-first**: the whole game must be completable with hand tracking, no controller ever paired |
| Guidelines (scored) | Seated, 2 ft radius ("airplane seat test"); satisfying in 10 minutes or less; fast cold start; clean pause/resume; 60+ fps on Quest |
| Submit | APK in a Meta Developer Dashboard release channel named **"Competition"** + invite URL; public YouTube/Vimeo video under 3 min (headset or XR Simulator footage, real gameplay); Devpost form |
| Content rules | Original; no real brands, logos or branded products; no identifiable real people; English |
| Suggested SDK | UE 5.8 + Meta XR plugin v207+. We are on 5.9 and use the engine's **OpenXR** plugin instead (see section 6). |
| Eligibility | The submitter must be a Meta VR Start member with Developer Access on their Meta account |

Side awards worth aiming at: **Best First Five Minutes** (Marlowe's heckling doubles as the tutorial) and **Boldest Original Concept**.

---

## 2. Plan

1. **Done:** PC VR through OpenXR with Touch controllers (Air Link / Link / XR Simulator), head-gaze interaction, world-space UI panel.
2. **Next:** hands-first controls with OpenXR hand tracking (pinch to interact / click, pinch-and-point teleport or stick-free locomotion, palm-up menu). Required to pass the competition's first stage.
3. **Next:** Quest standalone build: Android OpenXR, mobile forward renderer, multiview, Vulkan, package name `com.ohmcodes.cleancutaction`, release-signed 64-bit APK, hand tracking permission in the manifest.
4. **Next:** comfort and session length: Phase 1 trimmed to about 10 minutes, pause when the headset comes off, optional vignette.
5. **Optional:** compile the Meta XR plugin for 5.9 for Meta-only features (Interaction SDK, passthrough / MRUK).

---

## 3. What changed (file by file)

### New files

| File | What it is |
|---|---|
| `Source/CleanCutAction/VR/LTTVRComponent.h/.cpp` | `ULTTVRComponent` (the whole VR mode) and `ULTTVRUIHost` (overlay widget the UI moves into). |
| `Scripts/Python/create_vr_input.py` | Creates the VR input assets below. Safe to re-run; rebuilds `IMC_VR`'s mappings each time. |
| `Content/_CleanCutAction/Input/VR/IA_VR_Turn` | Axis1D. Right stick X, snap turn. |
| `Content/_CleanCutAction/Input/VR/IA_VR_UIClick` | Bool, *Trigger When Paused*. Right trigger, clicks the UI panel. |
| `Content/_CleanCutAction/Input/VR/IA_VR_Pause` | Bool, *Trigger When Paused*. Left menu button, pause menu. |
| `Content/_CleanCutAction/Input/VR/IMC_VR` | Touch controller mappings (table in section 5). Description "VR Touch Controllers". |
| `Docs/10_VR_Meta_Quest.md` | This file. |

### Changed files

| File | Change |
|---|---|
| `CleanCutAction.uproject` | Enabled `OpenXR`, `OpenXRHandTracking`, and (editor only) `PythonScriptPlugin`, `EditorScriptingUtilities`, `AllToolsets` (tools for the `unreal-mcp` server). Added `Windows` to `TargetPlatforms`. |
| `Source/CleanCutAction/CleanCutAction.Build.cs` | Added the `HeadMountedDisplay` module (motion controllers, XR tracking system). |
| `Source/CleanCutAction/CleanCutActionCharacter.h/.cpp` | Creates the `VR` component (`ULTTVRComponent`). `DoMove` moves relative to the head's yaw while VR is active. |
| `Source/CleanCutAction/CleanCutActionPlayerController.cpp` | Binds `IA_VR_Pause` to the pause toggle. In VR the pause menu keeps *Game and UI* input (instead of UI only) so the controller laser can click it. |
| `Source/CleanCutAction/Shoot/LTTShootEndScreen.cpp` | Overwrites the baked `CopyrightText` with the Ohmcodes line at runtime (the `WBP_ShootEndScreen` asset still has the old text baked in). |
| `Config/DefaultInput.ini` | New `[/Script/EnhancedInput.EnhancedInputDeveloperSettings]`: `bEnableDefaultMappingContexts=True`, `IMC_VR` as a default mapping context. |
| `Config/DefaultGame.ini` | `bStartInVR=True`; `MapsToCook` entries for `IMC_VR` and the three `IA_VR_*` actions; company / copyright set to Ohmcodes (old company URLs and contact removed). |
| `Config/DefaultEngine.ini` | Android `PackageName=com.ohmcodes.cleancutaction`. |
| `COPYRIGHT.md`, `LICENSE`, `README.md`, `Docs/Itch_Page.md`, `UI/LTTInputDevice.*`, `Shoot/LTTShootEndScreen.cpp` | Owner changed to **Ohmcodes**. |
| `CLAUDE.md`, `Docs/00_README.md`, `Docs/01_Master_Checklist.md`, `Docs/02_Architecture_and_Systems.md`, `Docs/06_UI_and_Tweening.md`, `Docs/PackagingGuide/README.md`, `README.md` | Rewritten or extended for the Quest VR branch. |

---

## 4. How the VR mode works

`ULTTVRComponent` sits on `ACleanCutActionCharacter` (so on `BP_FirstPersonCharacter` too). Every tick it checks whether the headset is rendering (`GEngine->XRSystem->IsHeadTrackingAllowed()`) for the locally controlled pawn. On the change it calls `EnterVR()` or `ExitVR()`. Flat play never enters VR, so nothing changes there.

**`EnterVR()`**
- Tracking origin `Local` and a recentre (`ResetOrientationAndPosition(0)`): the start pose of the head becomes the origin, facing the pawn's forward.
- Spawns `VROrigin` on the capsule at `BaseEyeHeight` (64 cm above the capsule centre, about 160 cm above the floor). A seated player sees the set from standing height.
- Moves the camera from the arms' animated `head` bone onto `VROrigin`; turns off `bUsePawnControlRotation` and the first-person FOV / scale; `bLockToHmd` on. The headset now drives the camera.
- Hides the first-person arms mesh.
- Levels the control rotation (pitch 0). Only yaw matters in VR; snap turn changes it, and the pawn follows it (`bUseControllerRotationYaw`).
- Spawns motion controllers: `Left` and `Right` grip poses (each with a 6 cm sphere marker) and `RightAim` (aim pose, no marker). All tick while paused so the pause menu can still be pointed at.
- Spawns `VR_Laser` (stretched engine cylinder) and `VR_Pointer` (`UWidgetInteractionComponent`, custom hit source) on `RightAim`.
- Spawns `VR_UIPanel`, a world-space `UWidgetComponent` (1920 x 1080 draw size, 150 cm wide, transparent, no collision) hosting a `ULTTVRUIHost` overlay. It renders the captured widgets into a render target and gives the laser a plane to hit.
- Spawns `VR_UILayer`, a world-locked quad stereo layer (`UStereoLayerComponent`, `SLT_WorldLocked` set through reflection, live texture, no depth) showing the same render target. Console variable **`ltt.VR.UILayer`** picks which one is visible: **0 (default)** draws the widget panel in the scene; 1 hides it from the scene and shows the stereo layer, which the XR runtime composites on top of everything (no clipping into walls, not covered by fades). Layer mode isn't verified in the XR Simulator yet, so it's off by default.

**Intro in VR.** The flat game holds the screen black (`ALTTMessDirector::BlackOut()`, a manual camera fade) until the player presses Start on the briefing; on a monitor the briefing draws over the black. In VR the panel is in the scene, so that fade hid it and the whole view was black. Now `FinishIntro()` fades the set in behind the briefing when `ULTTVRComponent::IsHeadsetActive()`; the clock and movement still wait for Start, and the second fade is skipped if the set is already visible. The set is still dark at the start by design ("Power's off! Turn it on at the electric box").

**UI panel.** Screen-space UMG isn't drawn in a headset. Every 0.1 s the component finds the top-level `UUserWidget`s that are in the viewport (HUD, interact prompt, briefing, barks, pause menu, end screens, leaderboard, virtual keyboard), removes them from the viewport and inserts them into the overlay, sorted by their viewport Z order (read from `UGameViewportSubsystem`). The game still controls them as usual: a later `RemoveFromParent()` takes a widget off the panel. The panel floats 140 cm in front of the eyes, 10 cm low, and follows the head lazily (stays put until the head turns more than 30 degrees away). It hides when nothing is captured. `ExitVR()` puts every widget back in the viewport at its old Z order.

**Pointer.** Each tick the right aim ray is intersected with the panel plane in code (the panel has no collision, so it never blocks gameplay traces) and fed to the widget interaction component with `SetCustomHitResult`. If the right controller isn't tracked or misses the panel, the **head's gaze** is used instead (keyboard-and-mouse testing). The laser only shows while the controller is over something clickable. `IA_VR_UIClick` (right trigger, **left mouse button**, **Enter**) presses / releases a virtual left mouse button. When a new widget is captured, the player controller is switched to *Game and UI* input (menus set UI-only, which would swallow the laser / gaze click).

**Movement and turning.** The left stick goes through the existing `IA_Move` binding; `DoMove` uses the camera's yaw while VR is active, so "forward" is where the head faces. The right stick snap-turns 45 degrees (fires past 0.6 tilt, re-arms below 0.3); disabled while paused.

**Interaction.** Unchanged code. `ULTTInteractionComponent`, `ULTTWaterPuddle` and `ALTTEffectStation` aim with `GetPlayerViewPoint`, which is the headset's view in VR: the player aims by looking. `ULTTCarryComponent` holds props relative to the camera, so a carried prop floats in front of the face.

Tunables on the component (`Details > VR`): `SnapTurnDegrees` (45), `PanelDistance` (140), `PanelWidth` (150), `PanelFollowAngle` (30).

---

## 5. Controls (Touch controllers)

| Control | Action | Asset |
|---|---|---|
| Left stick | Move (head-relative), 0.2 dead zone | `IA_Move` |
| Right stick left/right | Snap turn 45 degrees | `IA_VR_Turn` |
| Right trigger | Interact / pick up / place, and click the UI panel | `IA_Interact` + `IA_VR_UIClick` |
| Left mouse button / Enter (keyboard testing) | Click the UI button you're looking at | `IA_VR_UIClick` |
| Left trigger | Interact | `IA_Interact` |
| A | Jump | `IA_Jump` |
| B | Throw | `IA_Throw` |
| Y | Emote | `IA_Emote` |
| Left menu button | Pause | `IA_VR_Pause` |

Crouch has no button in VR; crouching for real lowers the view (the capsule stays tall).

**Why the bindings are assets.** The OpenXR input plugin builds the runtime's action bindings only from the Enhanced Input **default mapping contexts** (`Config/DefaultInput.ini`), and it injects input straight into those actions. Runtime-created actions and contexts (the project makes some for crouch, pause and emote) are invisible to OpenXR. Any new VR control needs an `IA_` asset mapped in `IMC_VR` (re-run or extend `create_vr_input.py`) and a `MapsToCook` entry if it's loaded by path.

---

## 6. Engine, plugins and SDK

- Engine: UE 5.9.0 Preview at `D:\UE5 Engines\UE_5.9` (registered as `5.9`). Build: `D:\UE5 Engines\UE_5.9\Engine\Build\BatchFiles\Build.bat CleanCutActionEditor Win64 Development -Project="D:\UE5 Projects\CCA_VR\CleanCutAction.uproject" -WaitMutex` (editor closed, or Live Coding off).
- XR stack: the engine's **OpenXR** plugin (+ `OpenXRHandTracking` for the hands-first work). It runs on PC (Link / Air Link / XR Simulator) and on Quest (Android).
- **Meta XR plugin:** Meta suggests UE 5.8 + v207. There's no 5.9 build; the plugin ships as source, so compiling it against 5.9 is possible but needs API fixes. Not needed for controllers or hand tracking; only for Meta-only features (Interaction SDK components, passthrough, MRUK). Decide after hands-first works.
- Only code depends on the `HeadMountedDisplay` engine module (no `XRBase` dependency), so the module still builds for every platform.

---

## 7. Testing without a headset: Meta XR Simulator

Meta XR Simulator is a standalone OpenXR runtime for Windows that simulates a Quest 3 / 3S / 2 / Pro (or Meta VR Glasses). It works with the engine's OpenXR plugin, so the Meta XR plugin isn't required.

1. Download **Meta XR Simulator (Windows)** v207.1+ from `developers.meta.com/horizon/downloads/package/meta-xr-simulator-windows/` and install it.
2. Open the simulator and turn on the **XR Simulator runtime toggle** in its header. This makes it the active OpenXR runtime (it shows under `HKLM\SOFTWARE\Khronos\OpenXR\1\ActiveRuntime`). Turn it off again before using Link with a real headset.
3. Open the project and use **Play > VR Preview**. (If VR Preview is greyed out, the editor didn't find an OpenXR runtime: check the toggle, then restart the editor.)
4. In the simulator: **Inputs > Device info > Device** = Meta Quest 3 (restart Play after changing it). Keyboard and mouse drive the head and the controllers **while the simulator window has focus**; the bindings are in the **Input bindings** panel. An Xbox controller also works.
   - The simulator only receives frames while the VR Preview window isn't minimized; it shows *"No frames received"* otherwise.
   - To press a menu button without a controller: aim your head at it in the simulator, then click (left mouse) or press Enter in the VR Preview window.
   - WASD / mouse in the **VR Preview** window drive the game pawn (move head-relative, mouse turns), after Start; movement is blocked while the briefing is up, as in flat play.
5. For the hands-first work: pick a profile with hand tracking and switch the inputs to hands; the simulator reports pinch and grab.
6. The simulator can record a session (VRS) for repeatable bug repros, and its footage is allowed for the competition video.

Logs: the editor log shows `VR: headset active, switching ... to VR mode` when the pawn enters VR.

**With a headset (Link / Air Link):** install the Meta Horizon Link PC app, set it as the active OpenXR runtime (Settings > General), connect, then VR Preview.

---

## 8. Known limits and follow-ups

- **Not tested in a headset or the simulator yet.** The editor build compiles and the input assets were checked by script.
- **No hands-first input yet.** This blocks the competition's pass/fail stage.
- In the default panel mode the UI is scene geometry: full-black fades (ending, tour cuts) and nearby walls can still hide or clip it. `ltt.VR.UILayer 1` avoids that once it's verified in the simulator / on Quest.
- Widgets that call `AddToViewport` again while already on the panel log a "has a parent" warning and stay where they are.
- The panel is 16:9 and HUD corners sit at the edge of view; the HUD may need a VR layout.
- Jump in VR can be uncomfortable; consider removing it.
- Room-scale drift: leaning or stepping moves the camera away from the capsule. Fine for seated play.
- `AllToolsets` is enabled for the editor; it failed the cook once before (game feature plugins). Cook-test before the Quest package and disable it for that build if needed.
- `WBP_ShootEndScreen` still has the old copyright text baked into the asset (fixed at runtime). Re-bake or edit it in the editor when convenient.
