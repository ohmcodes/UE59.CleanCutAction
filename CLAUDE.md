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
- `Source/CleanCutAction/VR/LTTVRComponent` (`ULTTVRComponent`, on the character): switches the pawn into VR when a headset renders (camera on a VR origin at eye height, controllers, snap turn, laser pointer) and moves every viewport widget onto a world-space UI panel. Flat play is untouched.
- Interaction aims with the head (`GetPlayerViewPoint` is the HMD view); carry holds props in front of the camera.
- **OpenXR only sees input from Enhanced Input default mapping contexts** (`Config/DefaultInput.ini`, currently `IMC_VR`). New VR controls need an `IA_` asset in `/Game/_CleanCutAction/Input/VR/`, a mapping in `IMC_VR` (`Scripts/Python/create_vr_input.py`) and a `MapsToCook` entry if loaded by path. Runtime-created actions don't reach OpenXR.
- `bStartInVR=True` in `Config/DefaultGame.ini`.
- Testing without a headset: Meta XR Simulator (standalone OpenXR runtime) set as the active runtime, then **Play > VR Preview**.

## Build and tools

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
- The editor's default map is `L_NYC_Cafe_Michael` (`Config/DefaultEngine.ini`), a per-dev copy of the cafe level.
- Set art comes from the StylizedNewYorkCity pack (`/Game/StylizedNewYorkCity`).

## Ownership and copyright

- The game is owned by **Ohmcodes**. The full notice is in `COPYRIGHT.md`; `Config/DefaultGame.ini` (`CompanyName`, `CopyrightNotice`) carries the same owner.
- Use `Copyright © 2026 Ohmcodes. All Rights Reserved.` wherever a copyright line is needed (end screens, credits, store pages, file headers).
- Don't name any other company as owner in code, config, UI or docs.
- Third-party packs in `Content/` keep their own licenses. Don't put the Ohmcodes notice on them.
- `LICENSE` is a proprietary all-rights-reserved license from Ohmcodes. Don't add open-source license headers to files.
