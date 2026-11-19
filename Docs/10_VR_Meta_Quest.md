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
2. **Working on a Quest over Link, layout final (2026-11-17):** hands-first controls with OpenXR hand tracking: each finger pinched to the thumb is a button (left: walk, turn left, jump, hint; right: interact, turn right, throw, dance), a forward flick of the right hand throws for real, scrubbing with the mop mops, left palm up = wrist menu, right palm held up = pause menu. VR Template hand meshes for hands and controllers. Sections 5b and 5c. Required to pass the competition's first stage.
3. **Configured 2026-11-17, waiting on the Android SDK install:** Quest standalone build: Android OpenXR, mobile forward renderer, multiview, Vulkan only, 4x MSAA, fixed foveation, Meta release manifest, package name `com.ohmcodes.cleancutaction`, `Scripts\Package_Quest.bat`. Section 9.
4. **Next:** comfort and session length: Phase 1 trimmed to about 10 minutes, pause when the headset comes off, optional vignette.
5. **Optional:** compile the Meta XR plugin for 5.9 for Meta-only features (Interaction SDK, passthrough / MRUK).

---

## 3. What changed (file by file)

### New files

| File | What it is |
|---|---|
| `Source/CleanCutAction/VR/LTTVRComponent.h/.cpp` | `ULTTVRComponent` (the whole VR mode: camera, controllers, hand tracking, snap turn, UI panel and pointer, body following, VR render settings), `ULTTVRUIHost` (overlay widget the UI moves into, with the pointer dots) and `ULTTVRHandMenu` (the wrist menu for hand tracking, built in C++). |
| `Scripts/Python/create_vr_input.py` | Creates / updates the VR input assets below. Safe to re-run; rebuilds `IMC_VR`'s mappings each time. |
| `Scripts/RunVR_Standalone.bat` | Runs the game outside the editor in VR (`-game -vr`); `build` argument builds first, `link` runs on the Quest through Meta Horizon Link / Air Link, `sim` runs in the XR Simulator (both set `XR_RUNTIME_JSON` for that run only). Much faster than VR Preview. |
| `Content/_CleanCutAction/Input/VR/IA_VR_Turn` | Axis1D. Right stick X, snap turn. |
| `Content/_CleanCutAction/Input/VR/IA_VR_UIClick` | Bool, *Trigger When Paused*. Right trigger (also left mouse / Enter for testing), clicks the UI panel. |
| `Content/_CleanCutAction/Input/VR/IA_VR_Pause` | Bool, *Trigger When Paused*. Left menu button, pause menu. |
| `Content/_CleanCutAction/Input/VR/IA_VR_Hint` | Bool. X (left controller), hint. |
| `Content/_CleanCutAction/Input/VR/IMC_VR` | Touch controller mappings (table in section 5). Description "VR Touch Controllers". |
| `Content/_CleanCutAction/Materials/VR/M_VRWidgetBase_OnTop` | Copy of the engine's `Widget3DPassThrough` with **Disable Depth Test** on. |
| `Content/_CleanCutAction/Materials/VR/M_VRWidget_OnTop` | Instance of it used by the VR UI panel, so walls never cut through the HUD. |
| `Scripts/Package_Quest.bat` | Packages the Quest APK (Shipping; `dev` for Development; `install` also installs it on the connected headset) into `Builds\Quest\Android_ASTC\`. |
| `Docs/10_VR_Meta_Quest.md` | This file. |

### Changed files

| File | Change |
|---|---|
| `CleanCutAction.uproject` | Enabled `OpenXR`, `OpenXRHandTracking`, and (editor only) `PythonScriptPlugin`, `EditorScriptingUtilities`, `AllToolsets` (tools for the `unreal-mcp` server). Added `Windows` to `TargetPlatforms`. |
| `Source/CleanCutAction/CleanCutAction.Build.cs` | Added the `HeadMountedDisplay` module (motion controllers, XR tracking system, spectator screen). |
| `Source/CleanCutAction/CleanCutActionCharacter.h/.cpp` | Creates the `VR` component (`ULTTVRComponent`). `DoMove` moves relative to the head's yaw while VR is active. |
| `Source/CleanCutAction/CleanCutActionPlayerController.cpp` | Binds `IA_VR_Pause` to the pause toggle. In VR the pause menu keeps *Game and UI* input so the laser can click it. The interact prompt shows **Trigger** / **B** in VR (instead of E / mouse or gamepad labels), **Pinch** / **Palm menu** with hand tracking. |
| `Source/CleanCutAction/Interaction/LTTMessDirector.cpp` | VR: fades the set in behind the briefing instead of holding black (the clock still waits for Start); binds `IA_VR_Hint`. |
| `Source/CleanCutAction/Interaction/LTTCleanupHUD.cpp` | VR controls legend (Touch labels, or `ControlsListHands` with the gestures while hands are tracked) shown instead of the keyboard / gamepad lists; "[H] Hide controls" footer hidden in VR; hint label "[X] Hint" in VR, "[Palm menu] Hint" with hands. |
| `Source/CleanCutAction/Interaction/LTTFirstPersonAnimInstance.cpp` | VR: free hands' IK follows the controllers' grip poses (carry IK keeps priority). |
| `Source/CleanCutAction/Shoot/LTTShootEndScreen.cpp` | Overwrites the baked `CopyrightText` with the Ohmcodes line at runtime (the `WBP_ShootEndScreen` asset still has the old text baked in). |
| `Source/CleanCutAction/Interaction/LTTCleanupBarker.cpp` | Comment wording only (no "jam"). |
| `Source/CleanCutAction/Interaction/LTTCarryComponent.h/.cpp` | `ThrowWithVelocity(Velocity)`: throws the held prop with a given velocity (the VR hand throw). `Release` takes an optional velocity; `Throw()` is unchanged. |
| `Source/CleanCutAction/Interaction/LTTInteractionComponent.h/.cpp` | `TryMopStroke()`: one mop stroke if the held mop is aimed at a puddle right now, otherwise nothing (never drops the mop). Used by VR scrubbing. |
| `Source/CleanCutAction/Interaction/LTTCleanupBarker.cpp` | Marlowe's first-throw line names the VR control: "A flick of the hand" with hands, "B" with controllers. |
| `Config/DefaultInput.ini` | New `[/Script/EnhancedInput.EnhancedInputDeveloperSettings]`: `bEnableDefaultMappingContexts=True`, `IMC_VR` as a default mapping context. |
| `Config/DefaultGame.ini` | `bStartInVR=True`; `MapsToCook` entries for `IMC_VR`, the four `IA_VR_*` actions and both `M_VRWidget*` materials; company / copyright set to Ohmcodes (old company URLs and contact removed). |
| `Config/DefaultEngine.ini` | Android `PackageName=com.ohmcodes.cleancutaction`. Quest hand tracking manifest entries: `com.oculus.permission.HAND_TRACKING`, optional feature `oculus.software.handtracking`, `com.oculus.handtracking.frequency=HIGH`, `com.oculus.handtracking.version=V2.0`. |
| `COPYRIGHT.md`, `LICENSE`, `README.md`, `Docs/Itch_Page.md`, `itch/ITCH_THEME_SETTINGS.md`, `UI/LTTInputDevice.*`, `Shoot/LTTShootEndScreen.cpp` | Owner changed to **Ohmcodes**; the competition is credited instead of the earlier jam. |
| `CLAUDE.md`, `Docs/00_README.md`, `Docs/01_Master_Checklist.md`, `Docs/02_Architecture_and_Systems.md`, `Docs/06_UI_and_Tweening.md`, `Docs/07_Audio_List.md`, `Docs/CleanCutAction_Design.md`, `Docs/PackagingGuide/README.md`, `Docs/PackagingGuide/03_Android_Changes.md`, `README.md` | Rewritten or extended for the Quest VR branch and the Meta VR Start Developer Competition 2026; CLAUDE.md documents the rebuild routine. |

Editor-only, not in the repo: **Use Less CPU when in Background** turned off in Editor Preferences (per-user setting) so the XR Simulator keeps getting frames.

---

## 4. How the VR mode works

`ULTTVRComponent` sits on `ACleanCutActionCharacter` (so on `BP_FirstPersonCharacter` too). Every tick it checks whether the headset is rendering (`GEngine->XRSystem->IsHeadTrackingAllowed()`) for the locally controlled pawn. On the change it calls `EnterVR()` or `ExitVR()`. Flat play never enters VR, so nothing changes there.

**`EnterVR()`**
- Tracking origin `Local` and a recentre (`ResetOrientationAndPosition(0)`): the start pose of the head becomes the origin, facing the pawn's forward.
- Spawns `VROrigin` on the capsule at `BaseEyeHeight` (64 cm above the capsule centre, about 160 cm above the floor). A seated player sees the set from standing height.
- Moves the camera from the arms' animated `head` bone onto `VROrigin`; turns off `bUsePawnControlRotation` and the first-person FOV / scale; `bLockToHmd` on. The headset now drives the camera.
- Hides the first-person mesh (see *Body follows the headset*).
- Sets the world body (`CharacterMesh0`) to `AlwaysTickPoseAndRefreshBones`. Characters default to `AlwaysTickPose`, which refreshes bones only when the mesh is drawn in a view; the body is owner-no-see, so mirrors (scene captures) showed it frozen and the dance didn't show. Verified: idle and dance animate in a mirror-style capture.
- Applies a PC VR performance set; `ExitVR()` / `EndPlay` restore every value:
  - Lumen GI and reflections off (`r.DynamicGlobalIlluminationMethod 0`, `r.ReflectionMethod 0`, plus `r.Lumen.DiffuseIndirect.Allow 0` and `r.Lumen.Reflections.Allow 0` so a post-process volume can't turn it back on).
  - Virtual shadow maps off, replaced by 2 cascaded shadow maps at 1024 px with a 0.6 distance scale. Distance-field shadows and AO off.
  - Volumetric fog, SSR, SSAO, motion blur, depth of field, lens flares and chromatic fringe off.
  - `r.ViewDistanceScale 0.6`.
  - `ltt.VR.ScreenPercentage` (default 100) sets `r.ScreenPercentage` only when it isn't 100. At 80 the engine recreated the headset swapchain smaller ("Resizing VR buffer"), and the Quest over Link showed nothing while input still worked, so it's left at the headset's own resolution.
- Logs `VR: runtime hand tracking available / NOT available` when VR starts. Over Link, hands need Developer Runtime Features on in the Meta Horizon Link app (Settings > Beta).
- `vr.InstancedStereo=True` in `Config/DefaultEngine.ini`: both eyes render in one pass. Takes effect after an editor restart and a full shader recompile.
- Sets the desktop mirror to the full eye, letterboxed (`ESpectatorScreenMode::SingleEyeLetterboxed`).
- Levels the control rotation (pitch 0). Only yaw matters in VR; snap turn changes it, and the pawn follows it (`bUseControllerRotationYaw`).
- Spawns motion controllers: `Left` and `Right` grip poses and `RightAim` (aim pose). No visible markers (they blocked the view in the XR Simulator); tracked hands are drawn as joint spheres (section 5b). With hand tracking, `Left` / `Right` follow the palms (the `OpenXRHandTracking` legacy motion sources). All tick while paused so the pause menu can still be pointed at.
- Spawns `VR_Laser` (stretched engine cylinder) and `VR_Pointer` (`UWidgetInteractionComponent`, custom hit source) on `RightAim`.
- Spawns `VR_UIPanel`, a world-space `UWidgetComponent` (1920 x 1080 draw size, 150 cm wide, transparent, no collision) hosting a `ULTTVRUIHost` overlay. It renders the captured widgets into a render target and gives the laser a plane to hit.
- Spawns `VR_UILayer`, a world-locked quad stereo layer (`UStereoLayerComponent`, `SLT_WorldLocked` set through reflection, live texture, no depth) showing the same render target. Console variable **`ltt.VR.UILayer`** picks which one is visible: **0 (default)** draws the widget panel in the scene; 1 hides it from the scene and shows the stereo layer, which the XR runtime composites on top of everything (no clipping into walls, not covered by fades). Layer mode isn't verified in the XR Simulator yet, so it's off by default.

**Intro in VR.** The flat game holds the screen black (`ALTTMessDirector::BlackOut()`, a manual camera fade) until the player presses Start on the briefing; on a monitor the briefing draws over the black. In VR the panel is in the scene, so that fade hid it and the whole view was black. Now `FinishIntro()` fades the set in behind the briefing when `ULTTVRComponent::IsHeadsetActive()`; the clock and movement still wait for Start, and the second fade is skipped if the set is already visible. The set is still dark at the start by design ("Power's off! Turn it on at the electric box").

**UI panel.** Screen-space UMG isn't drawn in a headset. Every 0.1 s the component finds the top-level `UUserWidget`s that are in the viewport (HUD, interact prompt, briefing, barks, pause menu, end screens, leaderboard, virtual keyboard), removes them from the viewport and inserts them into the overlay, sorted by their viewport Z order (read from `UGameViewportSubsystem`). The game still controls them as usual: a later `RemoveFromParent()` takes a widget off the panel. The panel sits 140 cm in front of the eyes and is sized from the headset's reported horizontal field of view (**`ltt.VR.UIScreenFill`**, default 0.85 of it; about 2.7 m wide on a Quest profile), so the HUD sits where a screen HUD would. In VR the flat game's `Crosshair` (in `WBP_InteractPrompt`) is collapsed and `ULTTCleanupHUD::bShowPickupRing` is turned off (the ring is placed with the flat screen's projection). The desktop window mirrors the full eye letterboxed (`ESpectatorScreenMode::SingleEyeLetterboxed`) instead of the default crop, so it matches the headset view. **`ltt.VR.UIHeadLocked`** picks how it moves: **1 (default)** = head-locked, glued to the view like a screen (level, no roll); 0 = lazy follow (stays in the world and glides back in front once the head turns more than 30 degrees away, pitch clamped to 40 degrees, 10 cm below eye level). With head-locked UI the gaze always lands mid-panel, so menu buttons are clicked with the controller ray. It hides when nothing is captured. `ExitVR()` puts every widget back in the viewport at its old Z order.

**Pointer.** Each tick the right aim ray is intersected with the panel plane in code (the panel has no collision, so it never blocks gameplay traces) and fed to the widget interaction component with `SetCustomHitResult`. If the right controller isn't tracked or misses the panel, the **head's gaze** is used instead (keyboard-and-mouse testing). Two dots drawn on the panel itself, **only while a menu that takes clicks is open** (or the pointer is over a button): yellow `VR_DotPointer` where the pointer lands (bigger over a button), cyan `VR_DotGaze` where the head looks. The laser follows the same rule, so nothing crosshair-like shows during play. **Click fallback:** if a click arrives while the controller ray isn't on a button, the pointer is re-aimed along the gaze (`SimulatePointerMovement` through an access shim) and clicks there if that's a button, holding the gaze until release (needed in the XR Simulator, whose hands sit off to the side). Each click logs `VR: UI click (over panel, over button, aimed by controller|gaze)`. `IA_VR_UIClick` (right trigger, **left mouse button**, **Enter**) presses / releases a virtual left mouse button. When a new widget is captured, the player controller is switched to *Game and UI* input (menus set UI-only, which would swallow the laser / gaze click), with keyboard focus on the top focusable captured menu so the menus' own shortcuts keep working (briefing: Enter = time trial, N = no timer, Left / Right = pages). Verified in VR Preview: N closes the briefing; the UI-click action on the pause menu's Resume button closes it.

**Body follows the headset** (`LTTUpdateVRBody` in `LTTVRComponent.cpp`, every tick in VR):
- *Capsule:* sweeps horizontally to wherever the head moved (walking in the room, the XR Simulator's WASD), stopping at walls; the VR origin shifts back by the same amount so the view stays at the capsule's centre. The head can't leave the body or pass through walls.
- *Meshes:* both Rookie meshes turn to the head's yaw and slide so their `head` bone sits 8 cm behind and 6 cm under the eyes: the first-person mesh (owner-only: arms and hands) exactly, the world body (`CharacterMesh0`, seen in mirrors / by others) horizontally only, feet on the floor.
- *Hands:* the first-person mesh is **hidden in the headset**. Shown, the full body seen from just behind the eyes (torso, arms reaching to the controllers) filled the view in the XR Simulator; its neck/head bones are hidden as well. `ULTTFirstPersonAnimInstance` still puts each free hand's IK on its controller's grip pose (wrist 8 cm behind the grip, elbow hint below and outside the shoulder; carry IK keeps priority), ready for a hands-only look. Visible hands are planned with OpenXR hand-tracking meshes (hands-first work). The world body has no hand IK, so in a mirror its arms play the normal animation.
- Verified in VR Preview: both meshes face the head's yaw, the first-person head bone sits 8 cm behind / 6 cm under the eyes, and a view from in front of the player (what a mirror captures) shows the Rookie standing at the player's position, facing where the head looks.

**Controls legend and prompts.** In VR `ULTTCleanupHUD` builds a third legend list, `ControlsListVR`, with the Touch bindings (L Stick move, R Stick turn, Trigger pick up / place, B throw, A jump, Duck = crouch for real, X hint, Y dance, Menu pause) and shows it instead of the keyboard / gamepad lists; the "[H] Hide controls" footer is hidden (no VR button for it). The score box reads "[X] Hint". The interact prompt (`WBP_InteractPrompt`, labelled in `ACleanCutActionPlayerController::ApplyInputDevice`) reads **Trigger** for pick up / place and **B** for throw. Verified in VR Preview: legend and labels switch, and X triggers the hint (its cooldown starts).

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
| X | **Info menu** above the left controller (score, hint timer, controls, buttons: Hint, Pause, Jump, Throw, Turn, Dance, Recenter). Point with the right controller, trigger to press. X again closes it; a pause or end menu closes it too | `IA_VR_Hint` -> `ALTTMessDirector::HandleVRHintInput` -> `ULTTVRComponent::ToggleControllerMenu` |
| Y | Dance (emote; in the XR Simulator Y is the **N** key, which also presses B = throw while *Use same input for both sides* is on) | `IA_Emote` |
| Left menu button | Pause | `IA_VR_Pause` |

Crouch has no button in VR; crouching for real lowers the view (the capsule stays tall).

**In-headset UI (2026-11-17):** the HUD's top-left score card and the controls card are hidden in VR, with controllers as well as hands: both live on the wrist / info menu (left palm with hands, X with controllers), which the player brings into view instead of reading the lens edge. The head-locked UI panel (pause menu, briefing, end screen, timer) now fills **62 %** of the headset's field of view instead of 85 % (`ltt.VR.UIScreenFill`), so its corners sit in the clear part of the lens. The trigger doesn't pick anything up while the laser is on a menu button (`ULTTVRComponent::IsPointerOnUI`, checked in `ULTTInteractionComponent::Interact`).

## 5b. Controls (hand tracking, no controller)

Hands-first is pass/fail, so every controller action has a hand equivalent. Controllers and hands both work; the game switches on its own (hands mode while a hand is tracked and for 1.5 s after it drops out).

Each finger pinched against the thumb is its own button. Layout set by the developer on 2026-11-17:

| Gesture | Action | How |
|---|---|---|
| **Left index pinch, held** | Walk where you look, at a walking pace | Injects `IA_Move` (0, `PinchWalkSpeed` = 0.45) every frame; the character moves head-relative |
| **Left middle pinch** | Snap turn left | `SnapTurn(-1)` |
| **Left ring pinch (hold)** | Walk backward, same pace as the index walk (throw is the right hand flick, or Throw in the wrist menu) | `UpdateHands` injects `IA_Move` with a negative Y |
| **Left pinky pinch** | Hint | Injects `IA_VR_Hint` |
| **Right index pinch** | Same as the right trigger: pick up / place what the **right hand points at** (laser), mop once when holding the mop over a puddle, and click the menu button under the hand ray (falls back to the gaze like the trigger) | Injects `IA_Interact` (skipped when the ray is on a button or a menu is open) + virtual left click |
| **Right middle pinch** | Snap turn right | `SnapTurn(1)` |
| **Right ring pinch** | Jump (on the right hand so the left index can keep walking: walk + jump) | Injects `IA_Jump` |
| **Right pinky pinch** | Dance | Injects `IA_Emote` on the pinch. Holding it longer does nothing in the game (left to the headset) |
| **Flick the right hand forward** while holding a prop | **Real throw**: the prop leaves with the hand's direction and speed | `UpdateHandMotion` (below) calls `ULTTCarryComponent::ThrowWithVelocity` |
| **Scrub the right hand back and forth** while holding the mop and looking at a puddle | **Real mopping**: one stroke per 14 cm of hand travel | `UpdateHandMotion` calls `ULTTInteractionComponent::TryMopStroke` |
| **Left palm toward your face** | Opens the **wrist menu** above the left hand: the score (placed / destroyed / points / hint cooldown), the full gesture list, and buttons for Hint, Pause, Jump, Throw, < Turn, Turn >, Dance, Recenter. Point with the right hand and index-pinch a button | `ULTTVRHandMenu` buttons inject `IA_VR_Hint`, `IA_VR_Pause`, `IA_Jump`, `IA_Throw`, `IA_Emote`; turns call the snap turn; Recenter resets the tracking origin |
| **Right palm toward your face** | Shows the **palm menu** above the right hand, styled like the wrist menu, gone when the hand turns away: the score counter (placed / destroyed / points / time left / hint) and buttons Pause (full pause menu), Scores (leaderboard), Music on/off, Unstuck, Restart, Quit. **Point at it with the left hand (a laser comes from the left hand) and pinch the left index.** Restart and Quit ask "Sure?" and need a second press within 3 s | `ULTTVRPalmMenu`, `UpdatePalmMenu` |
| **Any menu: point and index-pinch** (one method everywhere, like the Quest's own menus) | Big panels (briefing, pause, end screen): either hand; the ray and laser follow the hand that pinched last. Left wrist menu: the right hand. Right palm menu: the left hand. During play the left index pinch is still walking | `GetAimRay` (left ray while the palm menu is up or the left hand pinched last in a menu), `HandlePinch` (left index = click in menus). Poke is off: `ltt.VR.Poke 1` turns it back on |
| **Point at a prop far away and index-pinch** | **Gravity pull**: the prop flies across the room into the hand (up to 12 m, 0.25 - 0.9 s flight) | `ULTTInteractionComponent` (`bGravityPull`, `PullDistance`, `PullSpeed`) + `ULTTCarryComponent::PullFromAfar` |
| **Duck for real** | Crouch | Head tracking |

While a hand is tracked the cleanup HUD hides its top-left score card and the controls card (both are on the wrist menu); the timer stays at the top centre.

**Aiming with the hand (2026-11-18).** In VR, picking up, placing, pulling and mopping aim along the **right hand's ray** (tracked hand: shoulder through the knuckles; controllers: the right aim pose), not the head. A thin laser shows the ray during play, out to whatever it hits. Every interaction trace goes through `ULTTVRComponent::GetAimViewPoint` (used by `ULTTInteractionComponent` and `ALTTWaterPuddle`), which falls back to the head view in flat play or with no right hand / controller tracked. `ltt.VR.HandAim 0` switches back to head gaze.

**Walking through chairs.** In VR the player capsule passes through chairs and stools (meshes named `*Chair*`, `*Bar_Chai*`, `*Stool*`): they ignore the `Pawn` channel. Props, physics and traces still hit them. Re-applied every second because picking up and placing a chair resets its collision. `ltt.VR.WalkThroughChairs 0` turns it off.

**Not in the mirror.** The hands, controller models, laser and hand menus are `SetHiddenInSceneCapture(true)`, so the mirror reflects the set and the body, not floating hands.

**Controller models.** With controllers, each hand holds the Quest 2 Touch controller model (`/OpenXR/Devices/OculusTouch_v2/...`, the OpenXR plugin's own device meshes, authored in grip space and attached to the grip pose with no offset). They're in `MapsToCook` because the plugin content is only cooked when referenced.

**How it works** (`ULTTVRComponent::UpdateHands`, every tick in VR, paused too):
- Joint data comes from `IXRTrackingSystem::GetHandTrackingState` (OpenXR `XR_EXT_hand_tracking` through the `OpenXRHandTracking` plugin): 26 joints per hand in world space, with radii. No Meta XR plugin needed.
- **Pinch** (tuned from `ltt.VR.PinchDebug` logs on a Quest 2, 2026-11-18): a real pinch brings that finger's thumb gap to 0.7 cm or less (often below 0); a relaxed or half-curled hand rests at 1.5 - 2.5 cm on several fingers at once, which with the earlier 2 cm threshold walked by itself, and a 3.5 cm release kept a pinch "held" for seconds so the next one never fired. Now: pressed when one finger's gap is below `PinchDistance` (1.2 cm) **and** at least 0.5 cm smaller than any other finger's, two frames in a row; released above `PinchDistance + PinchReleaseExtra` (2.0 cm); pinching another finger while one is held switches straight to it. `ltt.VR.PinchDistance` overrides the threshold live; `ltt.VR.PinchDebug 1` logs every gap 5 times a second. Lost tracking releases.
- **Hand ray:** like the Quest system pointer, from an estimated right shoulder (18 cm right of and 20 cm below the head) through the midpoint of the index and thumb knuckles, smoothed. The knuckles barely move while pinching, so a click doesn't knock the ray off a button. The hand ray takes priority over the controller aim pose; `GetAimRay` gives the controller ray when no hand is tracked. The pointer traces the wrist menu and the UI panel and uses the nearer hit; the laser shows on either.
- **Wrist menu:** a `UWidgetComponent` (`ULTTVRHandMenu::DrawSize` 1040 x 560, `HandMenuWidth` 36 cm wide, `M_VRWidget_OnTop`, sort priority above the HUD panel) placed 6 cm above the left palm (bottom edge), facing the eyes. Left column: score (read from `ALTTMessDirector`; hidden in Phase 2) and the gesture list. Right column: a 2 x 4 button grid. Opens when the palm normal points at the head (dot > 0.6), closes below 0.3, stays open while the right ray is on it. Hidden while a menu (briefing, pause, end screen) is open or the game is paused. Its buttons aren't focusable, so they never take focus from the main menus. Left-pinch walking is off while it's open.
- **Visible hands (VR Template meshes, `/Game/XRMannequins`):**
  - *Tracked hands:* `UPoseableMeshComponent`s with `SKM_MannyXR_left/right`. `LTTBuildHandBones` maps 20 bones (hand, thumb, and the metacarpal + three segments of each finger) to OpenXR joints. For each bone it stores the offset between a frame built from the bone's direction (toward the next joint) and the back of the hand, and the bone's own reference rotation, so no bone-axis convention is assumed. Every frame the mesh is placed on the wrist joint, scaled to the player's hand (wrist to middle knuckle), and each bone's world rotation is set from the live joints (finger twist from each joint, thumb twist from the palm).
  - *Controllers:* the template's own hand component (`BP_MannequinsXR` with `ABP_MannequinsXR`, which mirrors the right-hand animations for the left via `bMirror`) on each grip pose with the template's offsets. Its pose alphas come from new `IMC_VR` actions: grip axis = grasp, trigger axis = index curl, finger off the trigger = point, thumb off A / B / X / Y / stick = thumbs up (`IA_VR_HandGrip_*`, `IA_VR_HandTrigger_*`, `IA_VR_HandIndexTouch_*`, `IA_VR_HandThumbTouch_*`).
  - Controller hands hide while hand tracking is active, tracked hands hide when their hand isn't tracked. The joint spheres only show if the meshes failed to load.
  - *Glow:* `M_VRGlow` (unlit translucent, fresnel rim, params `Color`, `Intensity`, `Base`, `Opacity`) is the overlay material on all four hand meshes, the laser and the joint spheres, so hands and pointer stay visible when the breaker kills the lights.
- **Labels:** `ULTTVRComponent::IsUsingHands()` switches the HUD legend to `ControlsListHands`, the interact prompt to **Pinch** / **Palm menu**, and the hint label to "[Palm menu] Hint".
- Gestures go through `UEnhancedInputLocalPlayerSubsystem::InjectInputForAction`, so they reach the same bindings as the keys and buttons; nothing in the gameplay code changed.
- The palm direction assumes OpenXR's joint axes (+Y out of the back of the hand = Unreal +Z, so the palm normal is the palm joint's -Z). If the menu opens with the back of the hand instead, flip the sign in `LTTPalmNormal`.

- **Right palm menu** (`UpdatePalmMenu`, `ULTTVRPalmMenu`): the same rules as the wrist menu, mirrored. Opens when the right palm normal points at the head (dot > 0.6), closes below 0.3 (stays while a fingertip presses it), 6 cm above the right palm, `PalmMenuWidth` 30 cm, `DrawSize` 900 x 440. Not while paused, over a menu, or with the wrist menu up (and the wrist menu doesn't open while the palm menu is up). The score texts share `LTTUpdateMenuStats` with the wrist menu. While the right palm faces the head (dot > 0.5) right pinches are ignored (palm toward you + index pinch is the Quest's own Meta / home gesture), and left pinches are ignored while the palm menu is up (the left hand is poking).
- **Poke** (`UpdatePoke`, off by default, `ltt.VR.Poke 1`): the fingertip's depth in front of a hand menu; hover within 7 cm, press at 0.8 cm, release at 2 cm. Turned off so every menu works one way (point + pinch).
- **Real throw** (`UpdateHandMotion`, every tick while a prop is held and play is running):
  1. The right palm joint is stored with a timestamp **in VR origin space** (`VROrigin->InverseTransformPositionNoScale`), the last 0.2 s. In origin space a snap turn or pinch-walking doesn't look like the hand moving.
  2. Hand velocity = position change over the last ~70 ms, rotated back to world space.
  3. It's a throw when the speed is above `HandThrowMinSpeed` (220 cm/s), the motion points forward (dot with the head's flat forward > 0.35, not steeply down), no right finger is pinched, the right palm isn't facing you, and `HandThrowGrace` (0.45 s) has passed since the prop was picked up or since a right pinch. The grace stops the pickup pinch itself from throwing.
  4. Prop speed = hand speed x `HandThrowBoost` (2.6) x the prop's own throw multiplier, clamped to 300 - `HandThrowMaxSpeed` (1900) cm/s, along the hand's direction, plus the pawn's velocity. `ULTTCarryComponent::ThrowWithVelocity` does the release (same wall sweep, break-on-impact flag and `OnReleased` broadcast, so Marlowe's throw barks and breaking work as before). Log: `VR: hand throw, hand N cm/s, prop N cm/s`.
  5. The mop is never flick-thrown (scrubbing would throw it); Throw in the wrist menu still throws it.
- **Real mopping** (same function, when the held prop has `bIsMop`): an anchor point is kept in origin space; once the hand has moved `MopStrokeDistance` (14 cm) horizontally from it, `TryMopStroke()` runs. That re-checks the aim (`UpdatePlaceHint`) and only calls `ALTTWaterPuddle::MopUp` if the mop points at a puddle; it never drops the mop. The puddle refuses a stroke while the last one is still playing (`StrokeTime`), so the anchor only moves to the new spot when a stroke lands. Pinching the right index still mops as before.
- Turning either feature off: untick `bHandThrow` / `bHandMop` on the VR component (`BP_FirstPersonCharacter > VR`). The wrist menu Throw and the index pinch keep working.

Tunables (`Details > VR|Hands`): `PinchDistance` (1.2 cm), `PinchReleaseExtra` (0.8 cm), `PinchWalkSpeed` (0.45), `HandMenuWidth` (36 cm), `PalmMenuWidth` (30 cm); `VR|Hands|Throw`: `bHandThrow`, `HandThrowMinSpeed` (220), `HandThrowBoost` (2.6), `HandThrowMaxSpeed` (1900), `HandThrowGrace` (0.45); `VR|Hands|Mop`: `bHandMop`, `MopStrokeDistance` (14).

**Other gestures that could be added later** (not built): both palms facing you = recenter; a fist (all four fingers curled, no pinch) = grab; index pointed out = show the laser only while pointing. None are needed for the competition: every action already has a gesture or a wrist menu button.

## 5c. How to change a hand gesture yourself

All hand input lives in `Source/CleanCutAction/VR/LTTVRComponent.cpp`. Fingers are numbered **0 index, 1 middle, 2 ring, 3 pinky**.

1. **Remap a pinch:** open `ULTTVRComponent::HandlePinch`. The `if (bRightHand)` block has one `case` per right finger; the left-hand `switch` at the bottom has one per left finger. Move the `DoHandAction(...)` line to the finger you want. Left index (0) is walking and is handled every frame at the end of `UpdateHands` (`LeftPinchFinger == 0`); to move walking, change that `0`. Right index (0) is also the menu click; keep it on a finger that's easy to hit.
2. **Add a new action:** add a value to `ELTTVRHandAction` (`LTTVRComponent.h`), handle it in `ULTTVRComponent::DoHandAction` (inject an existing input action with `InjectAction(Path, FInputActionValue(true))`, or call code directly), then call it from a finger. A new `IA_` asset is only needed if a controller button must also trigger it (OpenXR only sees `IMC_VR`, see below).
3. **Add a wrist menu button:** in `ULTTVRHandMenu::NativeOnInitialized`, add an `AddButton(...)` line (row, column) and an `On...()` `UFUNCTION` in the header that calls `Fire(ELTTVRHandAction::...)`.
4. **Update the text the player sees** in three places: the wrist menu legend (`HandLegend...` lines in `ULTTVRHandMenu::NativeOnInitialized`), the HUD controls card (`ControlsListHands` in `Interaction/LTTCleanupHUD.cpp`), and the interact prompt labels (`bHands ? ...` in `CleanCutActionPlayerController::ApplyInputDevice`). Then this doc's table.
5. **Tune feel without code:** select `BP_FirstPersonCharacter`, the `VR` component, `Details > VR|Hands` (values above). Pinches too twitchy: raise `PinchDistance`. Throws fire by accident: raise `HandThrowMinSpeed` or `HandThrowGrace`. Throws too weak: raise `HandThrowBoost`.
6. **Rebuild** after header changes: close the editor, build `CleanCutActionEditor`, reopen (CLAUDE.md, *Rebuild routine*). Live Coding can't add new `UPROPERTY`s.
7. **Test:** watch the log for `VR: wrist menu action N` (N = position in `ELTTVRHandAction`), `VR: hand throw ...` and `VR: right palm menu shown`. For pinch trouble, `ltt.VR.PinchDebug 1` (section 5b).

**Mirror in VR (`ALTTMirror`).** A single scene capture from the head looks flat and wrong with both eyes open (each eye needs its own reflection). In VR the mirror renders one capture per eye (`Capture` left, `CaptureRight` right, square targets of `MaxResolution`): each from that eye's position (`CalculateStereoViewOffset`) reflected in the glass, with that eye's off-centre projection (`GetStereoProjectionMatrix`, horizontal offset flipped because the capture image is mirrored, near plane at the glass). `M_Mirror` picks the texture per eye with a Custom node (`ResolvedView.StereoPassIndex`: 0 = `MirrorTexture`, 1 = `MirrorTextureRight`). Flat play still uses one capture. Costs a second reflection render in VR while the mirror is on screen.

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
   - **Frame rate matters.** Below roughly 30 fps the simulator logs *"Client `Alpha` is not responsive"*, shows *"No frames received"* and **drops controller input (T does nothing)**. Measured on 2026-11-17: VR Preview in the editor ran at 18 fps (game thread 42 ms: editor Slate UI plus PIE; GPU 15 ms), 24 fps with Lumen off. `ULTTVRComponent` now switches Lumen GI off (`r.DynamicGlobalIlluminationMethod 0`) and uses screen-space reflections (`r.ReflectionMethod 2`) while in VR, restoring them on exit. For a usable rate, run the game outside the editor: `Scripts\RunVR_Standalone.bat` (`build` argument builds first; close the editor for that, since a standalone run doesn't include Live Coding patches). Restart the simulator if it reports *"Invalid session"* after sessions were started and stopped.
   - Turn off **Editor Preferences > General > Performance > Use Less CPU when in Background** (done on this machine). Otherwise the editor throttles itself while the simulator window has focus, and the simulator reports *"Client `Alpha` is not responsive"* / *"No frames received"*. Keep the VR Preview window open, not minimized.
   - Simulator keyboard bindings (Input bindings panel, keyboard and mouse): **W A S D R F** move the headset in the room (physical walking, not game locomotion), **arrow keys** rotate the head, **Q / E** tilt, **T** = index trigger, **U** = grip, **B** = A/X, **N** = B/Y, **M** = menu, **I** = thumbstick press, **Y** = thumbstick up (more stick keys further down the list). Game walking is the left thumbstick and snap turn the right thumbstick. *Use same input for both sides* sends every key to both controllers.
   - **Game actions on simulator keys:**

     | Game action | Controller | Simulator key |
     |---|---|---|
     | Move | Left thumbstick | Y / H / G / J (up / down / left / right) |
     | Snap turn | Right thumbstick left / right | G / J |
     | Pick up / place, click menus | Trigger | T |
     | Jump | A | B |
     | Hint | X | B |
     | Throw | B | N |
     | Dance | Y | N |
     | Pause | Menu | M |
     | Look | Head | Arrow keys (Q / E tilt) |
     | Step in the room | Body | W A S D (R / F up / down) |

     With *Use same input for both sides* on, every key drives both hands: B = jump + hint, N = throw + dance, G / J = strafe + snap turn. Turn it off to test buttons separately.
   - Pointer dots are drawn on the UI panel itself: **yellow** = where the pointer is (bigger over a button), **cyan** = where the head looks while the controller has the pointer. Aim the cyan dot at a button with the arrow keys and press T; the click falls back to the gaze when the controller ray misses.
   - To press a menu button without a controller: use the menu's keyboard shortcuts in the VR Preview window (briefing: **N** = just clean, **Enter** = time trial), or aim your head so the cursor dot grows on the button, then left-click / Enter in the VR Preview window.
   - WASD / mouse in the **VR Preview** window drive the game pawn (move head-relative, mouse turns), after Start; movement is blocked while the briefing is up, as in flat play.
5. **Hand tracking:** pick a profile with hand tracking and switch the inputs to hands; the simulator reports pinch and grab. The log shows `VR: hand tracking active` when the game sees a hand, and `VR: wrist menu action N` for each wrist menu button.
6. The simulator can record a session (VRS) for repeatable bug repros, and its footage is allowed for the competition video.

Logs: the editor log shows `VR: headset active, switching ... to VR mode` when the pawn enters VR.

**With a headset (Link / Air Link):** install the Meta Horizon Link PC app, set it as the active OpenXR runtime (Settings > General), connect, then VR Preview.

---

## 9. Quest standalone build (Quest 2 and newer)

Checked against Meta's docs on 2026-11-17: *Application Manifests for Release Builds* (`developers.meta.com/vr/resources/publish-mobile-manifest`), *Meta XR performance settings in Unreal Engine* and *Building and Packaging Apps* (Unreal).

### 9.1 One-time setup (this PC has no Android SDK yet)

0. **Engine Android component** (launcher install): Epic Games Launcher > *Unreal Engine > Library* > the 5.9 tile's **v** arrow > **Options** > tick **Android** under *Target Platforms* > **Apply**. Without it `Engine\Build\Android` (the Java / Gradle template) and the Android engine libraries are missing: *Project Settings > Android > APK Packaging > Configure Now* fails, and packaging can't work. Checked 2026-11-17: missing on this PC (also in 5.6 - 5.8).
1. Install **Android Studio** from `developer.android.com/studio` (default location). Open it once and finish the setup wizard. It downloads the SDK to `%LOCALAPPDATA%\Android\Sdk`.
2. Close the editor. Run `D:\UE5 Engines\UE_5.9\Engine\Extras\Android\SetupAndroid.bat`. It installs what this engine needs (`Engine\Config\Android\Android_SDK.json`: platform android-36, build-tools 36, NDK 27.2.12479018, CMake 3.22.1), asks you to **accept the SDK licenses**, and sets `ANDROID_HOME`, `NDKROOT` and `JAVA_HOME` for your user.
   If you set them by hand instead (*Start > Edit environment variables for your account*): `ANDROID_HOME` = `C:\Users\<you>\AppData\Local\Android\Sdk`, `NDKROOT` = `%ANDROID_HOME%\ndk\27.2.12479018`, `JAVA_HOME` = `C:\Program Files\Android\Android Studio\jbr`. The SDK still needs the `android-36` platform and CMake `3.22.1` (Android Studio installed `android-37` only); `SetupAndroid.bat` installs both.
3. Sign out and back in (or reboot) so every program sees the new environment variables. `Saved\Logs\AutoSDKInfo.txt` should then show `Android - buildable: True`.
4. Headset: Developer Mode on (Meta Horizon app on the phone, *Devices > Headset settings > Developer mode*), USB cable, allow USB debugging in the headset.

### 9.2 Build and install

- `Scripts\Package_Quest.bat` builds a **Shipping** APK into `Builds\Quest\Android_ASTC\` (the first run compiles every mobile shader and takes a long time; later runs are much faster). `Scripts\Package_Quest.bat dev` builds a **Development** APK that writes logs (`adb logcat -s UE`) and accepts `stat fps` / `stat unit`. `Scripts\Package_Quest.bat install` installs it on the headset afterwards.
- Same thing from the editor: *Platforms > Android > Package Project*, flavour **Android (ASTC)**.
- To install: drag the `.apk` onto the device in **Meta Quest Developer Hub**, or run the `Install_CleanCutAction-arm64.bat` next to it. On the headset it's in the library under **Unknown Sources**.
- This APK is **debug-signed** (`ForDistribution=False` in `Config/DefaultGame.ini`), which is fine for sideloading and testing. The Developer Dashboard release channel ("Competition") needs a **release-signed** one, see 9.5.

### 9.3 What was set for Quest, and why

`Config/DefaultEngine.ini`, `[/Script/Engine.RendererSettings]` (mobile only; PC VR keeps the desktop renderer):

| Setting | Value | Why |
|---|---|---|
| `r.Mobile.ShadingPath` | 0 (forward) | Already set. Meta's recommended path for Quest |
| `vr.MobileMultiView` | True (was False) | Both eyes in one pass: about half the draw calls. Meta recommendation |
| `r.MobileHDR` | False (was True) | Meta's "disable post processing": skips the HDR scene colour and tonemapper pass, and makes MSAA cheap. Bloom, auto exposure and other post effects don't show on Quest. **Check** the black fades (briefing, ending) and colours on the headset |
| `r.Mobile.AntiAliasing` / `r.MSAACount` | 3 (MSAA) / 4 | Already set. Meta's recommended 4x MSAA |
| `r.Mobile.FloatPrecisionMode` | 0 (half) | Already set. Meta recommendation |
| `r.AllowOcclusionQueries` | True | Already set |

`[/Script/AndroidRuntimeSettings.AndroidRuntimeSettings]`:

| Setting | Value | Why |
|---|---|---|
| `bPackageForMetaQuest` | True | The engine's Quest packaging: arm64 + Vulkan only, adds `com.oculus.permission.HAND_TRACKING`, `oculus.software.handtracking`, the overlay keyboard feature, drops phone permissions the Store rejects, no launch image |
| `MinSDKVersion` / `TargetSDKVersion` | 32 / 34 (were 26 / 36) | Meta's release rules: Quest 2 min 32, apps created since 2026-03-01 must target 34 |
| `InstallLocation` | Auto | Required by the Store |
| `bBuildForES31` | False | Vulkan only (Meta recommendation; Quest uses Vulkan) |
| `bRemoveOSIG` | False | Must stay unchecked (Meta) |
| `Orientation`, `bShowLaunchImage` | Landscape, False | Phone settings, unused in VR |
| `Build/Android/ManifestRequirementsAdditions.txt` (file, not an ini key) | `<uses-feature android:name="android.hardware.vr.headtracking" android:required="true" android:version="1" />` | Required for immersive apps. The packager appends this file to the manifest's requirements. `Build/` is git-ignored: commit it with `git add -f` |
| `ExtraActivitySettings` | intent filter with `MAIN` + `com.oculus.intent.category.VR` | Required for OpenXR apps: launches the game as VR, not as a 2D panel |
| `ExtraActivityNodeTags` | `android:excludeFromRecents="true"` | Required by the Store |
| `ExtraApplicationSettings` | `<meta-data>` children: `com.oculus.supportedDevices` = `quest2\|questpro\|quest3\|quest3s`; hand tracking `HIGH` frequency, `V2.0` | Device list for the Store; better hand tracking |

**Careful with the manifest keys** (UE 5.9 `UEDeployAndroid.cs`): `ExtraManifestNodeTags`, `ExtraApplicationNodeTags` and `ExtraActivityNodeTags` add **attributes** inside the `<manifest>`, `<application>` and `<activity>` tags (e.g. `android:excludeFromRecents="true"`). Whole elements (`<meta-data>`, `<intent-filter>`) go in `ExtraApplicationSettings` / `ExtraActivitySettings` (one string, lines separated by `
`; in the editor: *Extra Settings for <application> / <activity> section*), and manifest-level elements in `Build/Android/ManifestRequirementsAdditions.txt`. An element in a `*NodeTags` key makes the manifest invalid XML and the package fails. Don't enter the same `<meta-data>` twice.

`Config/Android/AndroidDeviceProfiles.ini`, new `[Android_OpenXR DeviceProfile]` section (every Quest profile, such as `Oculus_Quest2`, inherits it; it in turn inherits the phone tiers above it):

| CVar | Value | Why |
|---|---|---|
| `t.MaxFPS` | 0 | The phone profile capped at 60; on a 72 Hz Quest 2 that judders. The OpenXR runtime paces frames |
| `r.MSAACount` | 4 | `Android_Mid` (phones) drops to 2 |
| `xr.OpenXRFBFoveationLevel` / `xr.OpenXRFBFoveationDynamic` | 2 / 1 | Fixed foveated rendering (`XR_FB_foveation`): fewer pixels at the lens edges, raised only when the GPU needs it |
| `r.Shadow.CSM.MaxMobileCascades` | 1 | One sun shadow cascade |

Unchanged from the Android phone work and still in effect on Quest: textures capped at 1024 px (cooked build only), the sky light captured once, sun shadow distance 0.1. The engine's own `Oculus_Quest2` profile adds `sg.ShadowQuality=1` and Adreno occlusion settings. The mirror (`ALTTMirror::BeginPlay`) already drops to 512 px, no shadows, 12 m update range on Android.

### 9.4 Checking performance on the headset

- Target: a steady **72 fps on Quest 2** (the competition minimum is 60).
- Development APK: `stat fps`, `stat unit` (console from the keyboard over `adb shell "am broadcast -a android.intent.action.RUN -e cmd 'stat unit'"`), or **OVR Metrics Tool** from Meta Quest Developer Hub (fps, GPU / CPU level, overlay in the headset).
- If it's GPU bound: raise foveation to 3 (`xr.OpenXRFBFoveationLevel=3`), lower `r.MSAACount` to 2, or render a little smaller with `vr.PixelDensity=0.9` (all in `[Android_OpenXR DeviceProfile]`). If it's CPU (game thread) bound: fewer dynamic lights / shadow casters, `r.StaticMeshLODDistanceScale=2`.

### 9.5 Release signing for the Developer Dashboard

1. Create a keystore once and keep it safe; every later update must use the same one: `keytool -genkey -v -keystore cleancutaction.keystore -alias cleancutaction -keyalg RSA -keysize 2048 -validity 10000` (keytool is in Android Studio's `jbr\bin`). Put it in `Build\Android\`. `.gitignore` ignores `*.keystore` / `*.jks` and `Builds/`; never commit the keystore or its password.
2. *Project Settings > Platforms > Android > Distribution Signing*: Key Store `cleancutaction.keystore`, Key Alias `cleancutaction`, passwords. The editor writes these into `Config/DefaultEngine.ini` (`KeyStore`, `KeyAlias`, `KeyStorePassword`, `KeyPassword`): don't commit the password lines. `IniKeyDenylist` in `DefaultGame.ini` keeps them out of the packaged game.
3. *Project Settings > Packaging*: tick **For Distribution** (`ForDistribution=True`). That makes the manifest `android:debuggable="false"`, which the Store requires.
4. Package (`Scripts\Package_Quest.bat`), then upload the APK to the **Competition** release channel in the Developer Dashboard (or `ovr-platform-util upload-quest-build`).
5. Each new upload needs a higher `StoreVersion` (`[AndroidRuntimeSettings]`).

### 9.6 Quest 2 performance pass (2026-11-17)

**Measured first** (Shipping APK on a Quest 2, `adb logcat -s VrApi`, one line per second): **10 - 18 fps** against 72, GPU at 97 - 99 %, 60 - 90 ms of GPU time per frame (budget about 13.9 ms). Hand tracking and pinches are read once per frame, so at that rate pinches were missed or late. Over Quest Link the same scene felt fine because the PC renders it; Link numbers say nothing about the APK.

How to read the `VrApi` line: `FPS=12/72` = frames delivered / display rate; `Stale` = frames the compositor had to repeat; `GPU%` near 1.0 = GPU bound; `App=` = the app's GPU time per frame. Meta Quest Developer Hub's OVR Metrics Tool shows the same live in the headset.

**What the level had** (`L_NYC_Cafe_Michael`): 37 **movable** local lights (16 spot, 11 rect, 10 point; about 22 on at the start, nearly all after the power is fixed), a movable sun with a dynamic shadow, sky atmosphere, height fog, 7 Niagara systems, the mirror's two scene captures, and the ParisStreet street (`SampleMap` level instance: 189 actors, 184 meshes) outside the windows.

**Changes:**

| Change | Where | Why |
|---|---|---|
| ParisStreet street removed from the cafe level | `L_NYC_Cafe_Michael` (script: `Scripts/Python/quest_remove_samplemap.py`); its `MapsToCook` entry commented out | 184 meshes drawn through the windows, for both eyes |
| Phase 2 left out | `ALTTMessDirector::bOfferPhase2 = false` clears `NextLevelName` (the end screen hides "Shoot the Scene!"); the Phase 2 `MapsToCook` entry commented out | Competition build is Phase 1 only; smaller APK |
| Quest light budget | `LTTApplyQuestRenderBudget` in `LTTVRComponent.cpp`, run when VR starts on Android | Local lights: no shadows, no translucency lighting, no volumetric scattering, attenuation radius capped (`ltt.Quest.MaxLightRadius`, 450 cm). Sun shadow off (`ltt.Quest.SunShadows 0`). Rect lights can be taken out entirely (`ltt.Quest.RectLights 0`). Re-run live with `ltt.Quest.ApplyBudget`; `ltt.Quest.Budget 2` previews it on PC |
| Mirror updates one eye per frame on Quest | `ALTTMirror::Tick` (`#if PLATFORM_ANDROID`) | Each eye's reflection is a full extra scene render |
| UI panel redraws at 36 Hz on Quest | `UIPanel->SetRedrawTime` | The 1920 x 1080 widget tree was re-rendered into its texture every frame |
| Pinch threshold 1.0 -> 1.4 cm | `ULTTVRComponent::PinchDistance` | Quest standalone tracking reads touching fingertips a little further apart |

**Second pass (2026-11-18), measured on the Development APK** with live console commands:

| Test | Result | Reading |
|---|---|---|
| Start (after the first pass) | 28 fps, GPU 97 %, `Fov=0` | Still GPU bound; **foveation was never on** |
| `r.MSAACount 1` / `2` / `4` | 42 / 36 / 28 fps | MSAA is expensive here |
| `vr.PixelDensity 0.6`, `r.ScreenPercentage 60` | no change | Can't be changed while running on Quest (the swapchain is made at start) |
| `ShowFlag.StaticMeshes 0` | 73 fps, GPU 51 % | The meshes are the cost |
| `r.StaticMeshLODDistanceScale 4` (MSAA 1) | 43 -> 64 fps looking into the room | Geometry: see the audit |
| Post process, translucency, particles, decals, shadows, local lights off | about 0 each | Not the bottleneck |

The numbers move with where the headset looks, so compare within one view. `Scripts/Python/quest_mesh_audit.py` (run headless, writes `Saved/QuestMeshAudit.csv`) lists every mesh in the cafe: **4.4 M triangles at LOD0** (tables 41k each x 18, ceiling lamps 32k x 10, chairs 9k x 36, coffee bags 6k x 41...), 2.3 M at LOD1, 1.2 M at LOD2. Quest 2 handles about 1 M. Every heavy prop has working LODs at 50 % / 25 %, so no asset was changed.

| Change | Where | Why |
|---|---|---|
| Foveation on | `[/Script/OpenXRHMD.OpenXRHMDSettings] bIsFBFoveationEnabled=True` (`DefaultEngine.ini`); level 3 (high) + dynamic (`[Android_OpenXR DeviceProfile]`) | `xr.OpenXRFBFoveationLevel` does nothing without this flag: the engine never creates the foveation images (`VrApi ... Fov=0`) |
| MSAA 4x -> 2x | `[Android_OpenXR DeviceProfile] r.MSAACount=2` | 4 - 5 fps |
| LODs switch earlier | `[Android_OpenXR DeviceProfile] r.StaticMeshLODDistanceScale=4` | Cuts the drawn triangles to between a half and a quarter |
| Sun off on Quest | `ltt.Quest.Sun 0` (`LTTApplyQuestRenderBudget`) | Without its shadow the 3200 K sunset sun lit every surface facing it **indoors too**: the whole cafe looked orange-red |
| Height fog hidden on Quest | `ltt.Quest.Fog 0` |

**Build with all of the above, measured (2026-11-18):** foveation now on (`Fov=3`), MSAA 2x and LOD scale 4 applied (logcat `Pushing Device Profile CVar`), sun and fog off; still **~24 fps**, GPU 99 %. Perfetto traces (Meta Quest Developer Hub tooling, `mode gpu`) showed why:

- The eye pass is 1440 x 1584 per eye, binned into ~45 - 60 tiles of 0.5 - 0.7 ms each: ~32 - 37 ms per frame.
- GPU counters: **88 % of the time shading fragments, ~70 M fragments shaded per frame** for ~4.5 M screen pixels (both eyes): about **15 layers per pixel**. ~1.8 M polygons per frame, average area 81 px. The same with MSAA 1x and with low LODs: the cost is **overdraw**, not triangles, lights or materials.
- Tried at startup through a `UECommandLine.txt` pushed to `/sdcard/Android/data/com.ohmcodes.cleancutaction/files/UnrealGame/CleanCutAction/` (Development builds read it; **delete it after testing**): `-dpcvars=r.Mobile.EarlyZPass=1` (full depth prepass) made it worse (13 fps); `vr.PixelDensity=0.7` is not applied on this OpenXR path (the eye buffer stays 1440 x 1584).
- Not yet tested (the headset disconnected): `r.Mobile.MeshSortingMethod 1` (strict front-to-back, so Adreno's low-res Z rejects what's hidden; the default sorts by material state), and hiding geometry the player never sees (the building shell, upper floors, back faces of walls).
- **Tested next, on the headset with the cafe lit (2026-11-18): the local lights are the main cost after all.** `r.Mobile.MeshSortingMethod 1` made no difference (52 vs 53 fps), so it was left out. With the power off (lamps hidden by the game) the same build ran 50 - 65 fps; lit, 16 - 31. Live: rect lights out 16 -> 19 fps, light radius 250 cm -> 25 fps, 150 cm -> ~30 fps (too dark). (An earlier `ShowFlag.LocalLights 0` test looked flat only because the view kept changing.) The overdraw the GPU counters showed is the lights' per-pixel work across 37 movable lights whose 4.5 m radii overlapped everywhere.
- **New defaults** (`LTTVRComponent.cpp`): `ltt.Quest.RectLights 0`, `ltt.Quest.MaxLightRadius 200` (live, lit cafe: 450 cm 16 fps, 250 cm 25, 200 cm ~+20 % more, 150 cm too dark), `ltt.Quest.LightIntensityScale 2.5` (every lamp 2.5x brighter: free on Quest, and it stands in for the PC post process's exposure boost, auto exposure bias 1.3 - 2.0 in the cafe volumes, which Quest doesn't run; without it the lit cafe was too dark to see the floor), `ltt.Quest.SkyLightScale 3` (the level's sky light x 3 to fill in; base intensities remembered so re-applying doesn't compound), `ltt.Quest.PixelDensity 0.85` (eye buffers at 85 % of the recommended 1440 x 1584: UE 5.9 has no `vr.PixelDensity` cvar and nothing called `IHeadMountedDisplay::SetPixelDensity`, which is why the earlier resolution tests changed nothing), and an optional `ltt.Quest.LightSpacing` (cm) that switches off a lamp when a stronger one is kept that close. All re-applied live with `ltt.Quest.ApplyBudget`. Measured with 250 cm and no density change: briefing 62 - 67 fps (was 24), lit cafe a steady 35 fps (the runtime holds frames for 2 refreshes when one misses 72, so it shows 36 until the frame fits in 13.9 ms).
- **Third pass (2026-11-18, Quest 2):** VrApi showed `Fov=0`: the device profile's `xr.OpenXRFBFoveationLevel` was not in effect, so `LTTApplyQuestRenderBudget` now sets it on VR entry (`ltt.Quest.Foveation`, default 3). Measured live in the cafe: density 0.85 without foveation ~27 fps, 0.7 with foveation 3 ~50 - 72 fps depending on the view, so `ltt.Quest.PixelDensity` now defaults to **0.7**. Darkness: the level's sky light captures a black sky on mobile (no Lumen; the SkyAtmosphere needs an IsSky mesh), so everything outside the 200 cm lamp radius was black and `SkyLightScale` did nothing. `ltt.Quest.SkyCubemap 1` (default) switches the sky light to the engine's `/Engine/MapTemplates/Sky/DaylightAmbientCubemap` (in `MapsToCook`) with a warm lower hemisphere, or spawns one if the level has none; brightness via `ltt.Quest.SkyLightScale`.
- **Headset feedback fixes (2026-11-18):** menus lose their full-screen `Dim` backdrop in VR (`LTTAdaptWidgetForVR`); a menu (any focusable captured widget) is placed in front of the head when it opens and then stays in the room, coming back in front only after a 70 degree turn, while the HUD stays head-locked (`UpdatePanel`, `bPanelMenuPlaced`); carried props sit just past the right hand along its pointing direction (`ULTTVRComponent::GetHoldHand`, used in `ULTTCarryComponent::ComputeHoldTarget`); the focus / hint outline is the `M_VRGlow` overlay in VR (`ULTTVRComponent::SetOutline`, the PP_Outline post process needs Mobile HDR); left ring held = walk backward; pinch: a firm pinch (gap under half the threshold) presses on the first frame, and the left index walk needs a gap under 70 % of the threshold so a half-relaxed hand does not walk. Second round: the hand rays run through the index and middle knuckles (the thumb joint moved during a pinch and knocked the ray off buttons) and hold almost still while the index closes; the stricter walk threshold applies only in play, so menu clicks use the normal one; held props sit at the knuckles (`Radius * 0.3 + 1` cm out); the sky cubemap no longer un-hides the sky light, so the breaker blackout at the start stays dark; the laser has translucent sort priority 10010 so its end draws over the on-top UI panels.
- `Scripts/Python/quest_material_audit.py` (writes `Saved/QuestMaterialAudit.csv`): 824 of ~900 mesh components use one opaque two-sided material (`MM_MasterSimple`); masked (36) and translucent (46) are minor. Its reddish in-scatter (0.35, 0.16, 0.10) tinted the interior; on PC the post process (colour grading, saturation 0.9) balanced it, and Quest has no post process (`r.MobileHDR=False`) |

**Live tuning on a Development APK** (`Scripts\Package_Quest.bat dev`): console commands go in over USB, for example

```
adb shell "am broadcast -a android.intent.action.RUN -e cmd 'ltt.Quest.MaxLightRadius 300'"
adb shell "am broadcast -a android.intent.action.RUN -e cmd 'ltt.Quest.ApplyBudget'"
adb shell "am broadcast -a android.intent.action.RUN -e cmd 'ShowFlag.RectLights 0'"
```

and `adb logcat -s VrApi` shows the effect within a second. Values that work go into the cvar defaults in `LTTVRComponent.cpp` (or `[Android_OpenXR DeviceProfile]` for engine cvars).

**Project location:** the D: drive (Sabrent Rocket nano) started dropping out and returning stale or unreadable data on 2026-11-17, so work moved to a temporary copy on `C:\UE5 Projects\CCA_VR` for a day. On 2026-11-18 everything was copied back (checksum-verified) and **D: is the main copy again**: build and package from `D:\UE5 Projects\CCA_VR`.

## 8. Known limits and follow-ups

- **Quest frame rate:** the second pass (section 9.6) was measured piece by piece, not yet as a whole build. If it's still under 72, next steps in order: `vr.PixelDensity=0.8` in `[Android_OpenXR DeviceProfile]` (needs a new build: it can't change at runtime), `r.MSAACount=1`, and lower-poly replacements for the tables and ceiling lamps.
- **New on 2026-11-18, not yet tried in the headset:** the palm menu and fingertip poke, hand aiming with the in-play laser, gravity pull, walking through chairs, the Quest controller models, and the right ring jump / left ring throw swap.
- **Tested in the Meta XR Simulator and on a Quest over Link**, not as a standalone APK yet.
- **New on 2026-11-17, not yet tried in the headset:** the remapped finger layout, the right palm pause, the real throw and real mopping. Tune `HandThrowMinSpeed` / `HandThrowBoost` / `MopStrokeDistance` after the first try.
- **Hand tracking works on a Quest over Link** (pinches, walking, wrist menu tested by the developer; walking was too fast before `PinchWalkSpeed`). The per-finger pinch layout, the template hand meshes (tracked and controller), the glow and the per-eye mirror are built and compile but are not yet tested in the headset. If a tracked hand mesh looks twisted, the thumb twist (`bHandUp`) or the reference "back of the hand" sign in `LTTBuildHandBones` are the first things to check. The whole game still needs a full run with hands only before submission (pass/fail).
- The VR Template feature pack import showed "Failed to create asset /Game/TP_VirtualRealityBP" and "Failed to find BP_XRPawn": its content (`XRFramework`, `XRMannequins`, `VRSpectator`) was copied, only the pack's last step failed. It also added legacy action / axis mappings to `DefaultInput.ini` (GrabLeft, TriggerAxisRight, Spectator...); OpenXR ignores legacy mappings and nothing in the game uses them.
- **Frame rate:** On a Quest over Link (RTX 3070, 2880 x 1584), VR Preview measured 15 - 20 fps before the performance set and instanced stereo. At that rate, Link drops controller input and aiming at the UI is near impossible. VR Preview in the editor runs at roughly 18 - 24 fps on this PC, below what the simulator accepts; use `Scripts\RunVR_Standalone.bat` for real testing. The Quest build still needs the mobile renderer and its own performance pass (60 fps minimum).
- Full-black camera fades (ending, tour cuts) still cover the UI panel for their duration; `ltt.VR.UILayer 1` (stereo layer) would avoid that but isn't verified in the simulator.
- The world body has no hand IK, so in a mirror its arms play the normal animation. Walking with the headset (room-scale) moves the capsule without velocity, so the legs only animate for stick movement.
- Running the editor, VR Preview and the XR Simulator together exceeds video memory on this PC ("Video memory has been exhausted"); another reason to test with `RunVR_Standalone.bat`.
- The Phase 2 (shoot) HUD hasn't been reviewed in VR yet; its key labels may still show keyboard keys.
- Widgets that call `AddToViewport` again while already on the panel log a "has a parent" warning and stay where they are.
- Jump in VR can be uncomfortable; consider removing it.
- `AllToolsets` is enabled for the editor; it failed the cook once before (game feature plugins). Cook-test before the Quest package and disable it for that build if needed.
- `WBP_ShootEndScreen` still has the old copyright text baked into the asset (fixed at runtime). Re-bake or edit it in the editor when convenient.
- **Build machine clock:** on 2026-11-16 the PC clock jumped back about 11 hours, which made Live Coding report "no changes" and left stale unity objects (link errors). Keep Windows time synced; if it happens again, delete `Intermediate/Build/Win64/x64/UnrealEditor/Development/CleanCutAction` and rebuild.
