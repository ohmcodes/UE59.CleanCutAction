# Clean! Cut! Action! - Implementation Docs

First-person 3D casual game for a 3-day AI game jam. Engine: **Unreal Engine 5.9.0 Preview**, C++ core with Blueprint subclasses for content.

## The Game in One Paragraph

The player is the Rookie, a production assistant on the last day of shooting *Clean! Cut! Action!*. **Phase 1:** the two-floor cafe movie set was trashed at last night's wrap party; clean it while director Marlowe Finch heckles you through a megaphone. **Phase 2:** on the clean set, two actors (real-life exes Dash and Juniper) shoot their date scene, and the player runs between effect stations across both floors (rain, traffic, thunder, steam, prop tosses, boom mic) on Marlowe's cues while staying out of the camera's frame.

## File Index

| File | Contents |
|---|---|
| `00_README.md` | This file: overview, tech decisions, conventions, agent hand-off order |
| `01_Master_Checklist.md` | Exhaustive task checklist from setup to submission, grouped into phases with priorities |
| `02_Architecture_and_Systems.md` | Project structure, classes, save system, level flow, input, interaction, carry, tween system |
| `03_Phase1_Cleaning_Implementation.md` | Messy map construction, the Messifier tool, every cleaning mechanic, progression, abilities |
| `04_Phase2_Directing_Implementation.md` | Cafe shoot map, cue/beat director, effect stations, frame cone, take meter, retakes, actors |
| `05_Script_Director_and_Actors.md` | All of Marlowe's Phase 1 lines by trigger, the full Phase 2 scene script, cue call/result lines |
| `06_UI_and_Tweening.md` | Main menu, pause, HUDs, both end screens, settings, all animation/tween specs |
| `07_Audio_List.md` | Every SFX and music track to source or make, with implementation notes |
| `08_VFX_and_Shaders.md` | Niagara systems, materials, post-process effects |

## Key Technical Decisions

| Decision | Choice | Why |
|---|---|---|
| Language | C++ for systems, Blueprint subclasses for content/tuning | Text-based code is easiest for an AI agent to write and review |
| Maps | `L_MainMenu`, `L_Phase1_MessySet`, `L_Phase2_CafeShoot` | Two gameplay maps as requested, plus a menu map |
| Map authoring | Build the **clean cafe once** (`L_Phase2_CafeShoot`), duplicate it, then run the **Messifier** editor script to scatter props | Guarantees every prop's "home" in Phase 1 matches the clean set exactly |
| Save | Single `USaveGame` slot, completion flags + unlocks + settings | User request: simple save that knows if Phase 1 is done |
| Phase 1 mid-progress | Not saved | Keeps the save simple; Phase 1 is short (target 15–25 min) |
| Scene logic (Phase 2) | Beat-based state machine driven by a Data Asset, **not** one long Level Sequence | Actors wait for cue results, so failures and branches don't desync |
| Tweens | Small custom C++ tween subsystem + easing library | No plugin dependency, works on actors and widgets, runs while paused |
| UI | Plain UMG (no CommonUI) | Less setup for a 3-day jam |
| Perspective | First person, no visible body (hands/carry stack only) | Fewer animation assets |
| Input | Enhanced Input with separate mapping contexts for gameplay, station, UI | Clean mode switching |

## ⚠️ Preview Engine Warning

UE 5.9.0 is a **preview** build. Tell the agent:

- Verify every API used against the 5.9 preview headers before relying on it; this doc names stable UE5 APIs, but signatures can move between versions.
- Preview projects can't be opened in older engine versions. Commit often.
- If a preview-specific crash blocks progress, note it in `KNOWN_ISSUES.md` and work around it rather than debugging the engine.

## Naming Conventions

| Asset | Prefix | Example |
|---|---|---|
| C++ classes | `LTT` | `ALTTPropBase`, `ULTTSaveGame` |
| Blueprint | `BP_` | `BP_Prop_Chair` |
| Widget | `WBP_` | `WBP_PauseMenu` |
| Data Asset / Table | `DA_` / `DT_` | `DA_Scene42`, `DT_DirectorLines` |
| Material / Instance / Function | `M_` / `MI_` / `MF_` | `M_GhostTapeMark` |
| Niagara | `NS_` | `NS_PlaceSparkle` |
| Sound Wave / MetaSound / Cue | `SW_` / `MS_` / `SC_` | `MS_PlaceChime` |
| Input Action / Context | `IA_` / `IMC_` | `IA_Interact` |
| Map | `L_` | `L_Phase1_MessySet` |

## Content Folder Layout

```
Content/CleanCutAction/
  Core/            (GameMode, GameInstance BP, PlayerCharacter BP)
  Input/
  Maps/
  Props/           (BP_Prop_* children, grouped by category)
  Phase1/          (trash, piles, cables, lights, reels, costumes, lens)
  Phase2/          (stations, film camera, actors, scene data)
  Characters/      (Marlowe, Dash, Juniper meshes, ABPs, montages)
  UI/              (widgets, fonts, textures)
  Audio/           (SFX, Music, VO, SoundClasses, Submixes)
  VFX/             (Niagara systems, emitters)
  Materials/       (master materials, MPCs, functions, post-process)
  Data/            (DataTables, DataAssets)
Scripts/Python/   (Messifier, prop audit)
```

## Hand-off Order for the AI Agent

Give the agent one document section at a time, in this order. After each step, play-test before moving on.

1. `01_Master_Checklist.md` Phase 0 (setup, prop audit). **Review the prop audit output yourself.**
2. `02_Architecture_and_Systems.md` in full (framework, save, input, interaction, tween).
3. `03_Phase1_Cleaning_Implementation.md` §1–2 (clean cafe master + Messifier), then §3 onward (mechanics).
4. `05_Script_Director_and_Actors.md` Phase 1 section → import into `DT_DirectorLines`.
5. `04_Phase2_Directing_Implementation.md` in full.
6. `05_Script_Director_and_Actors.md` Phase 2 section → import into `DA_Scene42`.
7. `06_UI_and_Tweening.md`.
8. `08_VFX_and_Shaders.md` and `07_Audio_List.md`.
9. Polish, QA, and packaging sections of the checklist.

**Suggested agent prompt template:**

> You are implementing a UE 5.9.0 Preview C++ project called CleanCutAction. Read `<section>`. Implement exactly what it specifies. Use existing classes from earlier sections; don't rename them. If an API differs in 5.9, adapt and leave a `// NOTE(5.9):` comment. When done, list every file created/changed and any checklist IDs completed.
