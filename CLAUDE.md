# Clean! Cut! Action! VR (Meta Quest, `vr` branch)

First-person cleaning / directing game, ported to **Meta Quest VR** for the **Meta VR Start Developer Competition 2026** (Gaming track, New Experience division). Deadline: **2026-11-18 12:00 PM PT** (2026-11-19 04:00 Manila).
Engine: **UE 5.9.0 Preview** at `D:\UE5 Engines\UE_5.9`. C++ for systems, Blueprint subclasses for content and tuning.
Target: **Quest APK** (Android OpenXR) uploaded to a Meta Developer Dashboard release channel named "Competition". PC VR (Link / Air Link / Meta XR Simulator) is the dev loop.

This branch is Quest-only. The flat desktop, web (Wasm / itch.io) and Android phone builds live on `main`, `wasm`, `android`, `windows` and `linux`; don't optimise this branch for them.

**Start with `Docs/10_VR_Meta_Quest.md`**: competition rules, every VR change, how the VR systems work, controls, testing with the XR Simulator, known limits. Update it whenever VR code, input or config changes.

The original design docs live in `Docs/` and describe the flat game. The overview below is imported in full; read the other files when a task touches their area.

@Docs/00_README.md

## Read these when working on...

| Area | File |
|---|---|
| **VR / Quest port, competition rules, XR Simulator** | `Docs/10_VR_Meta_Quest.md` |
| Game design, story, characters | `Docs/CleanCutAction_Design.md` |
| What's done / what's next | `Docs/01_Master_Checklist.md` (VR tasks in section V) |
| Classes, save, level flow, input, interaction, carry, tweens, gameplay tags, director barks, pause | `Docs/02_Architecture_and_Systems.md` |
| Phase 1: messy set, Messifier tool, cleaning mechanics | `Docs/03_Phase1_Cleaning_Implementation.md` |
| Phase 2: cafe shoot, cue/beat director, effect stations, frame cone, take meter | `Docs/04_Phase2_Directing_Implementation.md` |
| Marlowe's lines, Phase 2 scene script | `Docs/05_Script_Director_and_Actors.md` |
| Menus, HUDs, end screens, tween specs (and the VR UI panel) | `Docs/06_UI_and_Tweening.md` |
| SFX and music | `Docs/07_Audio_List.md` |
| Niagara, materials, post-process | `Docs/08_VFX_and_Shaders.md` |
| Packaging (Windows, Web, Android, Linux; Quest in the VR doc) | `Docs/PackagingGuide/` |

## Competition rules that shape the code

- **Hands-first is pass/fail**: the whole game must be playable with hand tracking, never pairing a controller. Controllers are allowed as an extra.
- Design for **seated / 2 ft radius**, a satisfying session in **10 minutes or less**, fast start, clean pause/resume.
- **60 fps minimum on Quest**, no crashes. Keep essential UI inside a narrow field of view.
- No real brands, logos or identifiable people in the game. English text.
- The submitted APK is frozen at the deadline.

## VR implementation (current state)

- XR runs on the engine's **OpenXR** plugin (+ `OpenXRHandTracking`), not the Meta XR plugin (no 5.9 build; it could be compiled from source later for Meta-only features).
- `Source/CleanCutAction/VR/LTTVRComponent` (`ULTTVRComponent`, on the character): switches the pawn into VR when a headset renders (camera on a VR origin at eye height, controllers, snap turn, laser pointer, capsule and body following the head, Lumen off) and moves every viewport widget onto a head-locked, always-on-top UI panel (`M_VRWidget_OnTop`). HUD legend and prompts show Touch controller labels in VR. Flat play is untouched.
- **Hand tracking (hands-first):** the same component reads OpenXR hand joints (`GetHandTrackingState`). Each finger pinched to the thumb is a button: left index held = walk (slow), left middle = turn left, left ring = jump, left pinky = hint; right index = trigger (pick up / place, mop, click menus via a shoulder-to-knuckle hand ray), right middle = turn right, right ring = throw, right pinky = dance. A forward flick of the right hand throws the held prop with the hand's speed (`ULTTCarryComponent::ThrowWithVelocity`); scrubbing the right hand while holding the mop over a puddle mops (`ULTTInteractionComponent::TryMopStroke`). Right palm held toward the face 0.6 s = pause menu. Left palm toward the face = wrist menu (`ULTTVRHandMenu`: score, gesture list, Hint, Pause, Jump, Throw, Turn, Dance, Recenter); the HUD's top-left score and controls card hide while hands are used. Gestures inject the existing Enhanced Input actions (`InjectInputForAction`), so gameplay code doesn't know about hands. Hands use the VR Template meshes (`/Game/XRMannequins`): posed from the joints when tracked, the template's animated hand on each controller otherwise, with an unlit glow overlay (`M_VRGlow`). Works on a Quest over Link; the per-finger layout and meshes are not yet tested in the headset. Details: `Docs/10_VR_Meta_Quest.md` section 5b; how to remap gestures: section 5c.
- **Mirror in VR:** `ALTTMirror` renders one reflection per eye with that eye's projection; `M_Mirror` picks it with `ResolvedView.StereoPassIndex`.
- Real testing in the XR Simulator needs a decent frame rate: use `Scripts\RunVR_Standalone.bat` (after a rebuild) rather than VR Preview.
- Interaction aims with the head (`GetPlayerViewPoint` is the HMD view); carry holds props in front of the camera.
- **OpenXR only sees input from Enhanced Input default mapping contexts** (`Config/DefaultInput.ini`, currently `IMC_VR`). New VR controls need an `IA_` asset in `/Game/_CleanCutAction/Input/VR/`, a mapping in `IMC_VR` (`Scripts/Python/create_vr_input.py`) and a `MapsToCook` entry if loaded by path. Runtime-created actions don't reach OpenXR.
- `bStartInVR=True` in `Config/DefaultGame.ini`.
- Testing without a headset: Meta XR Simulator (standalone OpenXR runtime) set as the active runtime, then **Play > VR Preview**.

## Build and tools

- **Rebuild routine after C++ changes:** close the editor > Generate Visual Studio project files > build `CleanCutActionEditor` (Development Editor, Win64) > reopen the project. Live Coding only patches the running editor; standalone VR runs (`Scripts\RunVR_Standalone.bat`) load the module from disk and need this real build.

- Build the editor (editor closed or Live Coding off):
  `"D:\UE5 Engines\UE_5.9\Engine\Build\BatchFiles\Build.bat" CleanCutActionEditor Win64 Development -Project="D:\UE5 Projects\CCA_VR\CleanCutAction.uproject" -WaitMutex`
- Run an editor Python script headless (use forward slashes in the script path; `\U` in `D:\UE5...` breaks it):
  `"D:\UE5 Engines\UE_5.9\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\UE5 Projects\CCA_VR\CleanCutAction.uproject" -run=pythonscript -script="D:/UE5 Projects/CCA_VR/Scripts/Python/<script>.py" -unattended -nop4 -nosplash -NullRHI`
- `PythonScriptPlugin` and `EditorScriptingUtilities` are enabled for the editor only.
- The `unreal-mcp` server needs the editor open. `AllToolsets` is enabled with `TargetAllowList: ["Editor"]` (it's an editor-only plugin), so the editor exposes its tools and game targets never compile it.

## Current project state (differs from the design docs)

- Game content folder is `/Game/_CleanCutAction/` (`Blueprints/Interactables`, `Blueprints/Widgets`, `BreakFX/`, `Destruction/` geometry collections, `Input/` (+ `Input/VR/`), `Materials/`, `MusicFX/`, `UI/`), not `Content/CleanCutAction/` as the docs say.
- `Source/CleanCutAction/` has the template base classes (`CleanCutAction*` character, game mode, player controller, camera manager) plus `LTT` classes: `Interaction/` (carry, interaction component, interactable base, mess director, cleanup HUD, end screen, first person anim instance), `Shoot/` (Phase 2 scene director, effect stations, end screen), `UI/` (briefing screen, pause menu, UI theme, widget baker, input device, leaderboard, virtual keyboard), `Audio/` (music player), `VR/` (VR component). Some classes from the docs (save game, tween subsystem) aren't written.
- Cooked maps: `L_NYC_Cafe_Michael` (Phase 1, `GameDefaultMap`), `L_NYC_Cafe_Phase2_Michael` (Phase 2, `MapsToCook`) and the `/Game/ParisStreet/Levels/SampleMap` level instance both cafes use. `L_NYC_Cafe` is not cooked, so no level travel may point at it. `MapsToCook` in `Config/DefaultGame.ini` also force-cooks assets loaded only by path (widgets, SFX, materials, VR input); add any new one there.
- Packaging: `NNEDenoiser` is disabled so its 54 MB of path-tracer models are not force-cooked. `AllToolsets` (editor only) once failed the cook through its game feature plugins; before packaging the Quest APK, run a cook and, if it fails again, set it to `"Enabled": false` for that build. Keep `bSkipEditorContent=False`: the Landmass plugin's startup materials reference `/Engine/EditorMaterials`, and skipping editor content fails the cook.
- Android package name: `com.ohmcodes.cleancutaction`.
- **Working copy is on C: (`C:\UE5 Projects\CCA_VR`)** since 2026-11-17: the D: drive drops out and loses fresh writes. The engine is still on D:. The Quest build is **Phase 1 only** (`ALTTMessDirector::bOfferPhase2`; the Phase 2 and ParisStreet `MapsToCook` entries are commented out) and the ParisStreet level instance is removed from the cafe. Quest render budget: `ltt.Quest.*` cvars in `LTTVRComponent.cpp` (doc section 9.6).
- **Quest APK:** `Scripts\Package_Quest.bat` (`dev` = Development, `install` = also install on the headset) into `Builds\Quest\Android_ASTC\`. Quest settings: `bPackageForMetaQuest`, min / target SDK 32 / 34, Vulkan only, multiview, Mobile HDR off, 4x MSAA, Meta release manifest entries (`DefaultEngine.ini`), and `[Android_OpenXR DeviceProfile]` in `Config/Android/AndroidDeviceProfiles.ini` (no 60 fps cap, foveation). Needs the Android SDK from Android Studio + `Engine\Extras\Android\SetupAndroid.bat`. Release signing steps: `Docs/10_VR_Meta_Quest.md` section 9.
- The editor's default map is `L_NYC_Cafe_Michael` (`Config/DefaultEngine.ini`), a per-dev copy of the cafe level.
- Set art comes from the StylizedNewYorkCity pack (`/Game/StylizedNewYorkCity`).

## Ownership and copyright

- The game is owned by **Ohmcodes**. The full notice is in `COPYRIGHT.md`; `Config/DefaultGame.ini` (`CompanyName`, `CopyrightNotice`) carries the same owner.
- Use `Copyright © 2026 Ohmcodes. All Rights Reserved.` wherever a copyright line is needed (end screens, credits, store pages, file headers).
- Don't name any other company as owner in code, config, UI or docs.
- Third-party packs in `Content/` keep their own licenses. Don't put the Ohmcodes notice on them.
- `LICENSE` is a proprietary all-rights-reserved license from Ohmcodes. Don't add open-source license headers to files.
