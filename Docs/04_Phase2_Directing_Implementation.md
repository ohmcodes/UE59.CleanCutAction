# Phase 2 - Shooting the Date Scene (as built)

Covers checklist **P5, P6, P7**. Updated 2026-11-15 from the source; the original plan is summarised under "Planned / cut" at the end of each section.

Map: `/Game/StylizedNewYorkCity/Level/L_NYC_Cafe_Phase2_Michael` (cooked via `MapsToCook`). Reached from the Phase 1 end screen's **Shoot the Scene** button, which only shows when the set is clean (`ALTTMessDirector::NextLevelName`).

The scene is **Scene 42: The Cafe**. Dash and Juniper play Leo and Clara on a first date. They're real-life exes and keep going off-script. Marlowe shouts cues; the player runs to effect stations on both floors without walking into the shot.

Code lives in `Source/CleanCutAction/Shoot/`:

| Class | File | Role |
|---|---|---|
| `ALTTSceneDirector` | `LTTSceneDirector.h/.cpp` | Runs the scene, scoring, take meter, CUTs, HUD and end screen |
| `ULTTSceneData` | `LTTSceneData.h/.cpp` | Script data (beats, lines, cues) + the built-in Scene 42 |
| `ALTTEffectStation` | `LTTEffectStation.h/.cpp` | One class for every station; picks a mini-game type |
| `ALTTFilmCamera` | `LTTFilmCamera.h/.cpp` | Film camera, frame test, see-through cone, act moves |
| `ALTTFilmActor`, `ALTTActorMark` | `LTTFilmActor.h/.cpp` | Dash and Juniper, and the marks they walk to |
| `ULTTShootHUD` | `LTTShootHUD.h/.cpp` | Shoot HUD (baked to `WBP_ShootHUD`) |
| `ULTTShootEndScreen` | `LTTShootEndScreen.h/.cpp` | Wrap screen (baked to `WBP_ShootEndScreen`) |

---

## 1. Level Setup

Everything is placed directly as the C++ classes (there are no `BP_` subclasses for the director, stations or actors). Anything missing is skipped with a log warning.

### 1.1 Stations

11 `ALTTEffectStation` actors. Each has a `Station_*_Visual` mesh actor in `HighlightActors` (outlined while its cue is active). The mini-game type per station follows the suggested setup in `LTTEffectStation.h`.

| Actor | Cue(s) | Mini-game | Effect actors / sounds |
|---|---|---|---|
| `Station_Slate` | Slate (tutorial) | Press | `SW_ClapperboardClap` |
| `Station_CarRail` | Traffic | Sustain | `FX_TrafficHeadlights`; `SW_CarBeepBeep` / miss `SW_BicycleBell` |
| `Station_DoorBell` | Door (opt) | Timing | `SW_ShopDoorBell` / `SW_CowBell` |
| `Station_Espresso` | Steam | HoldRelease | `FX_Steam` (`NS_Steam`); `SW_Espresso` / `SW_CoffeeSputter` |
| `Station_PropTable1F` | Napkins | Toss | `SW_PaperSlide` / `SW_LoudBonk` |
| `Station_Thunder` | Thunder (x2) | Shake | `FX_ThunderFlash`; `SW_ThunderClap` / `SW_SheetWobble` |
| `Station_LightRig` | Sunset, Moonlight (opt) | Choice | Key light `KeyLight_Gel`; `SW_SwitchOn` / `SW_SwitchOff` |
| `Station_WindFan` | Wind (opt) | Sustain | `FX_Wind` (`NS_Wind`); `SW_WindFan` / `SW_FanTurnOff` |
| `Station_BoomMic` | Boom | Sustain | `SW_Creak` / `SW_MicFeedback` |
| `Station_RainRig` | Rain | Sustain | `FX_Rain` (`NS_Rain`); `SW_LightRain` (loops) / `SW_WaterDrop` |
| `Station_PropTable2F` | Umbrella, Roses | Toss | `SW_UmbrellaOpen` / `SW_LoudBonk` |

The sound pairings come from the level's references and the asset names. The exact station-to-sound wiring is inside the binary `.umap` and was not checked one by one.

All mini-game tuning (bands, times, ranges) uses the C++ defaults. None is overridden in the level.

### 1.2 Film camera

One `ALTTFilmCamera` (BroadcastStudio camera mesh). `ActAnchors` = `CamAnchor_A`, `CamAnchor_B`, `CamAnchor_C`. On an act change it moves in a straight line (location lerp + rotation slerp, InOutSine) over `MoveTime` = 3 s. After a CUT or an act jump it snaps there instantly.

### 1.3 Actor marks

`ALTTActorMark` (arrow = facing) placed for every mark the script uses: `A_LeoTable`, `A_ClaraDoor`, `A_ClaraTable`, `B_ClaraRail`, `B_LeoRail`, `C_Center_L`, `C_Center_R`. A NavMesh covers both floors.

### 1.4 World / weather state

When a cue lands, `ALTTSceneDirector::ApplyCueToWorld` tweens the `MPC_World` scalars `RainIntensity`, `Wetness`, `LightningFlash` (double flicker, 0.3 s), `WindStrength` and `BackdropTime` (1 sunset, 2 night). **`MPC_World` doesn't exist and `WorldMPC` is unset**, so this does nothing visible today. The visible weather is all station effect actors: rain/wind/steam Niagara, thunder flash light, headlights, and the gel key light colour.

**Planned / cut:** `MPC_World` and the materials that read it (wet surfaces, sky backdrop time of day), and the `ReceiveCueEffect` / `ReceiveTakeReset` Blueprint hooks (they exist but no Blueprint director implements them).

---

## 2. Scene Director

### 2.1 Data (`LTTSceneData.h`)

- `ELTTCue`: `Slate, Traffic, Door, Steam, Napkins, Thunder, Sunset, Wind, Boom, Rain, Umbrella, Roses, Moonlight` (a C++ enum, not gameplay tags).
- `ELTTGrade`: `None, Perfect, Good, Late, Miss`.
- `FLTTDialogueLine`: `Speaker`, `Text`, optional `Voice`, `Montage`, `MoveToMark`, `MoveActor`, `ExtraDelay`, `bOffScript`.
- `FLTTCueSpec`: `Cue`, `BaseWindow`, `bGradeBySpeed` (+ `PerfectFrac` 0.4 / `GoodFrac` 0.8), `bTutorial`, `CallLine` (the text itself).
- `FLTTBeat`: `BeatID`, `Act`, `bActCheckpoint`, `DashStartMark` / `JuniperStartMark`, `RestoreEffects`, `bOptional`, `PreDialogue`, `bHasCue`, `CueSpec`, `PostPerfect/Good/Late/Miss` (each falls back to the next better list), `PostAlways`.
- `ULTTSceneData`: `PreRoll`, `SlateDone`, `Beats`, `Wrap`, `ActTitles`, `RestartLines`.

**Scene 42 is built in C++** (`ULTTSceneData::FillWithScene42`, full text in `05 §2`). The level's director has no `SceneData` asset, so it calls `MakeScene42` at BeginPlay. A `DA_` asset can be made and filled with the **Fill With Scene 42** Details button, but none exists.

### 2.2 Flow

```
BeginPlay: reset game input, build Scene 42, add HUD, (next tick) gather stations/actors/marks/camera,
           lock Phase 1 props, camera to act A, actors to their A marks
StartDelay 1.5 s -> PreRoll lines -> Slate cue (no timer) -> SlateDone lines -> rolling
for each beat (optional beats skipped if bSkipOptionalBeats or no station):
   checkpoint beat of a new act: camera move + act title card (max(3 s move, 2.5 s card)), frame checks paused
   PreDialogue -> cue call (megaphone bark + SW_notification, station outlined) -> wait for station / timeout
   -> grade -> score + meter -> station effect -> Marlowe reaction bark -> Post lines + PostAlways
after last beat: finale (frame checks off, SW_applause, Wrap lines) -> wrap -> 1 s -> end screen
```

- One cue at a time. The HUD shows **NEXT: <cue>** during the beat's PreDialogue.
- A beat whose cue has no station plays its Good lines and moves on (optional beats are skipped instead).
- Line timing without voice-over: `0.3 s × words + 0.6 s`, min 1.6 s. No voice files are assigned.
- Speakers: a line by `Leo`/`Clara` shows as `LEO (Dash)` / `CLARA (Juniper)`; off-script lines show `Dash (off-script)`.
- Marlowe's per-cue reaction bark (`GetResultBark`, `05 §2.4`) is shown over the Post lines only when those lines have no Marlowe line, and never on the last beat.
- Events: `OnActStarted`, `OnCueCalled`, `OnCueResolved`, `OnCut`, `OnSceneWrapped`, `OnLineSpoken`.
- `bLockSetProps` (on): every `ALTTInteractableBase` is frozen and can't be picked up during the shoot.

### 2.3 Cue window and grading

```
Window = (BaseWindow + Travel) × WindowScale (1)
Travel = clamp((2D distance + 2 × height difference) / 450 cm/s × 1.15, 0, 12)   // straight line at call time
```

- The station reports a grade when its mini-game ends. If the window runs out first, `HandleCueTimeout` gives Miss, except: a prop already in the air → Late (Miss if the throw was a miss); a Sustain at ≥ 60% of its in-band time → Late.
- Speed-graded cues (`bGradeBySpeed`: Napkins, Sunset, Umbrella, Roses, Moonlight): time grade = Perfect within 40% of the window, Good within 80%, else Late. The worse of the station grade and the time grade counts.
- Sustain stations grade by quality: ≥ 0.85 Perfect, ≥ 0.6 Good, ≥ 0.3 Late, else Miss.

**Planned / cut:** NavMesh path length for travel; `ELTTBeatPhase` enum; Director's Patience (freeze), the Steady Hands / Long Throw / Dodge the Shot skills and the pre-shoot notebook; actor "waiting" idles during cues.

---

## 3. Effect Stations (`ALTTEffectStation`)

- Interactable (`ILTTInteractable`, kind `Station`) **only while one of its cues is active**. Then it gets an outline (custom depth stencil 1, `M_PP_Outline`) on itself and its `HighlightActors`. Prompt: "Use <DisplayName>".
- Entering locks movement and look (look stays free for Toss). The click that opened it is ignored (0.15 s arm).
- Input is read straight from keys, no Input Actions: **LMB / gamepad RT** = action; **E / RMB / gamepad B** = leave early (Marlowe: "Where are you going? The cue isn't done!"; the cue keeps ticking). Instructions switch to gamepad wording when a pad is in use.
- On resolve: `EffectActors` are shown and activated (Niagara restarts) for `EffectDuration` (0 = until take reset), `EffectSound` or `MissSound` plays. A looping sound (rain) keeps playing until the effect ends or the take resets. `bShowEffectOnMiss` is off everywhere, so a missed cue only plays its miss sound.

| Mini-game | Used by | Rules (C++ defaults) |
|---|---|---|
| **Press** | Slate | One click → Perfect. Tutorial: no timer, no score. |
| **Timing** | Door | A marker crosses the bar in 2.2 s; target at 0.75. Error ≤ 0.15 s Perfect, ≤ 0.35 s Good, ≤ 0.7 s Late, else Miss. Reaching the end = Miss. |
| **HoldRelease** | Steam | Hold to fill (2.2 s, eases out). Release in 0.62–0.78 Perfect, ±0.11 outside Good, ≥ 0.3 Late, else Miss. Holding past 1.2× fill time = over-pressure: "Too much! It's a cafe, not a sauna!", Miss. |
| **Sustain** | Traffic, Wind, Boom, Rain | Hold LMB / W / stick up to raise the needle (+0.7/s), release to drop (−0.5/s). Spend 3 s in the 0.55–0.75 band. Quality = in-band time / time since first entering the band − 0.5 × time above 0.9 / 3 s. Optional Perlin wobble (`SustainWobble`, 0 in the level). |
| **Shake** | Thunder | Alternate mouse left/right, A/D, a stick or LB/RB: each direction change +0.12, decays 0.25/s. At full, "FLASH!": click / F / gamepad Y within 0.4 s Perfect, 0.8 s Good, later Late (auto Late after 2.5 s). |
| **Choice** | Sunset, Moonlight | Gels Orange (Sunset), Blue (Moonlight), Green (never right). Pick with 1–3, wheel, A/D, d-pad or LB/RB; click to slide in. The key light takes the gel colour. Right gel first time → Perfect, after a wrong one → Late. Wrong gel: "<COLOR>?! WRONG GEL! WRONG GEL!" and keep trying. |
| **Toss** | Napkins, Umbrella, Roses | Aim with the camera at Dash's `CatchPoint`; hold to charge (bar goes up and back down, 1.1 s each way), release. Right power = distance / 1400 cm. Power error ≤ 0.07 and aim ≤ 4° Perfect; ≤ 0.15 and ≤ 8° Good; ≤ 0.28 and ≤ 16° Late (lands at his feet); else Miss (bonks him if roughly aimed, else flies off). The prop flies on an arc; the grade is reported when it lands. |

The toss prop is the engine cube (`TossMeshes` not set). Both prop tables throw to `Dash` (default `TossTargetName`).

**Planned / cut:** station camera tween to a `ViewPoint` and `IMC_Station`; "Not yet" prompt when no cue is active; the pushed cardboard car (Traffic is a Sustain meter); door timing tied to Juniper's walk; boom frame line / mic-in-shot test and "BOOM IN SHOT" bark; rain valve + hose aiming; over-wind; projectile arc preview and catch ring; catch-to-hand socket; desk-lamp flicker misses; per-station grade-type table with distinct timing/quality modes (now: station grade + optional speed grade).

---

## 4. Frame Cone

### 4.1 Test (`ALTTFilmCamera::IsPointInFrame`)
- Point in camera space; outside `NearDist` 50 – `MaxDist` 2200 cm → out.
- Horizontal half-angle = `HorizontalFOV` / 2 (default 40°, overridden on the level's camera); vertical from `AspectRatio` 1.777.
- Occlusion: line trace on Visibility; only blockers with a **static** root hide the player (props and characters don't).
- `IsPawnInFrame` tests the eyes and the capsule centre; either counts.

### 4.2 Rules (`ALTTSceneDirector::TickFrameCheck`)
- Checked only while rolling and not during the slate, act camera moves / title cards, the CUT interstitial or the finale. No penalty during the pre-roll.
- 0.25 s grace, then −4 take meter and −10 points per second. Marlowe barks one of 5 in-frame lines (8 s cooldown).
- Meter hits 0 → CUT.

### 4.3 Look
- `ConeMesh` = the engine cone stretched to the frustum, material `M_FrameCone` (faint, depth-tested, edge glow, scanlines, depth and near-camera fades; cooked via `MapsToCook`). The material's `Color` parameter is set per state:

| `ELTTConeLook` | When | Colour |
|---|---|---|
| Idle | Pre-roll and rolling, nobody in shot | Yellow |
| Alert | Player in shot | Red |
| Allowed | Slate | Green |
| Hidden | Finale and wrap | - |

- HUD while in shot: "YOU'RE IN THE SHOT!" and a pulsing red vignette.

**Planned / cut:** `M_FrameFloorDecal`, a "Show camera frame" setting, the alarm beep loop, the Dodge the Shot edge glow.

---

## 5. Scoring, Take Meter and Retakes

### 5.1 Points (director defaults)

| Event | Points | Take meter |
|---|---|---|
| Perfect | +100 | +10 |
| Good | +60 | +5 |
| Late | +25 | −3 |
| Miss | 0 | −15 |
| In frame (per second, after 0.25 s) | −10 | −4 |
| CUT | −50 | reset to 60 |

Take meter 0–100, starts at 60. Shown score is never below 0. `bAllowCut` can turn CUTs off.

### 5.2 Stars
`MaxScore` = 100 × cues that have a station (13 with every station placed).

| Stars | Rule |
|---|---|
| 1 | Finish the scene |
| 2 | Score ≥ 55% of max **and** ≤ 1 CUT |
| 3 | Score ≥ 80% of max **and** 0 CUTs **and** < 6 s in frame |

### 5.3 CUT / retake (`CallCut`, `ResetToCheckpoint`)
1. Everything stops, one of three "CUT!" barks, `SW_cut`, actors frozen, HUD shows "CUT! TAKE N" for 2.5 s.
2. Back to the act's checkpoint beat: score and grade counts go back to what they were when the act started, minus 50. Take meter 60. Stations reset (effects off, gel light restored). MPC values zeroed.
3. The act's `RestoreEffects` are replayed. Act C restores Sunset and Rain, so the rain is back on.
4. Camera snaps to the act anchor, actors teleport to the act's start marks, 0.8 s fade from black, Marlowe's restart line, then the checkpoint beat replays.

The take number counts every CUT in the run.

**Planned / cut:** `WBP_CutInterstitial` as its own widget (it's a HUD text), actors' slump montage.

---

## 6. Film Actors (Dash and Juniper)

- `ALTTFilmActor` (Character, auto-possessed `AAIController`). Meshes `SKM_StylizedMale` / `SKM_StylizedFemale` from `ST_Characters_JC`.
- No Anim Blueprint: a single-node blend space (`BS_Dash_Locomotion`, `BS_Juniper_Locomotion`, idle/walk/run) driven by speed.
- `IsNamed` matches either `ActorName` (Dash/Juniper) or `CharacterName` (Leo/Clara).
- `MoveToMark`: `MoveToLocation` (20 cm), then turn to the mark's arrow. Walk 160, run 300 for `StormOff` lines. If not there after 9 s, or there's no path, they teleport.
- `Speak` plays the line's named montage or a random talk montage. **The `Montages` and `TalkMontages` maps are empty in the level**, so the actors don't gesture yet.
- `SetFrozen` during CUTs. `CatchPoint` (35 cm in front of the chest) is the toss target.
- Hooks `ReceiveLineSpoken` / `ReceiveCueResult` exist; nothing implements them.

**Planned / cut:** seated poses, look-at, voice blips, waiting idles, reaction montages, `hand_r` catch socket.

---

## 7. HUD (`WBP_ShootHUD`)

REC + timecode (STANDBY before rolling, THAT'S A WRAP after), "Scene 42 - Take N - Act X", points, TAKE METER bar, cue card ("CUE: RAIN!" / "NEXT: RAIN", station name + floor label, countdown bar), in-shot warning + vignette, subtitles with speaker label, Marlowe's megaphone bark, act title card, CUT text, grade toast ("PERFECT!  +100"), station instruction + painted mini-game meter (green band, progress), an edge arrow to the active station, and a hint under the crosshair ("Rain Rig - 12 m (upstairs)").

---

## 8. End Screen (`WBP_ShootEndScreen`)

After the wrap (1 s delay): "THAT'S A WRAP!" slam-in, "N / 3 STARS" ("DIRECTOR'S CUT" at 3), Marlowe's quote by stars, cue counts (perfect/good/late/missed), CUTs, time in shot, points roll-up, **Shoot Again** (reloads the level), **Back to Cleanup** (opens `L_NYC_Cafe_Michael`), and the QLE copyright line. The game is **not paused**: the HUD hides and the player can keep walking the set with the cursor shown. Gamepad focus starts on Shoot Again.

**Planned / cut:** `bPhase2Complete` save, best score/stars, leaderboard submit (Phase 1 only), credits, main menu, crew-jacket "ITEM GET" toast, confetti and crew cheer, finale input lock and silhouette shot.

---

## 9. Debug

Console: `LTT.SkipCue [Perfect|Good|Late|Miss]`, `LTT.JumpToAct <A|B|C>`, `LTT.Cut`. Director options: `bSkipOptionalBeats` (drops Door, Wind, Moonlight), `bPlayPreRoll`, `WindowScale`, `bAllowCut`.

## 10. Packaging

`MapsToCook` in `Config/DefaultGame.ini` lists `L_NYC_Cafe_Phase2_Michael`, `M_FrameCone`, `WBP_ShootHUD` and `WBP_ShootEndScreen` (loaded by path). The director's default SFX (`SW_notification`, `SW_success`, `SW_miss`, `SW_cut`, `SW_applause`) are set in its constructor. The station SFX and `NS_*` systems are referenced by the level.

## 11. Bark hooks

There is no tag-based bark system in Phase 2. All lines are plain text in C++: script lines in `LTTSceneData.cpp`, and the reactions, in-frame, CUT and quote lines in `LTTSceneDirector.cpp`. The bail, over-steam and wrong-gel barks are in `LTTEffectStation.cpp`. See `05 §2`.
