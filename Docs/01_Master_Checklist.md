# Master Implementation Checklist

**Priority:** **[M]** Must ship · **[S]** Should ship · **[C]** Could ship (cut first)
**Day:** suggested jam day (D1/D2/D3)
IDs are referenced from the other docs. Tick items off as you go.

---

## Schedule at a Glance

| Day | Morning | Afternoon | Evening |
|---|---|---|---|
| D1 | P0 Setup, P1 Framework | P2 Cafe blockout, Messifier | P3 Phase 1 core mechanics |
| D2 | P4 Phase 1 progression, barks | P5 Phase 2 map, P6 Cue system + stations | P7 Actors + scene script |
| D3 | P8 UI complete, P9 Audio | P10 VFX/shaders, P11 Juice | P12 QA, P13 Build + submit |

**Cut line:** if you're behind at the end of D2, cut every **[C]** item and the **[S]** items marked ✂.

---

## P0 - Pre-production & Project Setup (D1)

### Project
- [ ] **P0-01 [M]** Create UE 5.9.0 Preview **C++** project `CleanCutAction` (First Person template, then strip template content you don't need).
- [ ] **P0-02 [M]** Initialize Git + Git LFS. `.gitattributes` tracks `*.uasset *.umap *.wav *.png *.fbx *.exr` with LFS. `.gitignore`: `Binaries/ Intermediate/ Saved/ DerivedDataCache/ .vs/ *.sln` (keep `.uproject`).
- [ ] **P0-03 [M]** Create `KNOWN_ISSUES.md` for preview-engine problems.
- [ ] **P0-04 [M]** Enable plugins: Enhanced Input, Niagara, Python Editor Script Plugin, Editor Scripting Utilities, Modeling Tools Editor Mode (for blockout), MetaSound. Disable unused ones (VR, online subsystems) to speed up startup.
- [ ] **P0-05 [M]** Project Settings → Maps & Modes: Editor/Game default map `L_MainMenu`, GameInstance `BP_LTTGameInstance`, default GameMode `BP_LTTGameMode_Menu`.
- [ ] **P0-06 [M]** Project Settings → Rendering: Custom Depth-Stencil Pass = **Enabled with Stencil** (needed for outlines and Director's Eye).
- [ ] **P0-07 [M]** Decide lighting: Lumen GI + Reflections on (default), with a Low scalability fallback. Set target 60 FPS on mid-range GPU.
- [ ] **P0-08 [M]** Collision: add trace channel `Interact` (default Ignore; props/stations Block), object channel `Prop`, trace channel `FrameCheck` (for camera occlusion test).
- [ ] **P0-09 [M]** Physical Materials: `PM_Wood`, `PM_Tile`, `PM_Carpet`, `PM_Metal`, `PM_Fabric`, `PM_Ceramic`, `PM_Paper`, `PM_Plastic` (footsteps + impact sounds).
- [ ] **P0-10 [M]** Create Content folder layout from `00_README.md`.
- [ ] **P0-11 [M]** Gameplay Tags file `Config/Tags/LTTTags.ini` with the tag list from `02_Architecture_and_Systems.md §8`.

### Prop Audit (do before any level work)
- [ ] **P0-12 [M]** Run `Scripts/Python/prop_audit.py`: list every StaticMesh/SkeletalMesh in the project with path, bounds (cm), triangle count, material count. Output `Saved/PropAudit.csv`.
- [ ] **P0-13 [M]** Classify each mesh into `DT_PropCatalog` rows: `PropType` tag, `SizeClass` (Small/Medium/Large/Fixed), `PhysMaterial`, `Zone hint`, `Use` (Placeable / Trash / Decor / Station part).
- [ ] **P0-14 [M]** Produce `Saved/MissingProps.md`: props this design needs that aren't in the project (see `03 §1.4`). Choose substitutes or simple modeled replacements.
- [ ] **P0-15 [M]** Check characters: meshes for Marlowe, Dash, Juniper. If missing, use UE5 Manny/Quinn with material color swaps + simple hat/scarf attachments.
- [ ] **P0-16 [M]** Check animations: locomotion, idle, talk gestures, sit, laugh, angry, catch, point. Retarget to UE5 skeleton with IK Retargeter if needed. List gaps.
- [ ] **P0-17 [S]** Generate Nanite on static props with >5k tris; enable collision (simple convex) on every prop that will be picked up.

### Art & Legal Direction
- [ ] **P0-18 [M]** UI style sheet: film-production theme (clapperboard black/white stripes, call-sheet paper, yellow tape-mark, red REC). Palette with hex codes in `06 §1`.
- [ ] **P0-19 [M]** Fonts with game-use licenses (e.g. OFL fonts such as Bebas Neue for headings, Courier Prime for body). Import as Font Faces.
- [ ] **P0-20 [M]** Read the jam rules: AI-generated asset rules, required credits, submission format, deadline timezone.
- [ ] **P0-21 [M]** Start `CREDITS.md` and log every third-party asset + license as it's added.

---

## P1 - Core Framework (D1)

### Game Framework Classes
- [ ] **P1-01 [M]** `ULTTGameInstance` + `BP_LTTGameInstance`.
- [ ] **P1-02 [M]** `ULTTSaveGame` with schema in `02 §3`.
- [ ] **P1-03 [M]** `ULTTSaveSubsystem` (GameInstanceSubsystem): Load on init, Save, NewGame reset (keeps settings), `IsPhase1Complete()`.
- [ ] **P1-04 [M]** `ULTTLevelFlowSubsystem`: `GoToMainMenu()`, `GoToPhase1()`, `GoToPhase2()`, fade out → OpenLevel → fade in. Guards against double calls.
- [ ] **P1-05 [M]** Game modes: `ALTTGameMode_Menu`, `ALTTGameMode_Phase1`, `ALTTGameMode_Phase2`; each sets HUD widget + player class.
- [ ] **P1-06 [M]** `ALTTPlayerController`: input mode switching, pause handling, widget ownership.
- [ ] **P1-07 [M]** `ALTTPlayerCharacter`: first-person camera at 165 cm eye height, walk 450 cm/s, sprint 750 (gated), jump (enabled), air control 0.3, FOV from settings.
- [ ] **P1-08 [S]** Subtle head bob (amplitude 1.2 cm, scales with speed) with toggle in settings.
- [ ] **P1-09 [M]** Footsteps by Physical Material, distance-based (every 180 cm walked, 220 sprinting).
- [ ] **P1-10 [M]** Kill-Z + out-of-bounds respawn for player (return to last grounded position).

### Input (Enhanced Input)
- [ ] **P1-11 [M]** Input Actions: `IA_Move, IA_Look, IA_Jump, IA_Sprint, IA_Primary (LMB), IA_Secondary (RMB), IA_Interact (E), IA_Ability1-3 (1,2,3), IA_Notebook (Tab), IA_Pause (Esc), IA_StationExit (E/RMB), IA_StationAxis (mouse/WASD), IA_StationAction (LMB), IA_StationAlt (F), IA_GelSelect (1-3/scroll)`.
- [ ] **P1-12 [M]** Contexts: `IMC_Gameplay` (priority 0), `IMC_Station` (priority 1), `IMC_UI` (priority 2).
- [ ] **P1-13 [S]** Gamepad bindings in the same contexts ✂.
- [ ] **P1-14 [M]** Mouse sensitivity + invert Y applied from settings via a Look modifier.

### Interaction & Carry
- [ ] **P1-15 [M]** `ILTTInteractable` interface (`02 §5`).
- [ ] **P1-16 [M]** `ULTTInteractionComponent`: camera trace 300 cm on `Interact` channel, focus change events, custom-depth outline on focus, hold-to-interact progress.
- [ ] **P1-17 [M]** `ULTTCarryComponent`: stack (capacity from skills), attach to `HandStack` scene component, stack layout, drop, throw (charge 0–1 s), carry weight class rules.
- [ ] **P1-18 [M]** Crosshair + prompt widget responds to focus (see `06`).

### Tween & Feedback
- [ ] **P1-19 [M]** `ULTTEasing` function library (Linear, InOutSine, OutCubic, OutBack, OutElastic, OutBounce).
- [ ] **P1-20 [M]** `ULTTTweenSubsystem` (tickable, works while paused for UI tweens): tween scale/location/rotation of SceneComponents, float lambdas, widget render transform/opacity, cancel by owner, completion callbacks.
- [ ] **P1-21 [M]** `ULTTFeedbackComponent`: `PlayPop()`, `PlaySquash()`, `PlayWobble()`, `PlayPlaceFeedback()` (tween + SFX + Niagara in one call).

### Director Barks (shared)
- [ ] **P1-22 [M]** `FLTTDirectorLine` row struct + `DT_DirectorLines` (columns in `05 §0`).
- [ ] **P1-23 [M]** `ULTTBarkSubsystem` (WorldSubsystem): event → line selection, priorities, cooldowns, no-repeat, queue of 1, subtitles, VO/blip playback.
- [ ] **P1-24 [M]** `WBP_Subtitle` with speaker name colors.

### Audio Plumbing
- [ ] **P1-25 [M]** Sound Classes: Master → Music, SFX, Voice, UI, Ambience. Sound Mix `SM_Settings` for volume sliders.
- [ ] **P1-26 [M]** Submixes: `SMX_Megaphone` (band-pass 500–3500 Hz + light distortion), `SMX_Pause` (low-pass 800 Hz when paused).
- [ ] **P1-27 [M]** Concurrency: `CON_Bark` (max 1, stop oldest), `CON_PropImpact` (max 6), `CON_Footstep` (max 2).

### Debug Tools
- [ ] **P1-28 [M]** Console commands: `LTT.CompleteZone <Name>`, `LTT.CompletePhase1`, `LTT.GiveCredits <N>`, `LTT.UnlockAllSkills`, `LTT.SkipCue`, `LTT.JumpToAct <A|B|C>`, `LTT.ResetSave`, `LTT.ShowFrameCone 0/1`.
- [ ] **P1-29 [S]** Debug HUD toggle (F10): progress numbers, focused actor, current beat/cue, take meter value.

---

## P2 - Cafe Set & Phase 1 Map (D1)

### Clean Master (becomes `L_Phase2_CafeShoot`)
- [ ] **P2-01 [M]** Soundstage shell: 40 × 30 m floor, 12 m walls, catwalk silhouettes, exit door (locked prop), work lights.
- [ ] **P2-02 [M]** Cafe set blockout (layout in `03 §1.2`): ground floor (entrance, counter, espresso bar, seating, back kitchen), **two staircases** (front stair + backstage crew stair), upper floor (mezzanine seating + open terrace with railing overlooking the street flat).
- [ ] **P2-03 [M]** Stair specs: width ≥ 180 cm, step height ≤ 18 cm, collision ramp (smooth first-person movement), railings.
- [ ] **P2-04 [M]** Street flat: painted façade + sidewalk strip outside the front window, cardboard car rail along it.
- [ ] **P2-05 [M]** Exterior backdrop translite (large emissive plane behind street flat) with Day / Sunset / Night material blend (`08 §2.6`).
- [ ] **P2-06 [M]** Crew area: director's chair + platform, video monitor, film camera on dolly, craft services table, costume rack, reel shelf, cable runs, 4 stage lights on stands.
- [ ] **P2-07 [M]** Set "back sides" visible from crew area (raw plywood, braces, "SET - DO NOT LEAN" stencil). It's a joke and a theme reinforcement.
- [ ] **P2-08 [M]** Every prop placed in its home position as `BP_Prop_*` with a unique `PropID` (`03 §2.1`).
- [ ] **P2-09 [M]** NavMesh covering both floors and both staircases (needed in Phase 2 for actors).
- [ ] **P2-10 [M]** Lighting pass for the clean set (warm practical lamps + stage lights).
- [ ] **P2-11 [M]** Two in-world **map boards** (one per floor) showing zones (`03 §1.3`).

### Messy Map (`L_Phase1_MessySet`)
- [ ] **P2-12 [M]** Duplicate the clean master → `L_Phase1_MessySet`.
- [ ] **P2-13 [M]** Run `messifier.py --record-homes` then `--scatter` (`03 §2`).
- [ ] **P2-14 [M]** Hand-compose "mess stories" (`03 §2.4`) on top of the scatter.
- [ ] **P2-15 [M]** Add trash (target 40–60 items), popcorn/confetti piles (8–12), tangled cables (4), mis-aimed lights (4), scrambled reels (5), costumes on the floor (6), smudged lens (1), hidden clapperboard (1), golden reels (3).
- [ ] **P2-16 [M]** Place zone volumes `ALTTZoneVolume` (7 zones) and assign tasks.
- [ ] **P2-17 [M]** Phase 1 lighting: harsh work lights only; stage lights off (they turn on per zone).
- [ ] **P2-18 [M]** Player start + first-view composition: one easy chair beside its ghost mark, golden reel #1 visible, Marlowe visible on his platform.
- [ ] **P2-19 [M]** Prop safety: kill volume + "lost prop" respawn to its messy start position if it falls out of bounds or under the floor.
- [ ] **P2-20 [M]** Blocking volumes so props can't be thrown into unreachable spots (rafters, behind flats).
- [ ] **P2-21 [S]** Ambient walla/room-tone audio volumes.

---

## P3 - Phase 1 Mechanics (D1–D2)

- [ ] **P3-01 [M]** `ALTTPropBase` states: Messy → Carried → Placed(Locked) and Messy → Dropped (`03 §3.1`).
- [ ] **P3-02 [M]** Ghost tape-mark actor at each home (`M_GhostTapeMark`), hidden until a matching prop is carried.
- [ ] **P3-03 [M]** Snap detection: aim point within snap radius of the matching ghost + line of sight → ghost highlights; click → snap tween.
- [ ] **P3-04 [M]** Carry stack (Small props), single carry (Medium), and Large/Fixed "Right it" hold interaction (tipped tables, sofa) (`03 §3.2`).
- [ ] **P3-05 [M]** Wrong placement: drop anywhere else = Dropped state (soft red flash, no penalty).
- [ ] **P3-06 [M]** Throwing: charge, arc, impact sounds by phys material, knock-on physics for loose props only (placed props are kinematic).
- [ ] **P3-07 [M]** Trash items + bins (direct deposit + throw-in detection with "swish" bonus) (`03 §3.4`).
- [ ] **P3-08 [M]** Broom tool + sweepable piles (instanced kernels removed in radius) (`03 §3.5`).
- [ ] **P3-09 [S]** Cable untangle (spline straighten + plug-in) (`03 §3.6`).
- [ ] **P3-10 [M]** Stage light aiming (hold + mouse rotate to target) (`03 §3.7`).
- [ ] **P3-11 [M]** Film reel shelf (ordered slots 1–5) (`03 §3.8`).
- [ ] **P3-12 [S]** Costume rack (color-coded hangers) ✂ (`03 §3.9`).
- [ ] **P3-13 [M]** Camera lens wipe (hold, smudge param 1→0) (`03 §3.10`).
- [ ] **P3-14 [M]** Clapperboard hidden item + slate stand placement (`03 §3.11`).
- [ ] **P3-15 [M]** Group sets (e.g. "Table 3 setting") with indicator lamps → **tier-2 feedback** (`03 §4.1`).
- [ ] **P3-16 [M]** Zones → **tier-3 feedback** (stage light on, music layer, toast, credits) (`03 §4.2`).

---

## P4 - Phase 1 Progression, Director & Flow (D2)

- [ ] **P4-01 [M]** `ULTTCleanProgressSubsystem`: totals, %, group/zone events, completion check.
- [ ] **P4-02 [M]** Crew Credits economy + `ULTTSkillComponent` (`03 §5`).
- [ ] **P4-03 [M]** Notebook UI: Skills page + Map page (`06 §6`).
- [ ] **P4-04 [M]** Active abilities: Director's Eye, Grab Bag, Set Dresser's Touch, with duration + cooldown + upgrades (`03 §5.3`).
- [ ] **P4-05 [M]** Passives: Comfy Sneakers, Stack 'Em, Wide Broom.
- [ ] **P4-06 [M]** Golden reels (3) → Sprint, Carry +1, Cooldown −15% (`03 §5.4`).
- [ ] **P4-07 [M]** Marlowe actor on platform: idle, look-at player, megaphone raise montage when speaking.
- [ ] **P4-08 [M]** Hook every Phase 1 bark trigger in `05 §1` to gameplay events.
- [ ] **P4-09 [M]** Walkie-talkie story drips at 20/40/60/80% (`05 §1.18`).
- [ ] **P4-10 [M]** Intro sequence: fade in, Marlowe intro lines, objective note pickup, first-chair tutorial (`03 §6.1`).
- [ ] **P4-11 [M]** Completion sequence: all zones done + clapperboard placed → lights up, "Places, everyone!" → end screen (`03 §6.3`).
- [ ] **P4-12 [M]** Phase 1 results: time, stars, stats → save `bPhase1Complete`, best time, best stars.
- [ ] **P4-13 [S]** "You're Fired!" joke ending: player walks out the exit door after 2+ minutes with 0 zones complete and confirms ✂.
- [ ] **P4-14 [S]** Hint system: after 90 s with no progress, nearest incomplete task's ghost pulses + Marlowe hint line.

---

## P5 - Phase 2 Map (D2)

- [ ] **P5-01 [M]** Open `L_Phase2_CafeShoot` (clean master). Remove Phase 1-only actors (trash, piles, golden reels).
- [ ] **P5-02 [M]** Place effect stations (`04 §3`): 1F Street Car Rail, Door Bell, Espresso Steam, Prop Table 1F; 2F Light Rig (gels), Thunder Sheet + Flash, Wind Fan, Boom Mic, Rain Rig, Prop Table 2F. Slate stand by camera.
- [ ] **P5-03 [M]** Film camera actor `ALTTFilmCamera` with 3 camera positions (Act A, B, C) + crane/dolly move between them.
- [ ] **P5-04 [M]** Actor marks (`ALTTActorMark`) for every beat.
- [ ] **P5-05 [M]** Rain area volume over terrace + wet-surface region.
- [ ] **P5-06 [M]** Marlowe at director's chair with monitor.
- [ ] **P5-07 [M]** Check that every station is reachable **without** crossing the active frame cone, and that some routes tempt the player through it.
- [ ] **P5-08 [S]** Crew NPC extras (static posed meshes) for life.

---

## P6 - Phase 2 Mechanics (D2)

- [ ] **P6-01 [M]** `ULTTSceneData` (DataAsset) + `FLTTBeat` / `FLTTCueSpec` structs (`04 §2`).
- [ ] **P6-02 [M]** `ALTTSceneDirector`: beat state machine, acts, checkpoints, cue windows, grading, events.
- [ ] **P6-03 [M]** `ALTTEffectStationBase` + station mode input context and camera behavior (`04 §3.1`).
- [ ] **P6-04 [M]** Stations: Slate (tutorial), Traffic Rail, Espresso Steam, Toss (napkins/umbrella/roses), Thunder, Rain, Boom Mic.
- [ ] **P6-05 [S]** Stations: Door Bell, Gel Swap (Sunset/Moonlight), Wind Fan.
- [ ] **P6-06 [M]** Prop toss system with arc preview, catch zone on actor, bonk on miss (`04 §3.6`).
- [ ] **P6-07 [M]** Frame cone: frustum test + occlusion trace + visual frustum mesh + floor decal (`04 §4`).
- [ ] **P6-08 [M]** Take meter + scoring + grade thresholds (`04 §5`).
- [ ] **P6-09 [M]** CUT/retake: meter hits 0 → CUT interstitial → restart current act checkpoint (`04 §5.3`).
- [ ] **P6-10 [M]** Cue window timing scales with travel distance (`04 §2.4`).
- [ ] **P6-11 [M]** Phase 2 skills: Cue Sense, Steady Hands, Long Throw, Director's Patience, Dodge the Shot.
- [ ] **P6-12 [M]** Off-screen cue arrow + floor badge (1F/2F).
- [ ] **P6-13 [S]** Director's Monitor picture-in-picture (SceneCapture2D, 15 fps) ✂.
- [ ] **P6-14 [M]** Weather/lighting state driven by cues via `MPC_World` (Wetness, RainIntensity, LightningFlash, BackdropTime).

---

## P7 - Actors, Story & Cinematics (D2–D3)

- [ ] **P7-01 [M]** `ALTTFilmActor` (Dash, Juniper) + AIController MoveTo marks across floors.
- [ ] **P7-02 [M]** Anim BP: locomotion blendspace, sit/stand, montage slot for gestures.
- [ ] **P7-03 [M]** Montages: Talk (×3), Laugh, Angry, Storm Off, Catch, Open Umbrella, Toss Away, Offer Roses, Lean In, Startled (for bonks/fails), Wave at camera (fail).
- [ ] **P7-04 [M]** Voice blips per character (pitch per speaker) synced to subtitle word count.
- [ ] **P7-05 [M]** Import full scene script from `05 §2` into `DA_Scene42`.
- [ ] **P7-06 [M]** Act title cards ("ACT I - THE ARRIVAL" etc.).
- [ ] **P7-07 [M]** Final sequence: silhouette lean-in during lightning flash, CUT, crew cheer, epilogue lines, crew jacket handoff, DIRECTOR'S CUT clapper.
- [ ] **P7-08 [S]** Actor reaction montages on Perfect (smile/nod) and Miss (glance at camera).
- [ ] **P7-09 [C]** Simple cloth/hair wetness: actor material wetness param during rain.

---

## P8 - UI (D1–D3)

- [ ] **P8-01 [M]** `WBP_MainMenu` with 3D background camera, title clapper animation, buttons (`06 §3`).
- [ ] **P8-02 [M]** Locked "Shoot the Scene" button until Phase 1 complete.
- [ ] **P8-03 [M]** `WBP_Settings`: volumes, sensitivity, invert Y, FOV, head bob, subtitles, quality preset.
- [ ] **P8-04 [M]** `WBP_PauseMenu`: Resume, Settings, Main Menu (confirm), Quit (confirm) (`06 §4`).
- [ ] **P8-05 [M]** `WBP_ConfirmDialog` reusable.
- [ ] **P8-06 [M]** `WBP_HUD_Phase1`: crosshair, prompt, carry stack, Set Ready %, credits, ability bar, reels, toasts, subtitles (`06 §5`).
- [ ] **P8-07 [M]** `WBP_Notebook` (skills + map pages).
- [ ] **P8-08 [M]** `WBP_EndScreen_Phase1` (`06 §7`).
- [ ] **P8-09 [M]** `WBP_HUD_Phase2`: REC/timecode, take meter, cue card, next cue, frame warning, station widgets, act cards (`06 §8`).
- [ ] **P8-10 [M]** `WBP_CutInterstitial`.
- [ ] **P8-11 [M]** `WBP_EndScreen_Phase2` + `WBP_Credits` (`06 §9`).
- [ ] **P8-12 [M]** `WBP_Fade` / loading cover between levels.
- [ ] **P8-13 [M]** All button/panel tweens per `06 §2`.
- [ ] **P8-14 [S]** Controller navigation focus on menus ✂.
- [ ] **P8-15 [M]** Pause works in both phases and in station mode; tweens and UI audio run while paused.

---

## P9 - Audio (D3)

- [ ] **P9-01 [M]** Source/create every **[M]** SFX in `07`.
- [ ] **P9-02 [M]** Music: menu, Phase 1 layered stems, Phase 2 per-act tracks, stingers.
- [ ] **P9-03 [M]** Phase 1 music layering driven by zone completions (`07 §3.2`).
- [ ] **P9-04 [M]** Place-chime pitch ladder (MetaSound) for streaks.
- [ ] **P9-05 [M]** Marlowe voice: TTS VO **or** voice blips + megaphone submix. Same for actors and walkie lines (walkie band-pass + squelch).
- [ ] **P9-06 [M]** Attenuation presets (props: 200–2500 cm falloff) and occlusion off (cheap).
- [ ] **P9-07 [M]** Pause low-pass + music duck on barks (−6 dB).
- [ ] **P9-08 [M]** Volume settings hooked to Sound Mix and saved.
- [ ] **P9-09 [M]** Log every audio source in `CREDITS.md`.

---

## P10 - VFX & Shaders (D3)

- [ ] **P10-01 [M]** `M_GhostTapeMark` (holographic tape-mark ghost) (`08 §2.1`).
- [ ] **P10-02 [M]** `PP_Outline` post-process (stencil-based, colors per stencil value) (`08 §2.2`).
- [ ] **P10-03 [M]** `NS_PlaceSparkle` (gold) + `NS_WrongPuff` (red).
- [ ] **P10-04 [M]** `NS_ZoneComplete` (light-beam bloom + confetti) and stage-light turn-on effect.
- [ ] **P10-05 [M]** `NS_DustPuff`, `NS_SweepDebris`, `NS_TrashSwish`.
- [ ] **P10-06 [M]** `NS_RainSprinkler` + `NS_RainSplash` + `MF_Wetness` on set materials.
- [ ] **P10-07 [M]** `NS_EspressoSteam`, `NS_ThunderFlash` (+ light + post bloom spike), `NS_WindDebris`, `NS_RosePetals`.
- [ ] **P10-08 [M]** `M_FrameCone` + `M_FrameFloorDecal`.
- [ ] **P10-09 [M]** `PP_Cinematic` for ACTION state (letterbox via UI, grade, grain, vignette) (`08 §3.2`).
- [ ] **P10-10 [M]** `M_LensSmudge`, `M_BackdropTranslite` (Day/Sunset/Night blend).
- [ ] **P10-11 [S]** Ability VFX: Director's Eye scan wave, Grab Bag suction trails, Set Dresser's Touch sparkle trails.
- [ ] **P10-12 [S]** `NS_GoldenReelShimmer`, `NS_StarStamp` (UI Niagara or widget sparkle).
- [ ] **P10-13 [C]** Water droplets on the screen when the player stands in rain.

---

## P11 - Juice & Polish Pass (D3)

- [ ] **P11-01 [M]** Every pickup: pop tween + sound + crosshair pulse.
- [ ] **P11-02 [M]** Every placement: snap tween + squash + sparkle + chime (pitch ladder).
- [ ] **P11-03 [M]** Every group/zone completion: indicator lamp tween, stage light power-on flicker, toast bounce.
- [ ] **P11-04 [M]** Every button: hover scale, press squash, release bounce, hover/click sounds.
- [ ] **P11-05 [M]** Every panel: open bounce-in, close scale-out.
- [ ] **P11-06 [M]** Counters roll up; stars stamp in sequence.
- [ ] **P11-07 [S]** Camera micro-shake on thunder, zone complete, CUT slam (tiny, toggle-able).
- [ ] **P11-08 [S]** Carried stack sways with mouse movement (spring lag).
- [ ] **P11-09 [M]** Bark subtitle slide-in + megaphone icon pulse.
- [ ] **P11-10 [M]** Make sure nothing important happens in silence: idle barks, ambience, music layers.

---

## P12 - QA (D3)

- [ ] **P12-01 [M]** Full playthrough fresh save: menu → Phase 1 → end → Phase 2 → end → credits → menu.
- [ ] **P12-02 [M]** Quit mid-Phase 1, relaunch: Phase 1 restarts, Phase 2 still locked.
- [ ] **P12-03 [M]** Finish Phase 1, relaunch: Continue goes to Phase 2; "Shoot the Scene" unlocked.
- [ ] **P12-04 [M]** New Game resets progress but keeps settings.
- [ ] **P12-05 [M]** Every prop can reach its home (no prop stuck behind geometry, no ghost blocked). Run the `LTT.ValidateProps` check (`03 §2.5`).
- [ ] **P12-06 [M]** 100% is reachable: no orphaned tasks, every zone completes.
- [ ] **P12-07 [M]** Props thrown out of bounds respawn.
- [ ] **P12-08 [M]** Phase 2: every cue can be Perfect, Good, Late, and Missed without soft-lock; retake from each act works.
- [ ] **P12-09 [M]** Pause during: carry, station mode, cue window, bark, CUT interstitial, end screen.
- [ ] **P12-10 [M]** Settings persist; volume sliders affect correct classes.
- [ ] **P12-11 [M]** Frame rate check on Low and High; fix any spikes (rain particles, Lumen).
- [ ] **P12-12 [M]** Subtitles readable at 1280×720 and 4K.
- [ ] **P12-13 [S]** First-time-player test with someone who hasn't seen the game; note where they got stuck.
- [ ] **P12-14 [M]** No placeholder text, debug draw, or on-screen print strings in Shipping.

---

## P13 - Build & Submission (D3)

- [ ] **P13-01 [M]** Packaging settings: Shipping, Win64, include `L_MainMenu`, `L_Phase1_MessySet`, `L_Phase2_CafeShoot` in maps to cook.
- [ ] **P13-02 [M]** Project name, version, company, icon, splash.
- [ ] **P13-03 [M]** Package, run on a **different** PC without the engine installed.
- [ ] **P13-04 [M]** Zip build; check size and upload limits.
- [ ] **P13-05 [M]** Jam page: description, controls list, screenshots (5), GIF of rain cue, credits + AI-use disclosure per jam rules.
- [ ] **P13-06 [S]** 30–60 s trailer clip (clean-up time-lapse → ACTION → rain kiss → DIRECTOR'S CUT).
- [ ] **P13-07 [M]** Submit before the deadline with a buffer of at least 1 hour.
