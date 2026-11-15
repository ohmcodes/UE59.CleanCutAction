# Master Implementation Checklist

## Status as of 2026-11-15 (HEAD 87eadcc, branch `windows`)

This checklist was written on 2026-11-12 as the plan. The game that got built is smaller in some areas and bigger in others, so every item below now carries one of three marks:

- `[x]` built (a short note says where, or how it differs from the plan)
- `[ ] ~~item~~ (cut)` dropped or replaced by something else; the note says what replaced it
- `[ ]` still open (some have a "Partial:" note)

**What ships:** Phase 1 is a cleanup on the StylizedNewYorkCity cafe (`/Game/StylizedNewYorkCity/Level/L_NYC_Cafe_Michael`, `GameDefaultMap`). `ALTTMessDirector` throws every prop around at the start (the "earthquake"); the player puts each prop back on its home silhouette, bins trash, washes dishes in the sink and mops puddles, against an optional countdown, while Marlowe heckles through `ALTTCleanupBarker`. The Phase 1 end screen's "Shoot the Scene" button opens Phase 2 (`L_NYC_Cafe_Phase2_Michael`, in `MapsToCook`), where `ALTTSceneDirector` runs the built-in Scene 42 and the player works 11 effect stations while staying out of the film camera's frame. A game is one level each; there is no save game, main menu, settings menu, skills or zones.

**Counts** (the 200 original P0-P13 items): 77 built, 61 cut, 62 open. Another 30 items that were not in the plan are listed at the end under **PX**, all built.

**Notable open items:** `CREDITS.md` and `KNOWN_ISSUES.md` don't exist (P0-03, P0-21, P9-09); no sound classes or volume mix (P1-25 to P1-27); no footsteps, sprint or out-of-bounds respawn (P1-07, P1-09, P1-10, P2-19); no `MPC_World`, so the Phase 2 rain/lightning/wetness material parameters aren't driven (P6-14); actor montages and voice blips (P7-03, P7-04); the Phase 2 final sequence (P7-07); the whole QA pass (P12); a test on a PC without the engine, a size check of the current package, and the jam submission itself (P13).

**Legend for priorities:** **[M]** Must ship · **[S]** Should ship · **[C]** Could ship. Day tags (D1/D2/D3) are from the original 3-day plan.

---

## P0 - Pre-production & Project Setup

### Project
- [x] **P0-01 [M]** C++ project `CleanCutAction` from the First Person template. Template content still used: `Content/FirstPerson/` (character, game mode, arms Anim BP `ABP_FP_Copy`), `Content/Input/` (`IA_Move`, `IA_Look`, `IA_MouseLook`, `IA_Jump`, `IMC_Default`, touch UI).
- [x] **P0-02 [M]** Git repo with `.gitignore` (Binaries, Intermediate, Saved...). There is no `.gitattributes`, so assets aren't tracked with LFS.
- [ ] **P0-03 [M]** `KNOWN_ISSUES.md` for preview-engine problems. Not created; preview workarounds are in `CLAUDE.md` and `Docs/PackagingGuide/` instead.
- [x] **P0-04 [M]** Plugins: ModelingToolsEditorMode, StateTree, GameplayStateTree, ControlRig, FullBodyIK on; `NNEDenoiser` and `AllToolsets` off for packaging (`CleanCutAction.uproject`).
- [ ] ~~**P0-05 [M]** Maps & Modes: default map `L_MainMenu`, `BP_LTTGameInstance`, `BP_LTTGameMode_Menu`.~~ (cut) There is no menu map. `GameDefaultMap` is `L_NYC_Cafe_Michael`, GameInstance is the engine default, game mode is the template `BP_FirstPersonGameMode` (`Config/DefaultEngine.ini`). `EditorStartupMap=None` (3cf7a1d, faster editor start).
- [x] **P0-06 [M]** Custom depth-stencil enabled (`r.CustomDepth=3`); used for the focus outline (stencil 1).
- [x] **P0-07 [M]** Lighting: desktop keeps Lumen (software); `Config/Windows/WindowsEngine.ini` sets TSR. The Wasm build uses the mobile deferred renderer with occlusion queries off (`Platforms/Wasm/Config/WasmEngine.ini`). No measured 60 FPS target.
- [ ] ~~**P0-08 [M]** Collision channels `Interact`, `Prop`, `FrameCheck`.~~ (cut) Only the template `Projectile` channel exists; the frame cone's occlusion test uses `ECC_Visibility`.
- [ ] ~~**P0-09 [M]** Physical Materials `PM_*` for footsteps and impacts.~~ (cut) Impact sounds come from `ULTTPropSoundSet` (`SFX/Props/DA_PropSounds`): each prop picks a sound material (wood, metal, glass, ceramic, paper, plastic, fabric, food).
- [x] **P0-10 [M]** Content folder: `/Game/_CleanCutAction/` (layout in `00_README.md`).
- [ ] ~~**P0-11 [M]** Gameplay Tags file `Config/Tags/LTTTags.ini`.~~ (cut) No gameplay tags are used.

### Prop Audit
- [ ] ~~**P0-12 [M]** `Scripts/Python/prop_audit.py` → `Saved/PropAudit.csv`.~~ (cut)
- [ ] ~~**P0-13 [M]** `DT_PropCatalog` rows.~~ (cut) Each prop is a `BP_Prop_*` Blueprint (111 in `Blueprints/Interactables/Props/`) with its settings on the actor.
- [ ] ~~**P0-14 [M]** `Saved/MissingProps.md`.~~ (cut) Phase 2's missing assets are listed in `Docs/09_Phase2_Asset_Needs.md`.
- [x] **P0-15 [M]** Characters: the Rookie (`Character/Rookie/SKM_Rookie`, seen in the mirror), Dash and Juniper (`ST_Characters_JC` `SKM_StylizedMale` / `SKM_StylizedFemale` in the Phase 2 level). Marlowe has no body; he is a voice in the bark widget.
- [ ] **P0-16 [M]** Animations. Partial: Rookie locomotion and emote (`Character/Rookie/Anims/`), Dash/Juniper locomotion blendspaces (`Characters/BS_*_Locomotion`), a video-to-animation script (`Scripts/Python/VideoToAnim/`). Talk, sit, laugh, catch and similar gestures are not in the project.
- [ ] **P0-17 [S]** Nanite on heavy props; simple collision on every pickup.

### Art & Legal Direction
- [x] **P0-18 [M]** UI style: a "cozy cafe" theme (cream paper panels, gold trim, wooden title banner) in `UI/LTTUITheme.h` instead of the film-production palette. Icons in `/Game/_CleanCutAction/UI/Icons` and `UI/Widget`.
- [ ] **P0-19 [M]** Licensed fonts imported as Font Faces. No font assets in `/Game/_CleanCutAction/`.
- [x] **P0-20 [M]** Jam rules read: `Docs/Submission/Theme_DirectorsCut.md`, `Docs/Submission/NVIDIA_RTX_Award.md` (not committed yet).
- [ ] **P0-21 [M]** `CREDITS.md` logging every third-party asset. Not created; `README.md` has a two-line Credits section.

---

## P1 - Core Framework

### Game Framework Classes
- [ ] ~~**P1-01 [M]** `ULTTGameInstance` + `BP_LTTGameInstance`.~~ (cut) Shared state lives in GameInstance subsystems (`ULTTLeaderboardSubsystem`, `ULTTInputDeviceSubsystem`).
- [ ] ~~**P1-02 [M]** `ULTTSaveGame`.~~ (cut) The only save is `ULTTPlayerProfileSave` (leaderboard player id and name; browser localStorage on Wasm).
- [ ] ~~**P1-03 [M]** `ULTTSaveSubsystem`.~~ (cut) No progress is saved.
- [ ] ~~**P1-04 [M]** `ULTTLevelFlowSubsystem`.~~ (cut) Level changes are direct `OpenLevel` calls: `ALTTMessDirector::LoadNextLevel` (to Phase 2), `ULTTShootEndScreen` (back to Phase 1), and restart from the pause menu and end screens.
- [ ] ~~**P1-05 [M]** Game modes per phase.~~ (cut) Both levels use the template game mode; each level's director actor adds its HUD.
- [x] **P1-06 [M]** Player controller: `ACleanCutActionPlayerController` (pause menu, input modes, touch controls, gamepad device switching, Unstuck).
- [x] **P1-07 [M]** Player character: `ACleanCutActionCharacter` (template first-person character, jump, crouch toggle, gamepad look curve, emote). No sprint.
- [ ] ~~**P1-08 [S]** Head bob.~~ (cut)
- [ ] **P1-09 [M]** Footsteps by surface. Not built.
- [ ] **P1-10 [M]** Kill-Z / out-of-bounds respawn. Partial: the controller's Unstuck puts the player back at their spawn point.

### Input (Enhanced Input)
- [x] **P1-11 [M]** Input actions: template `IA_Move/IA_Look/IA_MouseLook/IA_Jump` plus `IA_Interact`, `IA_Throw`, `IA_Emote` (`/Game/_CleanCutAction/Input`). Crouch, pause and hint make runtime actions from key lists when no asset is set.
- [ ] ~~**P1-12 [M]** `IMC_Gameplay` / `IMC_Station` / `IMC_UI`.~~ (cut) Template `IMC_Default` and `IMC_MouseLook` only; effect stations read keys directly.
- [x] **P1-13 [S]** Gamepad bindings (dae5924, 87eadcc). See PX-15.
- [ ] ~~**P1-14 [M]** Mouse sensitivity / invert Y from settings.~~ (cut) No settings menu.

### Interaction & Carry
- [x] **P1-15 [M]** `ILTTInteractable` (`Interaction/LTTInteractable.h`).
- [x] **P1-16 [M]** `ULTTInteractionComponent`: focus trace, custom-depth outline, interact and throw input.
- [x] **P1-17 [M]** `ULTTCarryComponent`: holds one prop in front of the camera, drop, throw with physics, hand IK targets for the arms. No stack.
- [x] **P1-18 [M]** Crosshair and interact prompt (`WBP_InteractPrompt`, 130b162); key labels follow the input device.

### Tween & Feedback
- [ ] ~~**P1-19 [M]** `ULTTEasing` library.~~ (cut) Easing is done inline in each class.
- [ ] ~~**P1-20 [M]** `ULTTTweenSubsystem`.~~ (cut)
- [ ] ~~**P1-21 [M]** `ULTTFeedbackComponent`.~~ (cut) Pickup/place/break feedback is in `ALTTInteractableBase`.

### Director Barks
- [ ] ~~**P1-22 [M]** `FLTTDirectorLine` + `DT_DirectorLines`.~~ (cut) Phase 1 lines are `LOCTEXT` tables in `Interaction/LTTCleanupBarker.cpp`; Phase 2 lines are the built-in Scene 42 in `Shoot/LTTSceneData.cpp`.
- [x] **P1-23 [M]** Bark logic: `ALTTCleanupBarker` actor (event → line, priorities, cooldowns, no repeats, one queued milestone, interrupts) instead of a world subsystem (b2d8f9b).
- [x] **P1-24 [M]** Subtitle: `ULTTBarkWidget` (Phase 1 gold pill) and the subtitle panel in `ULTTShootHUD` (Phase 2).

### Audio Plumbing
- [ ] **P1-25 [M]** Sound classes and settings mix. Not in the project; music has its own on/off toggle.
- [ ] **P1-26 [M]** Submixes `SMX_Megaphone`, `SMX_Pause`.
- [ ] **P1-27 [M]** Concurrency assets. Partial: `SFX/Props/ATT_Props` attenuation exists, no concurrency assets.

### Debug Tools
- [x] **P1-28 [M]** Console commands, a different set: `LTT.PlaceAll [LeaveCount]`, `LTT.DisableBarks [1|0]` (cb0e7c8), `LTT.SkipCue [Perfect|Good|Late|Miss]`, `LTT.JumpToAct <A|B|C>`, `LTT.Cut`. In-world `ALTTCheatButton` runs `LTT.PlaceAll` (258ebfa). The save/credit/skill/zone commands are cut with their systems.
- [ ] **P1-29 [S]** Debug HUD toggle.

---

## P2 - Cafe Set & Phase 1 Map

The set is the StylizedNewYorkCity cafe, not a soundstage built from scratch. Change logs: `Docs/L_NYC_Cafe_Changes.md`, `Docs/L_NYC_Cafe_Changes_4beed6a.md`, `Docs/Level_Changes_dev-michael.md`.

### Clean set
- [ ] ~~**P2-01 [M]** Soundstage shell (40 × 30 m, catwalks, exit door).~~ (cut) The cafe sits in the `/Game/ParisStreet/Levels/SampleMap` level instance.
- [x] **P2-02 [M]** Two-floor cafe: ground floor with counter and kitchen, staircase, mezzanine (2ec3ffa, 71cb6fd).
- [x] **P2-03 [M]** Stairs widened and collision fixed (2ec3ffa).
- [x] **P2-04 [M]** Street outside the window: ParisStreet `SampleMap` level instance (9ac9571).
- [ ] **P2-05 [M]** Backdrop with Day / Sunset / Night blend. Partial: sunset lighting (97529fa); no translite material.
- [ ] **P2-06 [M]** Crew area. Partial: the Phase 2 level has a BroadcastStudio camera and Hollywood_Vol3 light stands.
- [ ] ~~**P2-07 [M]** Plywood "back sides" of the set.~~ (cut)
- [x] **P2-08 [M]** Every prop is a `BP_Prop_*` child of `BP_MasterInteractables` (111 Blueprints). Homes are recorded at runtime, so no `PropID` is needed.
- [x] **P2-09 [M]** NavMesh in the Phase 2 level (NavMeshBoundsVolume in `L_NYC_Cafe_Phase2_Michael`).
- [x] **P2-10 [M]** Lighting pass (ef31e75, d0e6211, 97529fa).
- [ ] ~~**P2-11 [M]** Map boards per floor.~~ (cut) No zones.

### Messy map
- [ ] ~~**P2-12 [M]** Duplicate the clean master → `L_Phase1_MessySet`.~~ (cut) The level is authored clean and messed up at runtime (PX-01).
- [ ] ~~**P2-13 [M]** `messifier.py`.~~ (cut) Replaced by the runtime earthquake.
- [ ] ~~**P2-14 [M]** Hand-composed "mess stories".~~ (cut)
- [ ] ~~**P2-15 [M]** Trash 40–60, piles, cables, lights, reels, costumes, lens, clapperboard, golden reels.~~ (cut) What exists instead: trash + bins, dishes for the sink, water puddles + mop, breakable props (PX-02 to PX-06).
- [ ] ~~**P2-16 [M]** `ALTTZoneVolume` (7 zones).~~ (cut)
- [ ] ~~**P2-17 [M]** Phase 1 work-light-only lighting.~~ (cut) Lamps and the power box are interactive instead (PX-07, PX-08).
- [ ] **P2-18 [M]** First-view composition at the player start.
- [ ] **P2-19 [M]** Lost-prop respawn when a prop falls out of bounds. Not built.
- [ ] **P2-20 [M]** Blocking volumes against unreachable throws.
- [ ] **P2-21 [S]** Ambient room tone. Background music plays instead (PX-13).

---

## P3 - Phase 1 Mechanics

- [x] **P3-01 [M]** Prop states in `ALTTInteractableBase`: loose → carried → placed (locked), plus broken and trashed.
- [x] **P3-02 [M]** Home marker: `M_GhostSilhouette` silhouette of the prop at its home.
- [x] **P3-03 [M]** Aim at the matching silhouette and click to snap the prop home.
- [x] **P3-04 [M]** Carry: one prop at a time (no stack, no "right it" hold).
- [x] **P3-05 [M]** Dropping anywhere else leaves the prop loose; no penalty for a drop.
- [x] **P3-06 [M]** Throwing with physics; thrown props that slam into something break (PX-02).
- [x] **P3-07 [M]** Trash (`bIsTrash`) goes in a bin (`bIsTrashBin`, `BP_Prop_Bin`) by hand or thrown; Marlowe has a swish line.
- [ ] ~~**P3-08 [M]** Broom + sweepable piles.~~ (cut) Replaced by the mop and water puddles (PX-04).
- [ ] ~~**P3-09 [S]** Cable untangle.~~ (cut)
- [ ] ~~**P3-10 [M]** Stage light aiming.~~ (cut) Lamps switch on/off instead (PX-07).
- [ ] ~~**P3-11 [M]** Film reel shelf.~~ (cut)
- [ ] ~~**P3-12 [S]** Costume rack.~~ (cut)
- [ ] ~~**P3-13 [M]** Camera lens wipe.~~ (cut)
- [ ] ~~**P3-14 [M]** Clapperboard + slate stand.~~ (cut) The Phase 2 Slate station keeps the clapper idea.
- [ ] ~~**P3-15 [M]** Group sets with indicator lamps.~~ (cut)
- [ ] ~~**P3-16 [M]** Zone completion feedback.~~ (cut)

---

## P4 - Phase 1 Progression, Director & Flow

- [x] **P4-01 [M]** Progress in `ALTTMessDirector`: placed/broken counts, score, countdown, `OnAllPlaced` when every cleanup task is done (puddles count, 1f051d1).
- [ ] ~~**P4-02 [M]** Crew Credits + `ULTTSkillComponent`.~~ (cut) Points feed the online leaderboard instead.
- [ ] ~~**P4-03 [M]** Notebook UI.~~ (cut)
- [ ] ~~**P4-04 [M]** Active abilities.~~ (cut)
- [ ] ~~**P4-05 [M]** Passives.~~ (cut)
- [ ] ~~**P4-06 [M]** Golden reels.~~ (cut)
- [ ] ~~**P4-07 [M]** Marlowe actor on a platform.~~ (cut) Marlowe is voice only (bark widget + line sound).
- [x] **P4-08 [M]** Bark triggers: progress, pick up, drop, throw, break, hints, countdown, set clean (`ALTTCleanupBarker`, b2d8f9b).
- [x] **P4-09 [M]** Walkie-talkie story drips at 20/40/60/80% (`LTTCleanupBarker.cpp` §1.18 block).
- [x] **P4-10 [M]** Intro: `ULTTBriefingScreen` (Marlowe's note / how to play / controls, name entry, time-trial or no-timer start) over black while the mess is made, then the earthquake.
- [x] **P4-11 [M]** Completion: ending camera tour with a SKIP button and confetti (`ULTTEndingOverlay`), then the end screen.
- [x] **P4-12 [M]** Results: `ULTTEndScreen` ("SET READY!" / "TIME'S UP!", time, props returned/destroyed, hints, points), run sent to the leaderboard. Nothing is saved locally.
- [ ] ~~**P4-13 [S]** "You're Fired!" ending.~~ (cut)
- [x] **P4-14 [S]** Hints: a hint key (keyboard or gamepad) points the HUD arrow at a prop and counts as a hint used (`PointsPerHint`, default 0); Marlowe comments.

---

## P5 - Phase 2 Map (`L_NYC_Cafe_Phase2_Michael`)

- [x] **P5-01 [M]** Phase 2 level is a separate copy of the cafe without the Phase 1 directors (a4e12f6).
- [x] **P5-02 [M]** Stations placed: Slate, CarRail, DoorBell, Espresso, PropTable1F, PropTable2F, LightRig, Thunder, WindFan, BoomMic, RainRig (`Station_*` actors in the level).
- [x] **P5-03 [M]** `ALTTFilmCamera` with an anchor per act (A, B, C).
- [x] **P5-04 [M]** `ALTTActorMark` actors for the script's marks.
- [ ] **P5-05 [M]** Rain area volume and wet-surface region. Partial: `NS_Rain` is in the level; no wetness region.
- [ ] ~~**P5-06 [M]** Marlowe at the director's chair.~~ (cut) Voice only.
- [ ] **P5-07 [M]** Route check: every station reachable without crossing the frame cone.
- [ ] **P5-08 [S]** Crew NPC extras.

---

## P6 - Phase 2 Mechanics

- [x] **P6-01 [M]** `ULTTSceneData` (`UPrimaryDataAsset`) with line/cue/beat structs. No `DA_Scene42` asset exists; the director falls back to the built-in Scene 42 in C++.
- [x] **P6-02 [M]** `ALTTSceneDirector`: beats, acts with checkpoints, cue windows, grades, score, take meter, events (d64c22d).
- [x] **P6-03 [M]** `ALTTEffectStation`: one class for every station; takes over the mouse while its mini-game runs (E / right mouse leaves).
- [x] **P6-04 [M]** Station mini-games: Press (Slate), Sustain (Traffic, Wind, Boom, Rain), HoldRelease (Steam), Shake (Thunder), Toss (napkins, umbrella, roses).
- [x] **P6-05 [S]** Timing (Door bell) and Choice (Sunset/Moonlight gels); Wind uses Sustain.
- [x] **P6-06 [M]** Prop toss with a flight arc to the actor (`SW_LoudBonk` for misses).
- [x] **P6-07 [M]** Frame cone: view test + occlusion trace (`ECC_Visibility`) + see-through `M_FrameCone` mesh (a7252d6). No floor decal.
- [x] **P6-08 [M]** Take meter, scoring, grades, stars.
- [x] **P6-09 [M]** CUT when the meter is empty; the current act restarts.
- [x] **P6-10 [M]** Cue window grows with travel distance (`TravelSpeed`, `MaxTravelBonus`).
- [ ] ~~**P6-11 [M]** Phase 2 skills.~~ (cut) No skill system.
- [x] **P6-12 [M]** Arrow to the active station with an "upstairs / downstairs" hint (`ULTTShootHUD`, `FloorHeight`).
- [ ] **P6-13 [S]** Director's monitor picture-in-picture.
- [ ] **P6-14 [M]** Weather/light driven through `MPC_World`. Partial: the director drives the scalars if an MPC is assigned, but no `MPC_World` asset exists.

---

## P7 - Actors, Story & Cinematics

- [x] **P7-01 [M]** `ALTTFilmActor` (Dash, Juniper placed in the Phase 2 level) walking between marks.
- [x] **P7-02 [M]** Locomotion: `BS_Dash_Locomotion`, `BS_Juniper_Locomotion` (a4e12f6).
- [ ] **P7-03 [M]** Montages. Partial: `ALTTFilmActor` plays montages by name (`Montages`, `TalkMontages`), but no montage assets are in the project.
- [ ] **P7-04 [M]** Voice blips per character.
- [x] **P7-05 [M]** Scene script in data: built-in Scene 42 in `LTTSceneData.cpp` (no `DA_Scene42` asset).
- [x] **P7-06 [M]** Act title cards in `ULTTShootHUD`.
- [ ] **P7-07 [M]** Final sequence (lightning lean-in, crew cheer, jacket handoff, DIRECTOR'S CUT clapper). `SW_applause` exists; the sequence isn't verified.
- [ ] **P7-08 [S]** Actor reactions on Perfect / Miss. `On Cue Result` Blueprint event exists; no content.
- [ ] **P7-09 [C]** Actor wetness in rain.

---

## P8 - UI

- [ ] ~~**P8-01 [M]** `WBP_MainMenu`.~~ (cut) The game opens on the briefing screen (`WBP_Briefing`).
- [ ] ~~**P8-02 [M]** Locked "Shoot the Scene" until Phase 1 is complete.~~ (cut) "Shoot the Scene" is on the Phase 1 end screen and only shows when the set is clean.
- [ ] ~~**P8-03 [M]** `WBP_Settings`.~~ (cut)
- [x] **P8-04 [M]** Pause menu `ULTTPauseMenu` / `WBP_PauseMenu`: Resume, Music on/off, Leaderboard, Restart, Quit (P / Esc / gamepad Start).
- [ ] ~~**P8-05 [M]** `WBP_ConfirmDialog`.~~ (cut)
- [x] **P8-06 [M]** Phase 1 HUD `ULTTCleanupHUD` / `WBP_CleanupHUD`: placed, destroyed, points, countdown, hint, arrow to the held prop's home.
- [ ] ~~**P8-07 [M]** `WBP_Notebook`.~~ (cut)
- [x] **P8-08 [M]** Phase 1 end screen `ULTTEndScreen` / `WBP_EndScreen` (Play Again, Keep Playing, Shoot the Scene, Leaderboard).
- [x] **P8-09 [M]** Phase 2 HUD `ULTTShootHUD` / `WBP_ShootHUD`: REC + timecode, take, score, take meter, cue card, next cue, in-shot warning, subtitles, act cards, station meter (a9d49ee).
- [x] **P8-10 [M]** CUT interstitial (part of `ULTTShootHUD`).
- [ ] **P8-11 [M]** Phase 2 end screen and credits. Partial: `ULTTShootEndScreen` / `WBP_ShootEndScreen` ("THAT'S A WRAP!", stars, Shoot Again, Back to Cleanup); no credits screen.
- [ ] **P8-12 [M]** Fade / loading cover between levels.
- [ ] **P8-13 [M]** Button/panel tweens per `06 §2`. Partial: widgets pop in/out in their own code.
- [x] **P8-14 [S]** Gamepad menu focus: `ULTTInputDeviceSubsystem` focus helpers and B = back.
- [x] **P8-15 [M]** Pause works in both levels (the player controller owns it).

---

## P9 - Audio

- [ ] **P9-01 [M]** Every [M] SFX in `07`. Partial: prop pick-up/place/break per material, station SFX (`SFX/stations/`, 258ca78), coffee, microwave, sink, power, mop and puddle sounds, applause/cut/success/miss.
- [x] **P9-02 [M]** Music: `ALTTMusicPlayer` shuffles six `MusicFX` tracks with crossfades; emote track `EmoteMusic/paroparo`. No layered stems or per-act tracks.
- [ ] ~~**P9-03 [M]** Music layers by zone.~~ (cut) No zones.
- [ ] **P9-04 [M]** Place-chime pitch ladder.
- [x] **P9-05 [M]** Marlowe: text barks with a line sound and a walkie sound (`ALTTCleanupBarker` `LineSound`, `WalkieSound`). No recorded VO.
- [x] **P9-06 [M]** Prop attenuation `SFX/Props/ATT_Props`.
- [ ] **P9-07 [M]** Pause low-pass and music duck on barks.
- [ ] ~~**P9-08 [M]** Volume settings saved.~~ (cut) Music on/off only (pause menu and `ULTTMusicToggle`).
- [ ] **P9-09 [M]** Log every audio source in `CREDITS.md`.

---

## P10 - VFX & Shaders

- [x] **P10-01 [M]** Home marker material `M_GhostSilhouette`.
- [x] **P10-02 [M]** Outline post-process `Materials/PostProcess/M_PP_Outline`.
- [ ] **P10-03 [M]** Place sparkle / wrong puff. Breaks use the shared `GC_Generic_Shatter` and `M_BrokenProp`.
- [ ] ~~**P10-04 [M]** `NS_ZoneComplete`.~~ (cut) Confetti is drawn by `ULTTEndingOverlay`.
- [ ] ~~**P10-05 [M]** Dust / sweep / swish Niagara.~~ (cut) `NS_MopSplash` and `NS_SinkBubbles` instead.
- [ ] **P10-06 [M]** Rain + wetness. Partial: `NS_Rain`; no `MF_Wetness`.
- [ ] **P10-07 [M]** Station VFX. Partial: `NS_Steam`, `NS_Wind`, `NS_Sparks` (a7252d6); no thunder flash or rose petals.
- [x] **P10-08 [M]** `M_FrameCone` (no floor decal).
- [ ] **P10-09 [M]** `PP_Cinematic`.
- [ ] **P10-10 [M]** Backdrop material. `M_LensSmudge` is cut with the lens task.
- [ ] ~~**P10-11 [S]** Ability VFX.~~ (cut)
- [ ] **P10-12 [S]** Star stamp effect. Golden reel shimmer is cut.
- [ ] **P10-13 [C]** Rain droplets on screen.

---

## P11 - Juice & Polish

- [x] **P11-01 [M]** Pickup sound per material.
- [ ] **P11-02 [M]** Placement snap + squash + sparkle + chime ladder. Partial: place sound per material.
- [ ] ~~**P11-03 [M]** Group/zone completion juice.~~ (cut)
- [ ] **P11-04 [M]** Button hover/press feedback and sounds (`SW_UiHover`, `SW_UiClick` exist).
- [ ] **P11-05 [M]** Panel open/close animation.
- [ ] **P11-06 [M]** Counters roll up; stars stamp in.
- [ ] **P11-07 [S]** Camera shake. Partial: `EarthquakeShake` on `ALTTMessDirector`.
- [ ] **P11-08 [S]** Carried prop sway.
- [x] **P11-09 [M]** Bark pops in and fades out (`ULTTBarkWidget`).
- [x] **P11-10 [M]** No silence: shuffled music in both levels, idle barks.

---

## P12 - QA

- [ ] **P12-01 [M]** Full playthrough: Phase 1 → end → Phase 2 → end → back to Phase 1.
- [ ] ~~**P12-02 [M]** Quit mid-Phase 1, Phase 2 still locked.~~ (cut) No save.
- [ ] ~~**P12-03 [M]** Continue to Phase 2 after relaunch.~~ (cut) No save.
- [ ] ~~**P12-04 [M]** New Game keeps settings.~~ (cut) No save or settings.
- [ ] **P12-05 [M]** Every prop can reach its home. `LTT.ValidateProps` doesn't exist; `LTT.PlaceAll` tests the ending.
- [ ] **P12-06 [M]** 100% reachable (every cleanup task completes, including puddles).
- [ ] **P12-07 [M]** Props thrown out of bounds respawn (depends on P2-19).
- [ ] **P12-08 [M]** Phase 2: every cue can be Perfect/Good/Late/Miss without soft-lock; retake from each act.
- [ ] **P12-09 [M]** Pause during carry, station mode, cue window, bark, CUT, end screen.
- [ ] ~~**P12-10 [M]** Settings persist.~~ (cut)
- [ ] **P12-11 [M]** Frame rate check (Wasm and Windows).
- [ ] **P12-12 [M]** Subtitles readable at 1280×720 and 4K.
- [ ] **P12-13 [S]** First-time-player test.
- [ ] **P12-14 [M]** No debug draw or print strings in Shipping; decide whether `ALTTCheatButton` stays in the levels.

---

## P13 - Build & Submission

- [x] **P13-01 [M]** Packaging: Shipping, `GameDefaultMap` `L_NYC_Cafe_Michael`, `MapsToCook` with `L_NYC_Cafe_Phase2_Michael`, the ParisStreet `SampleMap` and path-loaded assets (`Config/DefaultGame.ini`). Targets: Wasm (itch.io) and Windows. See PX-28 to PX-30.
- [x] **P13-02 [M]** Name, company, copyright notice (`Config/DefaultGame.ini`), icon (`game logo icon.ico`), splash (`SplashImage` in `DefaultEngine.ini`, c228ed8).
- [ ] **P13-03 [M]** Run the package on a PC without the engine.
- [ ] **P13-04 [M]** Size check: `pakchunk0-Wasm.ucas` under itch.io's 200 MB limit for the current build.
- [ ] **P13-05 [M]** Jam page. Partial: copy in `Docs/Itch_Page.md` (written for Phase 1 only), art in `itch/`. Screenshots, GIF and AI-use disclosure still to check.
- [ ] **P13-06 [S]** Trailer clip.
- [ ] **P13-07 [M]** Submit before the deadline.

---

## PX - Built but not in the original plan

Dates are commit dates from `git log`.

### Phase 1 gameplay
- [x] **PX-01** Runtime mess: `ALTTMessDirector` records each prop's home and what it rests on, then throws everything around with a camera shake (the "earthquake").
- [x] **PX-02** Breakable props: `Destruction/GC_*` geometry collections and a shared `BreakFX/GC_Generic_Shatter` + `M_BrokenProp`; broken props count against the score.
- [x] **PX-03** Dishwashing: `ULTTWashStationComponent` on `BP_Sink`; dishes shrink into the basin with bubbles (`NS_SinkBubbles`) (1b32aa7, 2026-11-14).
- [x] **PX-04** Water puddles + mop: `ALTTWaterPuddle` / `BP_WaterPuddle` slow the player and are cleaned with `BP_Prop_Mop`; puddles count as cleanup tasks (2621808, 1f051d1, 89e1d3e, 2026-11-14/15).
- [x] **PX-05** Usable appliances: `ULTTCoffeeMachineComponent` (espresso routine), `ULTTApplianceComponent` (microwave) on `ULTTUsableComponent` (0949e65, 2026-11-14).
- [x] **PX-06** Time trial or no-timer mode, chosen on the briefing screen.
- [x] **PX-07** Switchable lamps `ALTTLightFixture` (`BP_Lamp_*`), lamp switches, LED sign flicker `ULTTFlickerComponent` (`BP_ShopSign_02_LED`).
- [x] **PX-08** Power breaker: `ALTTPowerBox` (`BP_ElectricBox`) + `ULTTPowerSubsystem` cut and restore every local light (0949e65).
- [x] **PX-09** Prop sounds by material: `ULTTPropSoundSet` (`SFX/Props/DA_PropSounds`).
- [x] **PX-10** Crouch toggle with HUD prompt (0d9f11c, 2026-11-13).

### Player and world
- [x] **PX-11** Rookie character mesh and first-person arms with hand IK on the held prop (`ULTTFirstPersonAnimInstance`, 7235963, 40d4d4b).
- [x] **PX-12** Emote / dance: `IA_Emote`, `A_Emote_Rookie`, music `EmoteMusic/paroparo` (2621808); video-to-animation tool in `Scripts/Python/VideoToAnim/`.
- [x] **PX-13** Background music `ALTTMusicPlayer` (shuffle + crossfade) with an on/off button in the pause menu and `ULTTMusicToggle` on the touch HUD.
- [x] **PX-14** Mirror `ALTTMirror` (scene capture, screen-space UVs, `Materials/Mirror/`) so the player sees the Rookie (bf0bc41).

### Input and devices
- [x] **PX-15** Gamepad support: right-stick look curve, gamepad hint/pause keys, device tracking in `ULTTInputDeviceSubsystem`, prompts and legends that switch between keyboard and gamepad labels (dae5924 2026-11-14, 87eadcc 2026-11-15).
- [x] **PX-16** Touch controls (`UI_TouchSimple`), hidden while a gamepad is in use.
- [x] **PX-17** On-screen keyboard for touch: `ULTTVirtualKeyboard` / `WBP_VirtualKeyboard` (the Wasm build has no platform keyboard) (602aab3).
- [x] **PX-18** Unstuck: teleports the player back to spawn.

### Online and UI
- [x] **PX-19** Online leaderboard: `ULTTLeaderboardSubsystem` (browser `fetch()` on Wasm, `FHttpModule` elsewhere; server at cca.ohmcodes.com), player name on the briefing screen, `ULTTLeaderboardPanel` / `WBP_Leaderboard` top 10 (656297a, 2026-11-13).
- [x] **PX-20** Briefing screen: three-page carousel (arrows, d-pad or swipe) (602aab3, 31f6b5d).
- [x] **PX-21** Ending overlay: SKIP for the camera tour and confetti that keeps falling over the paused end screen (`ULTTEndingOverlay`).
- [x] **PX-22** Shared UI theme `LTTUITheme.h` and the editor tool `ULTTWidgetBaker` that copies a C++ layout into a Widget Blueprint.
- [x] **PX-23** Dev cheats: `ALTTCheatButton`, `LTT.PlaceAll`, `LTT.DisableBarks`.

### Phase 2 (`Source/CleanCutAction/Shoot/`)
- [x] **PX-24** Classes: `ALTTSceneDirector`, `ULTTSceneData`, `ALTTEffectStation`, `ALTTFilmCamera`, `ALTTFilmActor` + `ALTTActorMark`, `ULTTShootHUD`, `ULTTShootEndScreen` (d64c22d, 2026-11-14).
- [x] **PX-25** Phase 2 level set up with stations, actors, marks and NavMesh (a4e12f6); rain/wind/steam VFX and see-through cone (a7252d6); station SFX (258ca78); shoot UI moved to Blueprints (a9d49ee).
- [x] **PX-26** Phase 2 asset wish list: `Docs/09_Phase2_Asset_Needs.md`.

### Packaging (see `CLAUDE.md` and `Docs/PackagingGuide/`)
- [x] **PX-27** Copyright and ownership: QLE OPC notice in `COPYRIGHT.md`, `LICENSE`, `Config/DefaultGame.ini` (b455792, a928041).
- [x] **PX-28** Web build: Wasm textures capped at 1024 (`Platforms/Wasm/Config/WasmDeviceProfiles.ini`), occlusion queries off (`WasmEngine.ini`, c42505f), empty editor startup map (3cf7a1d), local test server `Scripts/serve_wasm.py` (c50b596), itch.io settings and art (`itch/`, 4c00772).
- [x] **PX-29** Cook fixes: `NNEDenoiser` and `AllToolsets` disabled in the `.uproject`, `PackageCompressionLevel_TestShipping=7`, `bSkipEditorContent=False`, "Back to Cleanup" points at the cooked `L_NYC_Cafe_Michael` (`LTTSceneDirector.h`), path-loaded assets in `MapsToCook` (c4fdfa7, b426e44, d8d64ed).
- [x] **PX-30** Windows build: `Config/Windows/WindowsEngine.ini` (TSR) and the packaging guide `Docs/PackagingGuide/` (d0bb3e9). `Config/Windows/` is not committed yet.
