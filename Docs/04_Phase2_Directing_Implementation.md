# Phase 2 - Shooting the Date Scene (Implementation)

Covers checklist **P5, P6, P7**. Map: `L_Phase2_CafeShoot` (the clean master cafe). Target playtime 5–7 minutes per take attempt.

The scene is **Scene 42: The Cafe**. Dash and Juniper play Leo and Clara on a first date. They're real-life exes and keep going off-script. Marlowe shouts cues; the player operates effects across both floors without walking into the shot.

---

## 1. Map Setup

### 1.1 Stations (placement)

| Station | Floor | Location | Cue(s) |
|---|---|---|---|
| Slate stand | 1F | In front of the film camera, Act A position | `Cue.Slate` (tutorial) |
| Car Rail | 1F exterior | Behind the street flat, handle visible through the front window | `Cue.Traffic` |
| Door Bell rope | 1F | Beside the cafe door, inside the crew side of the wall | `Cue.Door` |
| Espresso Steam lever | 1F | Back of the espresso machine (crew side of the counter) | `Cue.Steam` |
| Prop Table 1F | 1F | Kitchen pass-through window | `Cue.Napkins` |
| Thunder Sheet + Flash | 2F | Backstage landing at top of the crew stair | `Cue.Thunder` |
| Light Rig (gels) | 2F | Mezzanine corner, on a scaffold | `Cue.Sunset`, `Cue.Moonlight` |
| Wind Fan | 2F | Terrace side, behind a planter | `Cue.Wind` |
| Boom Mic | 2F | Mezzanine, next to the terrace door | `Cue.Boom` |
| Rain Rig valve + hose | 2F | Scaffold above the terrace (reached from mezzanine ladder, 3 rungs) | `Cue.Rain` |
| Prop Table 2F | 2F | Mezzanine bar | `Cue.Umbrella`, `Cue.Roses` |

Two staircases (front stair + backstage crew stair) let the player choose routes. The **front stair is inside the Act A frame**, so taking it during Act A costs points; the crew stair is longer but safe. This is the core positioning decision.

### 1.2 Film camera positions

| Act | Camera position | Frames |
|---|---|---|
| A - The Arrival | 1F on dolly, facing the seating area and front window | Table 3, the door, front stair bottom |
| B - The Terrace | On a crane at the mezzanine edge, facing the terrace | Terrace, terrace door, part of the mezzanine |
| C - The Rain | Crane lowered/rotated for a wider terrace shot including the railing and street flat | Terrace + railing + backdrop |

Moves between acts: tween the camera actor along an authored spline over 3 s (InOutSine) while the act title card plays. The frame cone moves with it.

### 1.3 Actor marks
`ALTTActorMark` (arrow component + name): `A_LeoTable`, `A_ClaraDoor`, `A_ClaraTable`, `B_ClaraRail`, `B_LeoDoor`, `B_LeoRail`, `C_Center_L`, `C_Center_R`, plus exits. Each beat references marks by name.

### 1.4 Lighting / weather state
Driven by `MPC_World` parameters (see `08`):

| Param | Start | Changed by |
|---|---|---|
| `BackdropTime` (0 day, 1 sunset, 2 night) | 0.3 | Sunset cue → 1, Moonlight cue → 2 |
| `RainIntensity` 0–1 | 0 | Rain cue (ramp with valve) |
| `Wetness` 0–1 | 0 | Follows rain with 4 s lag; never dries in-scene |
| `LightningFlash` 0–1 | 0 | Thunder flash pulse |
| `WindStrength` 0–1 | 0 | Fan crank |

Stage key light color is driven by the Light Rig gel (orange sunset, blue moonlight, **green** for the wrong gel joke).

---

## 2. Scene Director

### 2.1 Data

```cpp
UENUM() enum class ELTTGrade : uint8 { None, Perfect, Good, Late, Miss };
UENUM() enum class ELTTBeatPhase : uint8 { PreDialogue, CueCall, CueActive, PostDialogue, Done };

USTRUCT() struct FLTTDialogueLine {
  FName Speaker;            // Marlowe, Dash, Juniper, Leo(=Dash in character), Clara(=Juniper in character)
  FText Text;
  TSoftObjectPtr<USoundBase> Voice; // optional
  FName ActorMontage;       // e.g. "Talk2", "Laugh", "Angry"
  FName MoveToMark;         // optional: start moving when line starts
  float ExtraDelay = 0.f;
  bool bOffScript = false;  // subtitle style: italic + "(off-script)" tag, camera-shy glance
};

USTRUCT() struct FLTTCueSpec {
  FGameplayTag Cue;         // Cue.Rain
  FGameplayTag Station;     // Station.Rain
  float BaseWindow = 8.f;   // seconds after the call before Miss (before travel bonus)
  float PerfectFrac = 0.4f; // first 40% of window = Perfect for timing-graded cues
  float GoodFrac = 0.8f;
  bool  bQualityGraded = false; // true → grade from station performance (sustain cues) instead of speed
  bool  bTutorial = false;      // can't fail, no meter change
  FName CallLineID;             // Marlowe's call (DT_DirectorLines row)
};

USTRUCT() struct FLTTBeat {
  FName BeatID;
  FName Act;                            // "A", "B", "C"
  TArray<FLTTDialogueLine> PreDialogue;
  bool bHasCue = true;
  FLTTCueSpec CueSpec;
  TArray<FLTTDialogueLine> PostPerfect; // played on Perfect
  TArray<FLTTDialogueLine> PostGood;    // on Good (falls back to PostPerfect if empty)
  TArray<FLTTDialogueLine> PostLate;    // on Late (falls back to PostGood)
  TArray<FLTTDialogueLine> PostMiss;    // on Miss (comedic recovery)
  bool bActCheckpoint = false;          // first beat of an act
  FName CameraPosition;                 // set at act start
};

UCLASS() class ULTTSceneData : public UPrimaryDataAsset {
  UPROPERTY(EditAnywhere) TArray<FLTTBeat> Beats;
  UPROPERTY(EditAnywhere) TMap<FName, FText> ActTitles; // A → "ACT I - THE ARRIVAL"
};
```

The full content of `DA_Scene42` is written in `05 §2`. The agent should import it from a JSON file (`Data/Scene42.json`) via a small editor utility or by hand-filling the asset.

### 2.2 Beat state machine (`ALTTSceneDirector`)

```
PreRoll (Marlowe intro, slate tutorial) ─▶ "ACTION!" ─▶ for each Beat:
   PreDialogue: play lines in order (actors move/montage per line)
   CueCall:     Marlowe call line (megaphone) + HUD cue card pops + station highlighted (stencil 5)
   CueActive:   window timer runs; station reports result → grade
                (actors loop an idle "waiting" montage: sip coffee, look around)
   PostDialogue: play PostPerfect/Good/Late/Miss list
   → next beat
After last beat ─▶ Finale ─▶ "CUT! That's a wrap!" ─▶ End screen
```

- Only **one cue is active at a time**. The next beat's PreDialogue starts as soon as the grade is known.
- `Cue Sense`: when a cue resolves, the HUD shows "NEXT: <Cue> (<Floor>)" for the next beat's cue during its PreDialogue.
- Events (multicast): `OnBeatStarted`, `OnCueCalled(Spec, Window)`, `OnCueResolved(Cue, Grade, Points)`, `OnTakeMeterChanged(Value)`, `OnCut(Reason)`, `OnActStarted(Act)`, `OnSceneWrapped(Score, Stars)`.

### 2.3 Rolling state
The camera "rolls" from ACTION to CUT. While rolling: REC indicator, timecode, `PP_Cinematic` blend weight 1 (tween 0.6 s), letterbox bars slide in (UI), frame-cone checks active. Between acts the camera keeps rolling (no penalty during the 3 s camera move; checks paused).

### 2.4 Cue window & timing grades

```
Window = BaseWindow + TravelBonus
TravelBonus = clamp(PathLength(player → station) / 450 cm/s × 1.15, 0, 12)   // computed at call time, NavMesh path if available
```

For timing-graded cues, the station reports **completion time t** since the call:
- t ≤ Window × PerfectFrac × SteadyHandsMult → **Perfect**
- t ≤ Window × GoodFrac → **Good**
- t ≤ Window → **Late**
- else / failed station action → **Miss**

For quality-graded cues, the station reports a **quality 0–1**: ≥ 0.85 Perfect, ≥ 0.6 Good, ≥ 0.3 Late, else Miss (Steady Hands widens station tolerance instead of time).

**Director's Patience** (active, key 1 in Phase 2, once per take): freezes the window timer for 3 s, plays a big "EVERYONE FREEZE!" bark, actors freeze mid-pose (anim play rate 0), desaturated screen tint. Funny and useful.

---

## 3. Effect Stations

### 3.1 Base class - `ALTTEffectStationBase`
- Implements `ILTTInteractable` (`Kind = Station`). Interactable only when its cue is active (plus the Slate in pre-roll). Otherwise prompt says "Not yet - wait for the cue".
- Enter station mode (press E): push `IMC_Station`, lock movement, smoothly tween the camera to the station's `ViewPoint` component (0.35 s OutCubic) while keeping first-person feel (it's the player's head moving, ≤ 120 cm from the body), show the station widget.
- Exit: on success/failure (auto after 0.6 s), or press E/RMB to bail (cue keeps ticking).
- Virtual hooks: `StartStation()`, `TickStation(float Dt)`, `HandleAxis(FVector2D)`, `HandleAction(bool bPressed)`, `HandleAlt(bool)`, `FinishStation(ELTTGrade or float Quality)`.
- Each station has a "stand-in" mode for **missed** cues: if the cue misses, the station still plays a weak/funny version of its effect so the scene reads (e.g. a tiny drizzle).

### 3.2 Slate (tutorial, can't fail)
- Pre-roll. Player grabs the clapperboard (already in hand automatically), stands in the marked spot **inside the frame** (the one time it's allowed - frame cone turns green), presses LMB: clapper arm tween closes (0.08 s InBack) → CLAP SFX → white flash 1 frame on the monitor → Marlowe "ACTION!".
- Teaches: cue cards, station highlight, and that the cone is visible. After the clap, the cone turns red and Marlowe says "Now get OUT of my shot."

### 3.3 Traffic - Car Rail (quality-graded)
- Grab the handle; the cardboard car must travel from mark A to mark B (6 m) along the rail.
- Hold **W** (or move mouse forward) to push; speed meter on HUD with a green band (target 180–260 cm/s; Steady Hands widens ±20%/±40%).
- Quality = fraction of travel time spent inside the band. Travel ends at mark B; car honks automatically at the midpoint (comedic "meep meep").
- Visual: car wobbles (Punch roll) with speed; cardboard flap SFX, wheel squeak loop pitched by speed.
- Miss version: car gets pushed half-way, tips over (rotation tween), Marlowe groans.

### 3.4 Door Bell (timing, single press)
- A ring indicator around Juniper's walking marker: when she reaches the door, the target moment hits.
- Press LMB on the rope. |Δt| ≤ 0.15 s Perfect, ≤ 0.35 s Good, ≤ 0.7 s Late, otherwise Miss (bell rings at the wrong time, Juniper awkwardly re-opens the door).
- Steady Hands multiplies these tolerances.

### 3.5 Espresso Steam (hold & release)
- Hold LMB on the lever: steam pressure gauge needle rises (0 → 1 over 2.2 s, OutCubic feel).
- Release inside the green arc (0.62–0.78) → Perfect; yellow arcs (0.5–0.62 / 0.78–0.88) → Good; else Late/Miss.
- Over-pressure (hold past 1.0): **FOG EVERYWHERE** comedy - `NS_EspressoSteam` huge, actors cough montage, counts as Miss.
- Feedback: hiss loop pitch rises with pressure, machine shakes (Punch, intensity by pressure).

### 3.6 Prop toss (Napkins, Umbrella, Roses)
- At the prop table, the correct prop auto-goes to the player's hands when they press E (wrong props on the table are decorative).
- Catch zone: glowing ring (world-space widget or Niagara ring) above the target actor's hands, radius 35 cm (center 15 cm).
- Hold RMB to charge; dotted arc preview (20 points, `PredictProjectilePath`) colored green when it intersects the catch zone; release to throw. Long Throw increases max speed so far targets are reachable (some tosses are 1F → 2F railing!).
- Hit center → Perfect, ring edge → Good. If it lands within 2 m → Late (actor bends over to pick it up, "ugh" voice blip). Miss → it bonks the actor on the head (comedic "bonk" + Startled montage) or flies off the balcony; the actor improvises (PostMiss lines).
- Throw timing is also checked against the window (whichever is worse wins).
- Actor catch: when the projectile enters the catch sphere, detach physics, tween to the actor's hand socket (0.12 s), play Catch montage.

### 3.7 Thunder (two-step)
- Step 1: shake the thunder sheet - alternate mouse left/right (or A/D); each direction change adds rumble (+0.12). Rumble decays 0.25/s. Fill to 1.0.
- Step 2: when full, a "FLASH!" prompt appears; press **F** within 0.4 s (Perfect) / 0.8 s (Good) / later (Late).
- Effect: `LightningFlash` MPC pulse (0 → 1 → 0 over 0.25 s with a double flicker), thunder crack + rumble SFX, camera micro-shake.
- Miss version: a sad little "bwomp" with a desk-lamp flicker.

### 3.8 Light Rig - Sunset / Moonlight (choice)
- Three gel frames hang on the rig: orange, blue, green. Scroll or press 1–3 to select (selected gel pops up), LMB to slide it in (slide tween 0.2 s + clack).
- Correct gel → grade by time. Wrong gel → light turns that color (green = "Why is she green? Is she seasick?") and counts as Miss; the player can swap again within the window to recover as Late.
- Effect: key light color tween 1.2 s, `BackdropTime` tween to 1 or 2 over 2 s.

### 3.9 Wind Fan (sustain)
- Crank: hold LMB to spin up; RPM rises, releases fall. Keep the wind meter in the sweet spot (0.55–0.75) for a total of 3 s within the window.
- Quality = in-zone time / 3 s (capped) minus a penalty for time spent over 0.9 (over-wind: napkins and Juniper's scarf fly away comedically, `NS_WindDebris` heavy).
- Effect: `WindStrength` MPC → actor hair/cloth (if supported) + string lights sway + leaves.

### 3.10 Boom Mic (sustain + stay out of frame)
- Player holds a boom pole; mouse Y raises/lowers the mic, mouse X swings left/right a little.
- HUD shows the **frame line** (projected top edge of the camera frame at the actors' depth) and the **audio range band** just above it.
- Keep the mic tip inside the band for 5 s total (window). Mic below the frame line = "BOOM IN SHOT!" red flash, quality penalty, Marlowe bark.
- Wobble: Perlin noise sway on the pole (amplitude reduced by Steady Hands).
- Quality = time-in-band / 5 s − time-in-frame × 0.5.

### 3.11 Rain Rig (two-step + sustain) - the trailer moment
- Step 1: open the valve - rotate the wheel by moving the mouse in circles (or hold LMB; slower). Valve 0 → 1. Water starts at 0.3.
- Step 2: aim the hose nozzle (mouse) so the spray target circle covers the actors (marked area). Keep coverage ≥ 70% for 6 s.
- Quality = 0.3 × valve speed score + 0.7 × coverage score.
- Effect: `NS_RainSprinkler` from the rig, `NS_RainSplash` on terrace surfaces, `RainIntensity` + `Wetness` MPC ramp, rain audio loops, actors switch to "in rain" idle (shoulders up, then laughing).
- Rain **stays on** for the rest of Act C after success (stand-in drizzle if missed).

### 3.12 Station quick reference

| Cue | Station | Grade type | Base window | Act |
|---|---|---|---|---|
| Slate | Slate | Tutorial | ∞ | Pre |
| Traffic | Car Rail | Quality | 9 | A |
| Door | Door Bell | Timing | 7 | A |
| Steam | Espresso | Timing (release) | 8 | A |
| Napkins | Toss 1F | Timing + accuracy | 8 | A |
| Thunder | Thunder Sheet | Timing (2-step) | 10 | A |
| Sunset | Light Rig | Timing + choice | 9 | B |
| Wind | Fan | Quality | 9 | B |
| Boom | Boom Mic | Quality | 9 | B |
| Rain | Rain Rig | Quality | 12 | B→C |
| Umbrella | Toss 2F | Timing + accuracy | 8 | C |
| Roses | Toss 2F | Timing + accuracy | 8 | C |
| Moonlight | Light Rig | Timing + choice | 8 | C |
| Thunder (finale) | Thunder Sheet | Timing (2-step) | 9 | C |

If time is short, cut Door, Wind, Moonlight (and use Sunset only) - the script in `05` marks which beats are optional.

---

## 4. Frame Cone (Stay Out of Frame)

### 4.1 Test (`ALTTFilmCamera::IsPointInFrame(FVector P)`)
```
Local = CameraTransform.InverseTransformPosition(P)
if Local.X < NearDist (50) or Local.X > MaxDist (2200): false
HalfH = HorizontalFOV/2 (default 40° → half 20°), HalfV = HalfH / AspectRatio(2.39 anamorphic → use 16:9 for gameplay = 1.777)
if |atan2(Local.Y, Local.X)| > HalfH: false
if |atan2(Local.Z, Local.X)| > HalfV: false
Occlusion: line trace on FrameCheck channel from camera to P; if blocked by static geometry → false
```
Test two points on the player: head (camera location) and capsule center. Either inside → in frame.

### 4.2 Rules
- Active only while rolling and not during act camera moves, slate, or the finale lock.
- Grace 0.25 s before penalty starts. Penalty: take meter −4 per second in frame. First entry each beat triggers a bark (`Bark.P2.InFrame`, cooldown 8 s).
- Feedback while in frame: red screen-edge vignette (UI, pulse 2 Hz), REC dot turns into "⚠ IN SHOT", frame cone material turns red, a soft alarm beep loop (UI sound).
- **Dodge the Shot** skill: when within 100 cm of the frustum boundary (test with an inflated FOV +4° / range +100), the cone edge nearest the player glows orange + gentle tick sound.

### 4.3 Visuals
- `M_FrameCone`: a frustum mesh (pyramid from camera to MaxDist, scaled by FOV) with translucent unlit material: very low opacity fill (0.04), brighter edges (fresnel/edge mask), animated diagonal stripes, depth fade where it intersects geometry.
- `M_FrameFloorDecal`: deferred decal projecting the frustum footprint onto floors, yellow-white hatching (like tape marks).
- Toggle: settings option "Show camera frame" (default on).

---

## 5. Scoring, Take Meter & Retakes

### 5.1 Points

| Grade | Cue points | Take meter |
|---|---|---|
| Perfect | +100 | +10 |
| Good | +60 | +5 |
| Late | +25 | −3 |
| Miss | 0 | −15 |
| In frame (per second) | −10 | −4 |
| Director's Patience used | 0 | 0 |
| Retake (per CUT) | −50 | reset to 60 |

Take meter: 0–100, starts at 60. Shown as a film-strip bar (HUD). Hits 0 → **CUT**.

### 5.2 Stars (end of scene)
Max cue points = 100 × number of scored cues (13 with all optional cues, fewer when cut).

| Stars | Rule |
|---|---|
| ★ | Finish the scene |
| ★★ | Score ≥ 55% of max **and** ≤ 1 CUT |
| ★★★ | Score ≥ 80% of max **and** 0 CUTs **and** < 6 s total in frame |

### 5.3 CUT / retake flow
1. Marlowe: "CUT! CUT CUT CUT!" (Priority 3). Actors stop, slump montage.
2. `WBP_CutInterstitial`: clapper slam "CUT! - TAKE N", Marlowe quip, 2.5 s, then fade.
3. Reset to the current act's checkpoint beat: actors teleport to their first marks of that act (behind the fade), MPC values reset to the act's starting values, stations reset, take meter = 60, Patience restored.
4. Marlowe: "From the top of the act! Places! … ACTION!"
- Retake counter persists across the whole scene for scoring.

---

## 6. Film Actors (Dash & Juniper)

- `ALTTFilmActor` (Character) with `AAIController`. Movement: `MoveToLocation` on mark arrival with acceptance radius 20 cm, then rotate to the mark's facing (0.3 s tween).
- Walk speed 160 (calm), 300 (storming off).
- Anim BP: locomotion blendspace (idle/walk/run), `bSitting` pose for the table beats, montage slot `UpperBody` for talk gestures and `FullBody` for big reactions.
- Talking: on each dialogue line, play a random talk montage from the speaker's set unless the line names one; voice blips play per word (`07 §2.6`), mouth not animated (fine for the art style; optional jaw bone wobble from blip envelope [C]).
- Look-at: head look-at the other actor while talking; glance at the camera on off-script lines (comedy beat) and at the player on Miss beats ("Seriously?").
- Waiting idle during cues: per-character loop (Dash checks his reflection in a spoon; Juniper scrolls her phone).
- Reaction montages: Perfect → small smile/nod (30% chance); Miss → glance at camera.
- Catch points: socket `hand_r` with a sphere `CatchZone` (radius 35 cm) offset in front of the chest when "expecting" a toss.

---

## 7. Phase 2 Flow

1. **Arrival:** fade in at the crew area. Ambient set noise, crew walla, actors in position at marks for Act A. HUD minimal.
2. **Pre-shoot notebook (optional):** Marlowe: "Spend your credits now, Rookie. Once we roll, we roll." Notebook opens with Phase 2 skills tab first; closing it continues. (Skippable after first time.)
3. **Pre-roll:** Marlowe's speech (`05 §2.1`), slate tutorial, "ACTION!".
4. **Act A → B → C** per the script, with act title cards and camera moves.
5. **Finale (input locked from the last cue):** silhouette lean-in during the final lightning flash, Marlowe whispers, "CUT! That's a WRAP!", crew cheer, confetti from the catwalks, epilogue dialogue, crew jacket handoff (it flies to the camera like a pickup; HUD toast "ITEM GET: CREW JACKET").
6. **End screen** (`06 §9`), save `bPhase2Complete`, best score/stars → Credits (`06 §9.3`) → Main Menu.

---

## 8. Phase 2 Bark Hooks

| Event | Trigger tag |
|---|---|
| Arrival / pre-shoot | `Bark.P2.Arrival`, `Bark.P2.PreShootNotebook` |
| Pre-roll chain | `Bark.P2.PreRoll` |
| Cue call | per beat (`CallLineID`) |
| Cue result | `Bark.P2.Result.<Cue>.<Grade>` (one line per cue per grade, see `05 §2.4`) |
| In frame | `Bark.P2.InFrame` |
| Boom in shot | `Bark.P2.BoomInShot` |
| Wrong gel | `Bark.P2.WrongGel.<Color>` |
| Over-steam / over-wind | `Bark.P2.OverSteam`, `Bark.P2.OverWind` |
| Toss bonk | `Bark.P2.Bonk.<Actor>` |
| Station bail (left station early) | `Bark.P2.Bail` |
| Idle during an active cue (10 s, not moving) | `Bark.P2.CueIdle` |
| Patience | `Bark.P2.Patience` |
| CUT / restart act | `Bark.P2.Cut`, `Bark.P2.RestartAct.<Act>` |
| Wrap | `Bark.P2.Wrap` (chain) |
