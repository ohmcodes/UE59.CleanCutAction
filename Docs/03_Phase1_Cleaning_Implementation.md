# Phase 1 - Cleaning the Set (Implementation)

Covers checklist **P2, P3, P4**. Map: `L_Phase1_MessySet`. Target playtime 15–25 minutes.

---

## 1. The Set

### 1.1 Concept
The film's cafe is a **set built inside a soundstage**: two floors of cafe on the inside, raw plywood and braces on the outside, a painted street flat outside the front window, and the crew area (camera, director's platform, craft services) around it. Phase 1 and Phase 2 use the **same layout**; Phase 1 is the trashed version.

### 1.2 Layout (blockout dimensions, adjust to your props)

```
                 SOUNDSTAGE (40m x 30m)
 ┌──────────────────────────────────────────────────────────────┐
 │  CRAFT SERVICES        CREW AREA (camera dolly, monitor,    │
 │  (snack table,         director's platform, reel shelf,     │
 │   coffee urn)          costume rack, 4 stage lights)        │
 │                                                              │
 │   ┌──────────── CAFe SET (16m x 10m) ─────────────┐          │
 │   │ 1F:                                            │ BACKSTAGE│
 │   │  [KITCHEN]  [COUNTER + ESPRESSO]  [CREW STAIR]─┼─▶ (2nd   │
 │   │  [SEATING: 6 tables]   [FRONT STAIR]           │   stair) │
 │   │  [ENTRANCE + DOOR + FRONT WINDOW]              │          │
 │   └─────────────── STREET FLAT + CAR RAIL ─────────┘          │
 │         (painted façade, sidewalk, cardboard car)            │
 │   BACKDROP TRANSLITE (skyline: day / sunset / night)         │
 └──────────────────────────────────────────────────────────────┘
 2F (over the back 2/3 of the cafe): MEZZANINE (3 tables, bookshelf, plants)
     + open TERRACE at the front (2 tables, string lights, railing over the street)
```

Key measurements:
- 1F ceiling 380 cm; 2F floor at 400 cm; 2F railing 105 cm.
- Front stair: 200 cm wide, 22 steps of 18 cm with a landing. Crew stair: 180 cm wide, backstage, opens onto the mezzanine.
- Doorways ≥ 140 cm wide (player carries stacks; avoid snagging).
- Terrace must be visible from the camera's Act B/C positions (Phase 2).

### 1.3 Zones and Task Budget

| Zone | Tag | Placeables | Trash | Piles | Other tasks |
|---|---|---|---|---|---|
| Entrance & Counter | `Zone.Entrance` | 12 | 8 | 2 | Door sign, menu board |
| Seating (1F) | `Zone.Seating` | 24 (6 table sets) | 10 | 3 | 2 tipped tables (Right it) |
| Kitchen | `Zone.Kitchen` | 10 | 8 | 1 | - |
| Stairs (both) | `Zone.Stairs` | 4 | 6 | 2 | 1 cable |
| Mezzanine & Terrace (2F) | `Zone.Terrace` | 16 | 8 | 2 | 1 tipped table, string lights cable |
| Crew Area | `Zone.Crew` | 6 | 6 | 1 | 4 lights, 2 cables, 5 reels, lens, clapperboard slate stand |
| Craft Services | `Zone.Craft` | 8 | 8 | 1 | Costume rack (6) |
| **Total (target)** | | **~80** | **~54** | **~12** | |

Scale down by removing whole table sets if the prop audit comes up short. Never leave a zone with fewer than 6 tasks.

### 1.4 Props This Design Expects (check against `PropAudit.csv`)

| Need | Min count | Fallback if missing |
|---|---|---|
| Cafe chairs + stools | 24 + 6 | Any chair; vary materials |
| Cafe tables (small round/square) | 11 | Modeled cylinder table |
| Cups, saucers, plates | 30 | Simple modeled cup/plate |
| Table lamps / candles | 8 | Cylinder + emissive |
| Vases, plants, flowers | 10 | - |
| Menus, sugar jars, napkin holders | 15 | Boxes with decals |
| Picture frames, signs (OPEN/CLOSED) | 6 | Plane + texture |
| Cushions, books | 10 | - |
| Espresso machine, cash register | 1 each | Station mesh in Phase 2 |
| Trash: pizza boxes, cans, bottles, cups, crumpled paper, balloons, party hats, one shoe | 54 | Primitives + decals |
| Popcorn kernels / confetti (small instanced mesh) | 2 meshes | Tiny spheres / quads |
| Brooms | 1 (tool) | Cylinder + box |
| Cables (spline mesh), plugs | 4 | Spline mesh of cylinder |
| Stage lights on stands | 4 | Cylinder + cone |
| Film reels + reel shelf | 5 + 1 | Discs |
| Costume rack + hangers + outfits | 1 + 6 | Plane outfits |
| Film camera + dolly, director's chair, monitor | 1 each | - |
| Clapperboard + slate stand | 1 + 1 | Modeled |
| Megaphone, walkie-talkie | 1 each | - |
| Trash bins | 6 (1 per zone + spare) | Cylinder |

---

## 2. Building the Mess - the Messifier

The clean cafe (`L_Phase2_CafeShoot`) is authored first. Phase 1 is a duplicate where every placeable is moved away from home. The home transform stays on the prop, so the ghost appears exactly where the prop lives in Phase 2.

### 2.1 Prop identity

`ALTTPropBase` properties:

```cpp
UPROPERTY(EditAnywhere) FName PropID;                 // unique per level, e.g. "Seating_T3_Chair2"
UPROPERTY(EditAnywhere) FGameplayTag PropType;        // Prop.Type.Chair
UPROPERTY(EditAnywhere) FGameplayTag SizeClass;       // Prop.Size.Small/Medium/Large/Fixed
UPROPERTY(EditAnywhere) FGameplayTag Zone;            // Zone.Seating
UPROPERTY(EditAnywhere) FName GroupID;                // "Seating_T3" (table set), optional
UPROPERTY(EditAnywhere) FTransform HomeTransform;     // world, written by messifier --record-homes
UPROPERTY(EditAnywhere) FTransform MessyStartTransform; // written by --scatter (used for lost-prop respawn)
UPROPERTY(EditAnywhere) float SnapRadius = 60.f;
UPROPERTY(EditAnywhere) float SnapAngleTolerance = 25.f; // yaw tolerance before auto-correct, informational
UPROPERTY(EditAnywhere) bool bStartsAtHome = false;    // lets designers leave a few props already placed
```

PropID naming: `<Zone>_<Group>_<Type><Index>`.

### 2.2 Messifier algorithm (`Scripts/Python/messifier.py`)

Use the Python Editor Script Plugin (`unreal.EditorActorSubsystem`, `unreal.SystemLibrary.line_trace_single` or `unreal.EditorLevelLibrary` equivalents in 5.9).

```
--record-homes
  for each ALTTPropBase in level:
     assert PropID unique (else auto-generate and warn)
     HomeTransform = actor world transform
--scatter --seed 42
  for each prop where bStartsAtHome == False:
     pick a scatter rule by SizeClass:
       Small : random point within 150–600 cm of home, 30% chance anywhere in the same zone,
               10% chance another zone (incl. other floor) → creates cross-floor errands
       Medium: 100–400 cm from home; 40% tipped over (roll or pitch 70–100°)
       Large : stay within 80 cm; 60% tipped over (pitch 90°) or rotated yaw 30–150°
       Fixed : stays at home but rotated/offset (e.g. pushed table) → "Push back" task
     drop to surface: line trace down from point + 200 cm; reject points inside geometry
       (overlap test with prop bounds), on stairs steeper than 30°, or within 120 cm of the player start
     random yaw 0–360; Small items 25% chance random roll (lying on side)
     MessyStartTransform = final transform
  never leave a ghost spot occupied by another prop's messy position (min 80 cm away from any HomeTransform)
--restore   (debug) move every prop back to HomeTransform
--report    write Saved/MessReport.csv: PropID, zone, distance from home, floor change yes/no
```

After scatter, hit **Simulate**, let physics settle 3 s, then use "Keep Simulation Changes" on all props, and run `--record-messy` to overwrite `MessyStartTransform` with the settled poses.

### 2.3 Spawning the rest of the mess
Trash, piles, cables, lights, reels, costumes, the lens, clapperboard, and golden reels are **hand-placed** (or placed by `messifier.py --trash N` using random points on NavMesh per zone, then hand-adjusted). They exist only in Phase 1.

### 2.4 Hand-composed "mess stories"
Scatter alone looks random. Add these deliberate tableaus; each is funny on its own and teaches the player to look around:

1. **Pizza tower:** 9 pizza boxes stacked on the espresso machine, one open with a single slice.
2. **Chair throne:** a chair standing on top of a table with a party hat on it.
3. **Confetti blast:** cone-shaped confetti pile spraying from a confetti cannon prop by the stairs.
4. **Lamp in a plant:** a table lamp planted in a potted plant like a flower.
5. **Reel frisbees:** film reels stuck at odd angles on the 2F terrace (thrown from 1F).
6. **Costume scarecrow:** Dash's jacket on a stage-light stand with a pizza box "face".
7. **One shoe** on the director's chair (Marlowe has lines about it).
8. **Cable spaghetti:** all four cables knotted together in a pile at the crew area.
9. **Blanket fort:** 2F cushions arranged as a fort under a table.
10. **Camera lens:** smudged with a visible cartoony fingerprint (shader).
11. **The clapperboard:** hidden inside the kitchen fridge (openable door) or under the fort cushions (pick one; the fridge is funnier).

### 2.5 Validation (`LTT.ValidateProps` console command + Python `--validate`)
- Every prop has a unique PropID and a valid Zone.
- Every HomeTransform is reachable: NavMesh projection within 200 cm on the same floor.
- No two homes overlap (bounds intersection).
- Messy position isn't inside geometry and isn't beneath the KillZ.
- Count tasks per zone; warn if a zone has < 6.
- Print totals: placeables, trash, piles, special tasks.

---

## 3. Mechanics

### 3.1 Prop state machine (`ALTTPropBase`)

```
           pickup                  place on ghost
 Messy ───────────▶ Carried ─────────────────────▶ Placed (locked, kinematic, not interactable)
   ▲                  │ drop / throw
   │                  ▼
   └──── Dropped (physics on, counts as not placed) ◀─ (pickup again)
 Lost (fell out of bounds) → teleport to MessyStartTransform + DustPuff → Messy
```

- Messy/Dropped: physics simulated, `Prop` object channel, interactable.
- Placed: physics off, `SetMobility` stays Movable (avoid mobility changes at runtime), collision BlockAll, stencil off, **not interactable**. Placed props can't be knocked over (keeps progress safe).
- Events: `OnPlaced(Prop)`, `OnUnplaced(Prop)` (never fires once locked - kept for debugging).

### 3.2 Size classes

| Size | Carry | Notes |
|---|---|---|
| Small | Stacks up to capacity | Cups, plates, menus, books, vases, cushions |
| Medium | One at a time; can't add Small on top | Chairs, stools, lamps, plants, signs |
| Large | Not carried. **Hold E 0.8 s → "Right it"** | Tipped tables, sofa. Tween: rotate to upright home rotation 0.4 s OutBack, slide to home 0.3 s; dust puff on landing; counts as placed |
| Fixed | Not carried. **Hold E 0.6 s → "Push back"** | Counter stools bolted down, wall frames tilted (rotate back), the map board |

### 3.3 Ghost tape marks and snapping

- Each placeable spawns an `ALTTGhostMark` at runtime at its HomeTransform: same mesh, `M_GhostTapeMark` material, plus a yellow tape-mark decal on the surface under it.
- Ghost visibility:
  - Hidden by default (tape decal always visible but faint, 35% opacity - gives the player a hint of "something goes here").
  - When the player carries a prop: that prop's ghost fades in (0.2 s) everywhere; others stay hidden.
  - Director's Eye ability: all ghosts of the carried prop's **type** + loose matching props highlight (see §5.3).
- Snap check every tick while carrying: from the camera's aim hit point (trace 400 cm), find the carried top prop's ghost; valid if distance(aim point, ghost center) ≤ SnapRadius **or** the trace hits the ghost's collision (ghost has query-only collision on `Interact`), and line of sight is clear.
- Valid → ghost pulses gold (stencil 2), HUD prompt "Place" + crosshair turns gold.
- Click → **placement sequence**:
  1. Detach top prop from HandStack.
  2. Tween location to home (0.15 s OutCubic) and rotation (0.15 s, quat slerp) and scale to 1.
  3. Squash preset (`02 §7`).
  4. Spawn `NS_PlaceSparkle` at home; ghost fades out 0.15 s and is destroyed; tape decal swaps to a "checked" variant.
  5. Play `MS_PlaceChime` with pitch step = current streak (§3.14) + `SFX_Place_<PhysMat>` thunk.
  6. State → Placed; notify `ULTTCleanProgressSubsystem`.
- The prop is only placeable if it's the **top** of the stack. If a lower item in the stack matches the ghost being aimed at, show prompt "Top item first (scroll to cycle)", and allow **mouse wheel to rotate the stack order** (tween items 0.12 s each).

### 3.4 Trash and bins

- `ALTTTrashItem`: Small carry class, `Task.Trash`, no home.
- `ALTTTrashBin` (one per zone, more allowed): hold E to deposit **all trash** in the stack at once (each item flies in with 0.06 s stagger, OutCubic arc, tiny pop on the bin lid).
- Throwing trash: a trigger volume in the bin mouth; if a thrown trash item enters it → **Swish**: +1 basket counter, `NS_TrashSwish`, crowd "mild cheer" SFX, lid bounce tween, Marlowe bark (25% chance). The item is destroyed after 0.3 s.
- A thrown trash item that misses becomes a normal loose item (no penalty).
- Non-trash props thrown into a bin are **rejected**: bin "spits" it out with an upward impulse + wobble + bark `Bark.P1.BinWrongItem` ("That's a set piece, not garbage!").

### 3.5 Sweeping

- Broom: a tool picked up from the broom stand (Medium-class "tool", occupies the hands; press **Q** or click the stand again to put it back). While holding the broom, LMB = sweep.
- `ALTTSweepPile`: `UInstancedStaticMeshComponent` with 60–150 kernel/confetti instances in a disc (radius 80–160 cm), plus a dust decal under it.
- Sweep stroke (LMB, 0.35 s cooldown): broom swing animation (tween rotation of the broom mesh −35° → +35°, 0.25 s OutCubic). Sphere at aim point on floor, radius `SweepRadius` (60 cm, Wide Broom +30/+60); remove instances inside the sphere (up to 25 per stroke) by flinging each with a short tween toward the stroke direction then scaling to 0 (0.2 s) - cheap and readable.
- Pile cleared when < 5% of instances remain → remove the rest with a puff (`NS_DustPuff`), fade out the dust decal (0.4 s), count task done.
- Sound: brush stroke per swing + popcorn crunch layer scaled by instances removed.

### 3.6 Cable untangle

- `ALTTCablePuzzle`: spline mesh between a stage light and an outlet, with 6–10 control points in a messy tangle; plug end lying on the floor.
- Interaction: hold E on the plug end for 1.0 s → the cable "straightens": tween every spline point to its clean path (authored second spline) with 0.05 s stagger per point (OutBack), then the plug flies into the outlet (0.2 s), spark `NS_PlugSpark`, "click", light hums on at 20% (fully on when the light is aimed, §3.7).
- Optional [C] knot step: before straightening, 1–3 "knot" widgets on the cable must be clicked (each a small interactable sphere; click → knot pops with a "boing").

### 3.7 Stage light aiming

- `ALTTStageLight`: stand with yaw/pitch head, a `USpotLightComponent`, and a target mark ("X" decal on the set) defined by a target actor.
- Hold E → enter **aim mode**: mouse X/Y rotates the head (yaw ±70°, pitch ±45°) at 0.5× sensitivity; player movement locked; HUD shows "Aim at the X" and a closeness meter.
- Beam alignment = angle between light forward and direction to target. ≤ 3° → success. Feedback as the angle closes: beam color lerps cool → warm, a soft rising "whirr" pitch.
- On success: head locks with a Squash, lamp PowerOn preset, "kachunk" SFX, light's cable must also be plugged (if not, show "Needs power" and the light stays dim but aiming still counts - keep tasks independent to avoid confusion: **aiming and plugging are separate tasks**).
- Exit aim mode: release E, or success.

### 3.8 Film reel shelf

- Five reels (`ALTTReel`, Small) labeled 1–5 (number decals), scattered around.
- `ALTTReelShelf` with 5 ordered slots. Reels only snap to the slot with their number; ghosts behave like normal placeables.
- When all 5 are placed: shelf group completes, film-strip lamp above it lights up, and a short film-projector rattle SFX plays.
- Wrong-order joke: if the player aims reel N at slot M≠N, show a red ghost wobble and bark `Bark.P1.ReelWrongOrder` (cooldown 20 s).

### 3.9 Costume rack [S]

- Six outfits (Small fabric props) with colored tags: 3 blue (Juniper), 3 brown (Dash).
- Rack has 6 hanger slots color-coded (left blue, right brown); any matching color slot works (first free).
- Hanging tween: outfit flies to hanger, swings (rotation Punch roll ±12°, 0.6 s), hanger "shhk" SFX.

### 3.10 Camera lens

- On the film camera. Hold E → wipe for 2.0 s total; progress can be partial and resumes later.
- `M_LensSmudge` parameter `Smudge` 1 → 0 as you hold; the camera's small viewfinder screen (render-target texture or static image) sharpens accordingly.
- Each 0.5 s of hold plays a squeak and a tiny rotation wiggle of the cloth prop.
- Completion: "ding", lens glint flare (`NS_LensGlint`), bark.

### 3.11 Clapperboard

- Hidden (see mess story #11). It's a Small placeable with a home on the **slate stand** next to Marlowe's chair.
- When picked up the first time: special bark + the HUD shows a clapperboard icon in the progress widget.
- Placing it is required for Phase 1 completion. If all other tasks are done and the clapperboard isn't found, the hint system points at it (§6.4).

### 3.12 Throwing & physics
- Throw charge (hold RMB): 0–1 over 0.8 s; HUD shows a small arc preview dotted line (predict projectile path, 20 points) for the top item.
- Release → `ThrowTop(Charge)`. Item gets angular impulse for spin.
- Impacts: `OnComponentHit` with impulse threshold → impact SFX by phys material (concurrency-limited), small dust puff on hard hits.
- Thrown props may knock other **loose** props; placed props are immovable.
- Throwing a prop into its own ghost from range counts as a placement ("trick shot") if it enters the ghost's snap radius with speed < 1500 cm/s → extra sparkle + bark `Bark.P1.TrickShot`.

### 3.13 Lost-prop recovery
- Kill volume below the stage + out-of-bounds boxes. On overlap or if Z < KillZ: teleport to MessyStartTransform, physics sleep, `NS_DustPuff` + "poof" SFX, bark `Bark.P1.PropLost` (low chance).
- Every 5 s, any Dropped prop that's not moving and is inside a blocking/unreachable volume gets the same treatment.

### 3.14 Placement streak
- Streak counter increments on each correct placement within 6 s of the previous one; resets otherwise.
- Chime pitch ladder: C major scale steps (0, 2, 4, 5, 7, 9, 11, 12 semitones), capped at step 8 then loops at the top note.
- Streak 3/5/10 triggers barks and a little HUD "COMBO ×N" bounce.

---

## 4. Progress & Three-Tier Feedback

### 4.1 Tier 1 - item
Each correct action (place, binned trash, cleared pile, plugged cable, aimed light, reel, costume, lens) → item-level feedback as described above.

### 4.2 Tier 2 - group
- Groups (`GroupID`): a table set (table + its chairs + tabletop items), the reel shelf, the costume rack, a counter section, a light (aim + cable).
- Each group has an `ALTTGroupLamp` (small practical bulb or a tiny tally light on a C-stand). When all members are done → lamp PowerOn tween, "ding-ding" two-note chime, +1 Crew Credit, credit icon flies from the lamp's screen position to the HUD credits counter (UI tween 0.5 s OutCubic, counter bounce).

### 4.3 Tier 3 - zone
- `ALTTZoneVolume` (box/brush) per zone; holds the list of task actors inside (auto-gathered at BeginPlay by overlap + zone tag).
- Zone complete → that zone's big **stage light** turns on (PowerOn preset with "KA-THUNK"), warm light floods the area, `NS_ZoneComplete` (light motes + a little confetti), one **music layer** is added (`07 §3.2`), toast "SEATING - SET!" bounces in, +3 Crew Credits, zone bark.

### 4.4 Set Ready %

```
SetReady% = (done_place + done_trash + done_piles + done_special) / total_tasks × 100
```
All tasks count equally (simple and honest). The HUD clapperboard fills as % rises; milestone barks at 10/25/50/75/90/99%.

### 4.5 Completion condition
`SetReady == 100%` **and** clapperboard placed (it's one of the tasks, so this is automatic). Then trigger the completion sequence (§6.3).

---

## 5. Skills, Credits & Collectibles

### 5.1 Economy
- Sources: +1 per group (~28 groups), +3 per zone (7 zones = 21), +1 per 5 swishes (cap 3). Total ≈ 50 credits across a full run.
- Spend in the Notebook (Tab). Unlocks persist in the save (replays are faster - a nice loop).

### 5.2 Tree

| Branch | Skill | Kind | Ranks / Cost | Effect |
|---|---|---|---|---|
| Hustle | Comfy Sneakers | Passive | 2 / (2, 3) | +10% / +20% move speed |
| Hustle | Dodge the Shot (P2) | Passive | 1 / 4 | Frame cone edge warning (see `04 §4`) |
| Prop Master | Stack 'Em | Passive | 3 / (2, 3, 4) | Carry capacity +1 per rank (3 → 6) |
| Prop Master | Wide Broom | Passive | 2 / (2, 3) | Sweep radius 60 → 90 → 120 cm |
| Prop Master | Director's Eye | **Active (1)** | 3 / (3, 3, 4) | See §5.3 |
| Prop Master | Grab Bag | **Active (2)** | 3 / (4, 4, 5) | See §5.3 (requires Director's Eye rank 1) |
| Prop Master | Set Dresser's Touch | **Active (3)** | 2 / (5, 6) | See §5.3 (requires Grab Bag rank 1) |
| Special FX (P2) | Cue Sense | Passive | 1 / 3 | Next-cue preview |
| Special FX (P2) | Steady Hands | Passive | 2 / (3, 4) | Perfect windows ×1.25 / ×1.5 |
| Special FX (P2) | Long Throw | Passive | 1 / 3 | Max throw speed ×1.35, longer arc preview |
| Special FX (P2) | Director's Patience | Active (P2 only) | 1 / 5 | Once per take: freeze cue timer 3 s |

Phase 2 skills can be bought in Phase 1 (to plan ahead) and in the pre-shoot notebook at the start of Phase 2.

### 5.3 Active abilities

| Ability | Key | Duration | Cooldown by rank | Behavior |
|---|---|---|---|---|
| **Director's Eye** | 1 | 6 / 8 / 10 s | 30 / 24 / 18 s | Screen-space scan wave from the player (post-process ring expanding 0→30 m over 0.8 s). Everything of the **top carried item's PropType** (or, with empty hands, **every unfinished task within 15 m**) gets cyan outline (stencil 3) through walls; their ghosts show too. |
| **Grab Bag** | 2 | instant | 40 / 32 / 25 s | Pulls loose props of the **same PropType as the top carried item** (or trash, if holding trash) within 12 m, up to free capacity. Each flies to the stack along a curved path (0.4 s, staggered 0.05 s) with a sparkle trail. Not through walls (line-of-sight check from 150 cm above player). |
| **Set Dresser's Touch** | 3 | instant | 50 / 40 s | Every item in your stack whose home is within 10 m (rank 2: 15 m) flies home and places itself (0.3 s arc each, staggered 0.1 s), with full placement feedback. Items whose homes are out of range stay in your hands. |

Cooldowns use `ULTTSkillComponent` timers; HUD ability bar shows radial fill and plays a "ready" ping (`SFX_AbilityReady`) with an icon bounce when each finishes. Pressing a key on cooldown → icon wobble + soft "nope" + (rare) bark.

### 5.4 Golden film reels (hidden collectibles)

| Reel | Location | Reward |
|---|---|---|
| Reel.1 | **Visible from the player start**, on top of the craft-services coffee urn | Sprint (Shift) |
| Reel.2 | 2F blanket fort, under a cushion | Carry +1 |
| Reel.3 | On a catwalk-reachable spot above the kitchen (jump from counter → shelf → beam) | All ability cooldowns −15% |

Pickup: Bob idle tween + `NS_GoldenReelShimmer`; on pickup, it spins up into the camera (0.5 s), "unlock" fanfare, toast "GOLDEN REEL 1/3 - SPRINT UNLOCKED", bark. Saved in `FoundGoldenReels` immediately.

---

## 6. Phase 1 Flow

### 6.1 Intro (≈ 40 s, skippable after first completion)
1. Fade in on the player's start view. Ambience only, no music.
2. Marlowe's intro lines (`05 §1.1`) play in order (Priority 3). During them the player can already look around and move.
3. HUD fades in after line 2. The objective note (call sheet) appears as a UI card ("Clean the set. Find the clapperboard. Don't get fired.") with a stamp tween.
4. Line 4 ("Pick up that chair…") points at the tutorial chair: its ghost is forced visible and pulses; a world-space arrow bobs above the chair until it's picked up.
5. First placement → base music layer starts (fade in 2 s).

### 6.2 Contextual tutorials (one-shot)
Each first-time action fires a Priority-3 tutorial bark and a small prompt card (top-left, 4 s, slide-in). Cards: Pick up / Place / Stack / Throw (RMB) / Bin trash / Broom / Right it / Aim light / Cable / Notebook (fires on first credit) / Ability keys (fires on first ability unlock) / Sprint.

### 6.3 Completion sequence (≈ 12 s, input locked)
1. Last task done → 0.5 s silence (music ducks to 0).
2. All stage lights flicker on in a wave across the set (0.15 s stagger), bright PowerOn, big "KA-THUNK" chord.
3. Camera does a slow automatic 180° look-around (tween control rotation 3 s InOutSine) showing the clean set.
4. Marlowe completion barks (`05 §1.20`).
5. `WBP_EndScreen_Phase1` (see `06 §7`). Save results (`ReportPhase1Result`).

### 6.4 Hint system
- Track time since last progress. At 45 s → idle bark. At 90 s → find the nearest unfinished task (by path distance if cheap, else straight-line); its outline pulses cyan for 5 s and Marlowe gives a zone-specific hint ("Something's hiding in the kitchen, Rookie."). Repeat every 60 s.
- At ≥ 95% with only hard-to-find tasks left, also show a small HUD "Remaining: 2" list with zone names.

### 6.5 Stars

| Stars | Rule |
|---|---|
| ★ | Complete the set |
| ★★ | Complete under **25 min** **or** with ≥ 10 swishes |
| ★★★ | Complete under **18 min** **and** all 3 golden reels found (this run or earlier) |

Tune times after the first full play-test.

### 6.6 "You're Fired!" joke ending [S]
If the player opens the soundstage exit door with 0 zones completed after at least 2 minutes: confirm dialog ("Walk off the job?"). Yes → Marlowe line, a pink-slip end card "YOU'RE FIRED!", 1 star of shame, back to menu. Doesn't save completion.

---

## 7. Event → Bark Hook Table

| Gameplay event | Bark trigger tag | Notes |
|---|---|---|
| Level start | `Bark.P1.Intro` (chain of 4) | Priority 3 |
| First pickup / place / stack / throw / bin / sweep / right-it / light aim / cable / reel / costume / lens / notebook / ability | `Bark.P1.First.<Action>` | One-shot |
| Correct placement | `Bark.P1.Place.Correct` | 25% chance |
| Streak 3 / 5 / 10 | `Bark.P1.Streak.3/5/10` | |
| Drop not on ghost | `Bark.P1.Place.Wrong` | 30%, cooldown 15 s |
| Hard impact / crash | `Bark.P1.Crash` | 50%, cooldown 20 s |
| Throw any prop | `Bark.P1.Throw` | 15% |
| Trash binned / swish / miss | `Bark.P1.Trash.Bin/Swish/Miss` | 15% / 35% / 25% |
| Non-trash in bin | `Bark.P1.BinWrongItem` | |
| Sweep stroke / pile cleared | `Bark.P1.Sweep.Stroke/Cleared` | 5% / 40% |
| Cable plugged | `Bark.P1.Cable.Done` | |
| Light aimed | `Bark.P1.Light.Done` | |
| Reel wrong order / shelf done | `Bark.P1.Reel.Wrong/Done` | |
| Costume hung / rack done | `Bark.P1.Costume.Hung/Done` | |
| Lens clean | `Bark.P1.Lens.Done` | |
| Group complete | `Bark.P1.Group.Done` | 30% |
| Zone complete | `Bark.P1.Zone.<ZoneName>` | Priority 2 |
| Milestone % | `Bark.P1.Progress.<N>` | Priority 2 |
| Story drip % (20/40/60/80) | `Bark.P1.Story.<N>` | Priority 3, chains |
| Ability unlocked / used / on cooldown | `Bark.P1.Ability.<Name>.Unlock/Use`, `Bark.P1.Ability.Cooldown` | |
| Golden reel 1/2/3 | `Bark.P1.Reel.Gold.<N>` | |
| Clapperboard found | `Bark.P1.Clapper.Found` | |
| Idle 45 s | `Bark.P1.Idle` | |
| Hint 90 s | `Bark.P1.Hint.<ZoneName>` | |
| Jump spam (5 jumps in 4 s) | `Bark.P1.JumpSpam` | |
| Standing on a table | `Bark.P1.OnTable` | |
| Staring at Marlowe > 3 s | `Bark.P1.StareAtMarlowe` | |
| Prop lost & respawned | `Bark.P1.PropLost` | |
| Trick shot | `Bark.P1.TrickShot` | |
| Phase complete | `Bark.P1.Complete` (chain) | |
| Exit door / fired | `Bark.P1.Fired` | |
