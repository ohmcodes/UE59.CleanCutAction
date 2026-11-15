# Clean! Cut! Action! - Implementation Docs

First-person 3D casual game for a 3-day AI game jam (Epic Mega AI Jam 2026). Engine: **Unreal Engine 5.9.0 Preview**, C++ core with Blueprint subclasses for content. Builds: Wasm (WebGPU) for itch.io, and Windows.

These docs were first written on 2026-11-12 as a plan. `01_Master_Checklist.md` records what was actually built, cut or left open as of 2026-11-15 (HEAD 87eadcc).

## The Game in One Paragraph

The player is the Rookie, a production assistant on the last day of shooting *Clean! Cut! Action!*. **Phase 1:** the two-floor cafe set gets wrecked (an "earthquake" throws every prop around when the level starts). The player puts each prop back on its home silhouette, bins the trash, washes dishes in the sink and mops up puddles, against an optional countdown, while director Marlowe Finch heckles through a megaphone. Thrown props can break and cost points; scores go to an online leaderboard. **Phase 2:** once the set is clean, "Shoot the Scene" opens the shoot level. Two actors (real-life exes Dash and Juniper) play their date scene, and the player runs between effect stations on both floors (slate, traffic, door bell, steam, prop tosses, light gels, thunder, wind, boom mic, rain) on Marlowe's cues while staying out of the film camera's frame.

## File Index

| File | Contents |
|---|---|
| `00_README.md` | This file: overview, tech decisions, conventions, folder layout |
| `01_Master_Checklist.md` | Task checklist from setup to submission, marked built / cut / open, plus work added during the jam |
| `02_Architecture_and_Systems.md` | Project structure, classes, level flow, input, interaction, carry |
| `03_Phase1_Cleaning_Implementation.md` | Phase 1 cleaning mechanics and progression |
| `04_Phase2_Directing_Implementation.md` | Phase 2 shoot: cue/beat director, effect stations, frame cone, take meter, retakes, actors |
| `05_Script_Director_and_Actors.md` | Marlowe's Phase 1 lines by trigger, the Phase 2 scene script, cue call/result lines |
| `06_UI_and_Tweening.md` | Briefing, pause, HUDs, end screens, animation specs |
| `07_Audio_List.md` | SFX and music |
| `08_VFX_and_Shaders.md` | Niagara systems, materials, post-process effects |
| `09_Phase2_Asset_Needs.md` | Assets Phase 2 still needs from outside the project, with the slot each one plugs into |
| `CleanCutAction_Design.md` | Game design, story, characters |
| `Itch_Page.md` | Copy for the itch.io project page |
| `L_NYC_Cafe_Changes.md` | Change log of the cafe level since the stock StylizedNewYorkCity version |
| `L_NYC_Cafe_Changes_4beed6a.md` | Cafe level changes in commit `4beed6a` |
| `Level_Changes_dev-michael.md` | Cafe level changes on the `dev/michael` branch, with coordinates, for re-applying by hand |
| `PackagingGuide/` | How to package for Windows, Web (Wasm), Android and Linux / Steam Deck, plus `Build_Platform.bat` |
| `Submission/` | Jam theme answer and NVIDIA RTX Award notes |

## Key Technical Decisions

| Decision | Choice | Why |
|---|---|---|
| Language | C++ for systems, Blueprint subclasses for content/tuning | Text-based code is easiest for an AI agent to write and review |
| Maps | `/Game/StylizedNewYorkCity/Level/L_NYC_Cafe_Michael` (Phase 1, `GameDefaultMap`) and `L_NYC_Cafe_Phase2_Michael` (Phase 2, `MapsToCook`). Both use the `/Game/ParisStreet/Levels/SampleMap` level instance for the street. No menu map. | The StylizedNewYorkCity cafe pack gave a finished two-floor set |
| Map authoring | The cafe is authored clean. `ALTTMessDirector` records every prop's home at runtime, then throws the props around | Homes always match the clean set, and there is no separate messy map or Messifier script to maintain |
| Save | No progress save. `ULTTPlayerProfileSave` keeps only the leaderboard player id and name (browser localStorage on Wasm) | Each phase is one short session; the leaderboard is the persistent record |
| Level flow | Direct `OpenLevel` calls: Phase 1 end screen → Phase 2, Phase 2 end screen → back to Phase 1. Every travel target must be a cooked map | No level-flow subsystem was needed for two levels |
| Scene logic (Phase 2) | Beat-based state machine (`ALTTSceneDirector`), **not** one long Level Sequence. Reads a `ULTTSceneData` Data Asset if one is assigned; otherwise uses the built-in Scene 42 in C++ (no `DA_Scene42` asset exists) | Actors wait for cue results, so failures and retakes don't desync |
| Barks | Lines are `LOCTEXT` tables in C++ (`ALTTCleanupBarker` for Phase 1, `LTTSceneData.cpp` for Phase 2), not a DataTable | Faster to write and review as text |
| Tweens | No tween subsystem; each class eases its own animations inline | Fewer moving parts for a jam |
| UI | Plain UMG. Every widget builds its layout in C++ with the shared `LTTUITheme.h`; `ULTTWidgetBaker` copies a layout into a `WBP_` child for designer edits | Works the same on Wasm, Windows and touch |
| Perspective | First person with arms (hand IK on the held prop); the full Rookie body shows in `ALTTMirror` | |
| Input | Enhanced Input (template `IMC_Default` + `IA_Interact`, `IA_Throw`, `IA_Emote`). Keyboard/mouse, gamepad and touch; `ULTTInputDeviceSubsystem` switches prompts to the device in use | The web build runs on desktop and phones |
| Online | Leaderboard over HTTP (`ULTTLeaderboardSubsystem`): browser `fetch()` on Wasm, `FHttpModule` elsewhere | The engine's HTTP module is a stub on Wasm |

## ⚠️ Preview Engine Warning

UE 5.9.0 is a **preview** build.

- Verify every API against the 5.9 preview headers before relying on it.
- Preview projects can't be opened in older engine versions. Commit often.
- Work around preview-specific crashes instead of debugging the engine. Packaging workarounds are listed in `CLAUDE.md` and `PackagingGuide/` (there is no `KNOWN_ISSUES.md`).

## Naming Conventions

| Asset | Prefix | Example in the project |
|---|---|---|
| C++ classes | `LTT` | `ALTTInteractableBase`, `ALTTSceneDirector` |
| Blueprint | `BP_` | `BP_Prop_Chair_01` |
| Widget | `WBP_` | `WBP_PauseMenu` |
| Data Asset | `DA_` | `DA_PropSounds` |
| Geometry Collection | `GC_` | `GC_Generic_Shatter` |
| Material / Instance | `M_` / `MI_` | `M_GhostSilhouette`, `MI_Mop_Wood` |
| Niagara | `NS_` | `NS_Rain` |
| Sound Wave | `SW_` | `SW_PuddleSplash` |
| Input Action / Context | `IA_` / `IMC_` | `IA_Interact` |
| Map | `L_` | `L_NYC_Cafe_Michael` |

## Content Folder Layout

Game content is in `/Game/_CleanCutAction/` (`Content/_CleanCutAction/`). Set art and characters come from packs elsewhere in `Content/` (StylizedNewYorkCity, ParisStreet, ST_Characters_JC, BroadcastStudio, Hollywood_Vol3, ...), and the First Person template's `FirstPerson/` and `Input/` folders are still used.

```
Content/_CleanCutAction/
  Blueprints/
    Interactables/   (BP_MasterInteractables, lamps, sink, electric box, water puddle)
      Props/         (111 BP_Prop_* props)
    Widgets/         (WBP_* for every C++ widget)
  BreakFX/           (GC_Generic_Shatter, the shared break pieces)
  Character/         (Rookie mesh, rig and animations, incl. A_Emote_Rookie)
  Characters/        (Dash and Juniper locomotion blendspaces)
  Destruction/       (GC_* geometry collections per prop)
  EmoteMusic/        (emote dance track)
  Input/             (IA_Interact, IA_Throw, IA_Emote)
  Materials/         (silhouette, broken prop, frame cone, puddle, coffee, mirror, PostProcess/M_PP_Outline)
  Meshes/            (mop, Rookie source mesh)
  MusicFX/           (background music tracks)
  SFX/               (Props/ per-material sounds + DA_PropSounds, stations/ Phase 2 sounds, other SFX)
  UI/                (Icons/, Widget/ images, older WBP_ copies)
  VFX/               (NS_Rain, NS_Steam, NS_Wind, NS_Sparks, NS_MopSplash, NS_SinkBubbles)
Scripts/
  serve_wasm.py      (local web server for testing the Wasm build)
  Python/VideoToAnim/  (video-to-animation tool used for the emote)
```

Assets loaded only by path (widgets, some SFX, materials, the emote) must be listed in `MapsToCook` in `Config/DefaultGame.ini`.

## Hand-off Order (historical)

This was the order the docs were meant to be handed to the AI agent on day 1. The project no longer follows it: many planned systems were cut or replaced (see `01_Master_Checklist.md`), so use `CLAUDE.md` and the checklist for the current state.

1. `01_Master_Checklist.md` Phase 0 (setup, prop audit).
2. `02_Architecture_and_Systems.md` in full.
3. `03_Phase1_Cleaning_Implementation.md`.
4. `05_Script_Director_and_Actors.md` Phase 1 section.
5. `04_Phase2_Directing_Implementation.md` in full.
6. `05_Script_Director_and_Actors.md` Phase 2 section.
7. `06_UI_and_Tweening.md`.
8. `08_VFX_and_Shaders.md` and `07_Audio_List.md`.
9. Polish, QA, and packaging sections of the checklist.
