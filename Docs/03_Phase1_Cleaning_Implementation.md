# Phase 1 - Cleaning the Set (Implementation)

Covers checklist **P2, P3, P4**. This doc describes Phase 1 **as built and packaged** (branch `windows`, Nov 2026). The original plan (Messifier, zones, skills, golden reels, stars...) was mostly replaced by a simpler "earthquake + put it back" loop; what was cut is listed in §9.

| | |
|---|---|
| Map | `/Game/StylizedNewYorkCity/Level/L_NYC_Cafe_Michael` (`GameDefaultMap` in `Config/DefaultEngine.ini`). It loads the `/Game/ParisStreet/Levels/SampleMap` level instance for the street outside. |
| Next map | `/Game/StylizedNewYorkCity/Level/L_NYC_Cafe_Phase2_Michael` (in `MapsToCook`) |
| Core actors in the level | `ALTTMessDirector`, `ALTTCleanupBarker`, `BP_ElectricBox` (`ALTTPowerBox`), `ALTTMusicPlayer`, `ALTTMirror`, `ALTTCheatButton` |
| Code | `Source/CleanCutAction/Interaction/`, `UI/LTTBriefingScreen`, `UI/LTTEndingOverlay`, `UI/LTTBarkWidget`, `World/LTTMirror` |
| Target playtime | Under 30 min (the time-trial clock is 30 min) |

---

## 1. The Set

- The set is the StylizedNewYorkCity cafe, enlarged on `dev/michael`: two floors (ground floor Z 0, mezzanine Z 300), a wide stair with a landing, a kitchen in the east corner, a west bar. Details: `Docs/Level_Changes_dev-michael.md` (history, read-only).
- The level is **authored clean**. Every prop's position in the editor is its home. There is no separate messy map.
- Props are `BP_Prop_*` children of `/Game/_CleanCutAction/Blueprints/Interactables/BP_MasterInteractables` (C++ parent `ALTTInteractableBase`). About 110 prop Blueprints exist in `Blueprints/Interactables/Props/`: chairs, bar stools, tables, cups, plates, cutlery, pastries, syrups, coffee bags, books, pictures, vases, flower pots, pillows, boxes, laptop, lamps, sockets, the film camera, trash, the bin, the mop.
- Other interactables in `Blueprints/Interactables/`: `BP_ElectricBox`, `BP_Lamp_CeilingLamp_01/02`, `BP_Lamp_WallLamp_01` (`ALTTLightFixture`), `BP_Sink` (`ULTTWashStationComponent`), `BP_WaterPuddle`, `BP_Counter_Drawer` / `BP_Counter_Lid_a/b` (`BP_MasterOpenable`), `BP_ShopSign_02_LED` (`ULTTFlickerComponent`), `BP_BreakDebris`.

## 2. Making the Mess: the Earthquake (`ALTTMessDirector`)

No editor tool scatters props. The mess is made at runtime, behind a black screen (`LTTMessDirector.cpp`, `SetupTasks`, `TriggerEarthquake`):

1. `BeginPlay`: black screen, HUD created. Next tick every prop records its home (`RecordHome`) and finds the prop it rests on (`FindHomeSupport`, e.g. cup → saucer → table).
2. **Job cap.** `MaxCleanupTasks = 100`. Cleanup props plus extra jobs (puddles) are counted; if there are more than 100, the smallest freestanding props (nothing rests on them, not tools / switches / trash / dishes) become set dressing until the total fits (`TrimTasksToCap`). So the set always has **100 cleanup jobs**, including the 2 water puddles.
3. Props with `bStartsAtHome` snap home and count as placed.
4. After `EarthquakeDelay` (1 s) every cleanup prop gets a random launch: horizontal 200–550 cm/s, up 150–400 cm/s, spin up to 400°/s, scaled down for big props (chairs and tables slide and tip, small props fly). `RandomSeed = 0` = a different mess every run. Set dressing and the mop are not thrown.
5. Props settle for `SettleTime` (2.5 s) while the screen stays black. Stuck props are rescued (§6.5) before the fade-in.

Prop flags (`LTTInteractableBase.h`):

| Flag | Meaning |
|---|---|
| `bIsCleanupTask` (default on) | Counts as a job. Off = set dressing: never picked up, never simulated, ignored by the aim trace (commit `250621c`). |
| `bStartsAtHome` | Starts placed and locked. |
| `bUseCustomHome` / `CustomHome` | Home is somewhere else than the editor position (e.g. belongs in a drawer). `SetCustomHomeHere` editor button. |
| `bIsTrash` | No home; goes in a bin. |
| `bIsTrashBin` | `BP_Prop_Bin`. |
| `bIsDishwashable` | No home; goes in a sink. Set on cups, cup set, milk cup, plates, cookie plate, fork, knife, spoon. |
| `bIsMop` | `BP_Prop_Mop`: a tool, always pickable, never breaks, not thrown by the earthquake. |
| `bChangesMusic` | Once placed, Interact skips to the next music track (the laptop). |
| `BarkKind` | Which director lines the prop triggers (§7). `Auto` picks from the class name. |

## 3. Core Loop: Pick Up and Put Back

### 3.1 Carrying (`ULTTCarryComponent`)
- One prop at a time, held in front of the camera with first-person hand IK (two hands for props wider than 22 cm). No stacking.
- Pick up / place / drop: **LMB or E** (gamepad X). Throw: **RMB** (gamepad RT). Throw speed 1100 cm/s scaled by prop size.
- Picking up a prop wakes the props resting on it, so they fall.

### 3.2 Home silhouettes and placing (`ALTTInteractableBase`, `ULTTInteractionComponent`)
- While a prop is held, its home shows as a translucent silhouette (`M_GhostSilhouette`):

| Colour | Meaning |
|---|---|
| Blue | Its spot. |
| Yellow | You're aiming at it: Interact = **Place**. |
| Orange | The prop it rests on isn't back yet (or: this support is the one the held prop needs first). |

- Place is valid when the view ray (within 400 cm) passes through the silhouette or within the snap radius (automatic from size). Walking the carried prop into its silhouette also counts.
- Placing slides the prop home, locks it (kinematic, not interactable), plays its place sound and tells the mess director. Lamps light up when placed; placed lamps, the microwave and the espresso machine can then be used (§5).
- **Order matters:** a prop can't be placed until its home support is placed (table before plate before croissant). Trying to place it out of order drops it and Marlowe complains.

`ELTTPlaceHint` (what Interact does while holding): `Drop`, `Place`, `NeedsSupport`, `Fumble`, `Trash`, `Wash`, `Mop`.

### 3.3 Breaking
- **Fumble:** dropping a prop while the aim only just misses its silhouette (within 35 cm outside the snap radius) arms it to break on landing (`bBreakOnNearMissDrop`).
- **Throwing** breaks props on impact (Blueprint `BreakFromImpact` in `BP_MasterInteractables`, gated by `bBreakOnThrow`). Soft props can opt out.
- Breaking uses one shared pre-fractured cube, `/Game/_CleanCutAction/BreakFX/GC_Generic_Shatter`, scaled and tinted to the prop (`ShatterGeneric`); pieces vanish after 4 s.
- A broken prop is removed from the jobs and counts as **broken** (−15 points). If other props rest on it (a table under cups), it goes back to its home in red (`M_BrokenProp`) instead, so the set can still be finished (`BreakAway`, `NotifyBrokenInPlace`).
- **Trick shot:** a thrown prop that flies into its own silhouette slower than 1500 cm/s is placed.

### 3.4 Trash and bins
- Trash props (`BP_Prop_Trash`, `BP_Prop_Trash_02`, other instances flagged `bIsTrash`) have no home. Aim at `BP_Prop_Bin` and Interact, or drop / throw them in: they shrink into the bin (0.3 s) and count as one job done.
- A non-trash prop that lands in a bin is spat back out.

### 3.5 Dishes and the sink
- Dishwashable props have no home. Aim at a sink basin (`BP_Sink`, `ULTTWashStationComponent`, aim radius 30 cm) and Interact, or drop them in: bubbles + `SW_SinkWash`, the dish shrinks away and counts as one job.

### 3.6 Water puddles and the mop (`ALTTWaterPuddle`)
- Two `BP_WaterPuddle` spills on the floor. Walking through one slows the player to 45% speed, with splashes.
- The mop (`BP_Prop_Mop`) leans against the back wall of the store (commit `89e1d3e`). Hold it, aim at the puddle (reach 260 cm) and click: each stroke shrinks the puddle; 3 strokes clean it (`MopsNeeded`). The mop stays in the hand.
- Without the mop, the puddle prompt says "Needs a mop".
- Each puddle is an extra job counted inside the 100 (commit `1f051d1`).
- Assets: `M_WaterPuddle`, `NS_MopSplash`, `SW_MopSwish`, `SW_PuddleSplash`, `SW_PuddleClean`.

## 4. Power: Start in Total Darkness (`ALTTPowerBox`, `ULTTPowerSubsystem`)

Added in commit `0823dec` ("start total black required to turn on breaker").

- `BP_ElectricBox` has `bStartsOff = true`. On a map with a mess director it cuts the mains on the first tick, silently.
- Power off is a **full blackout**: every light component (lamps, sun, sky light) plus sky atmosphere, height fog and volumetric clouds are hidden. Only the box's red indicator dot and light stay on so it can be found. (The header comment in `LTTPowerSubsystem.h` still says sky lights are untouched; the `.cpp` hides them.)
- While the power is off and the player isn't holding anything, the HUD arrow points at the box with "Power's off! Turn it on at the electric box".
- Interact on the box toggles the power with sparks, a relay click and a crackle (0.6 s cooldown). Lights come back exactly as they were; lamp switches flipped in the dark take effect when the power returns.
- Power is not a job and doesn't block placing, but the player can barely see without it.
- Phase 2 (no mess director) always keeps the power on.

## 5. Things That Work Where They Stand

| Thing | Class | Behaviour |
|---|---|---|
| Wall / ceiling lamps | `ALTTLightFixture` + `ULTTLightSwitchComponent` | Interact toggles the light. `SW_LightClick`. |
| Pickup lamps (`BP_Prop_ModernLamp`) | `ULTTLightSwitchComponent` on a prop | Dark while out of place; lights up when placed; then Interact toggles it. |
| Microwave (`BP_Prop_Microwave`) | `ULTTApplianceComponent` | Once placed: door opens, closes, runs 4 s, ding. |
| Espresso machine (`BP_Prop_EspressoMachine`) | `ULTTCoffeeMachineComponent` | Once placed: grind 1.6 s, a mug pops onto the tray, coffee pours 3 s, buzz. |
| Laptop | `bChangesMusic` | Once placed: next music track (`ALTTMusicPlayer`). |
| LED shop sign | `ULTTFlickerComponent` | Neon flicker. |
| Counter drawers / lids | `BP_MasterOpenable` | Open / close. Props whose home is inside move with it (`ShiftHome`). |
| Mirror | `ALTTMirror` | Scene-capture mirror (commit `bf0bc41`). Decorative. |

The microwave and espresso routines derive from `ULTTUsableComponent`; a placed prop with one (or a light switch, or `bChangesMusic`) is used instead of picked up.

## 6. Run Flow

### 6.1 Briefing (`ULTTBriefingScreen`, `WBP_Briefing`)
Shown over the black screen while the mess is made. Three-page carousel:
1. **Director's note** (Marlowe's letter: wrap party, Dash and Juniper's first-date scene, "Break anything and it comes out of your paycheck").
2. **How to play** (silhouette colours, order rule, fumble / throw breaks, hint key).
3. **Controls and scoring.**

Also: leaderboard name box (random name, dice button, on-screen keyboard on touch) and a Leaderboard button. Two start buttons:
- **START TIME TRIAL!** (Enter / gamepad A): 30-minute countdown.
- **JUST CLEAN (NO TIMER)** (N / gamepad Y): no clock, no time bonus.

### 6.2 Start
Start → stuck-prop rescue → 3 s fade in → clock starts (time trial). The set is pitch black (§4). Marlowe's intro starts 1.2 s later, then the "find the breaker" line.

### 6.3 Controls (keyboard / gamepad)

| Action | Keyboard / mouse | Gamepad |
|---|---|---|
| Move / look | WASD or arrows / mouse | Left / right stick |
| Jump | Space | A |
| Crouch | C / Ctrl | R3 |
| Pick up / place / drop / use | LMB / E | X |
| Throw | RMB | RT |
| Hint | F | Y |
| Dance (emote) | G | D-pad Up |
| Controls card on / off | H | D-pad Down |
| Pause | P / Esc | Menu |

Touch controls exist for phones / tablets (on-screen hand, throw, jump, crouch, hint, pause, music buttons).

### 6.4 HUD (`ULTTCleanupHUD`, `WBP_CleanupHUD`)
- Placed count (`placed / total`), destroyed count, points, countdown (pulses; changes colour when low).
- "[F] Hint" with cooldown.
- Arrow toward the held prop's home, the hinted prop, or the electric box (power off), when it's off-screen.
- A ring over the nearest pickable prop near the crosshair.
- Bottom-left controls card (keyboard or gamepad list, follows the last device used).

### 6.5 Hints and stuck props
- **Hint (F / Y):** outlines the nearest prop that can be put back now (supports ready first) for 8 s, with the HUD arrow, name, distance and "upstairs / downstairs". 3 s cooldown. Hints cost 0 points but are counted on the end screen. Pressing again moves to a different prop.
- If the same prop is hinted twice in a row, it's probably clipped or under the floor: it's moved to the middle of its floor first (`bMoveRepeatHintToMiddle`).
- **Stuck-prop rescue:** every 2 s, props under the floor, more than 25 m from home, or stuck inside geometry (on two checks in a row) are moved to a free spot near home.
- Pause menu has **Unstuck (back to start)** for the player.

### 6.6 Scoring (`ALTTMessDirector`)

| Item | Points |
|---|---|
| Each job done (prop placed, trash binned, dish washed, puddle mopped) | +10 |
| Each prop broken | −15 |
| Each hint | 0 (`PointsPerHint`) |
| Time trial: each second left when the set is clean | +1 |

`Score = placed×10 + broken×(−15) + hints×0 + time bonus`. No stars.

### 6.7 End of phase
**Set clean** (all jobs done; broken props drop out of the total):
1. Clock stops, time bonus is computed, Marlowe's "Complete" chain plays.
2. After 1.5 s, the ending shots: a camera tour of the cafe (`TourCameraLabels`: Interior_15, 3, 14, 17, 6; 2.5 s each with fades; resolved to real cameras at save / cook time). A **SKIP** button (Space / Enter / A) is shown (`ULTTEndingOverlay`).
3. Confetti (drawn in the overlay, no Niagara), then the end screen.

**Time's up** (time trial only): straight to the end screen (no tour).

End screen (`ULTTEndScreen`, `WBP_EndScreen`):
- Title "SET READY!" (`SW_next_phase`) or "TIME'S UP!" (`SW_miss`).
- Rows: time on set, props returned (`placed / total`), props destroyed, hints used, time bonus, points.
- Buttons: **Shoot the Scene!** (set clean only; opens Phase 2), **Play Again**, **Keep Playing** (after time's up: play on without a clock), **Leaderboard**, **Full Leaderboard** (opens `cca.ohmcodes.com/leaderboard`).
- The run is sent to the online leaderboard (`ULTTLeaderboardSubsystem::SubmitRun`); the screen shows the rank.

There is no save game for Phase 1 progress; Phase 2 is reached through the end screen button.

## 7. Director Barks (`ALTTCleanupBarker`)

Marlowe heckles through `ULTTBarkWidget` (gold pill, bottom centre, megaphone icon). Lines are hard-coded in `LTTCleanupBarker.cpp` (`BuildPools`); the full list and triggers are in `05 §1`.

- Text only, no voice. Optional `LineSound` / `WalkieSound` blips.
- Line length: 0.32 s per word, min 2.4 s, +0.9 s padding.
- Priorities: 3 = tutorial / story / finale, 2 = milestones, 0–1 = reactions. New lines interrupt after 1 s on screen; interrupted priority 2–3 lines are said again, reactions are dropped. Reactions are at least 4 s apart. `ChattinessScale` multiplies reaction chances.
- Tutorial lines stop for the session once the player has placed 10 props and a tutorial line gets interrupted.
- **Disable barking:** console `LTT.DisableBarks` (or `LTT.DisableBarks 1`) stops all barks and clears the queue; `LTT.DisableBarks 0` turns them back on (commit `cb0e7c8`). Blueprint: `SetBarksEnabled`.

## 8. Dev Cheats

| Cheat | What it does |
|---|---|
| `LTT.PlaceAll [LeaveCount=5]` | Puts every job back (incl. puddles) except `LeaveCount` props nothing rests on. Tests the ending. |
| `ALTTCheatButton` (in the level) | Interact (or step on it) runs the same cheat. `bAllowInShipping` defaults to **true**; set it false or delete the button before release. |
| `LTT.DisableBarks [1/0]` | §7. |

## 9. Planned / Cut

The original plan in this doc was not built. Not in `Source/`, `Scripts/` or `Content/`:

- **Messifier** Python tool, `ALTTPropBase` with PropID / zone / size class / group, `LTT.ValidateProps`. Replaced by the runtime earthquake (§2).
- **Zones, group lamps, Set Ready %, zone stage lights, music layers per zone.** Progress is a single job count.
- **Carry stacks, size classes, "Right it" / "Push back" holds.** One prop at a time; tipped furniture is just picked up.
- **Sweep piles / broom, cable untangle, stage-light aiming, film reel shelf, costume rack, lens wipe, clapperboard.** Not built (the barker reuses some of those lines for books, pillows, sockets and the camera, see `05 §1`).
- **Placement streak chime / COMBO HUD.** Only the streak barks exist.
- **Crew credits, Notebook, skill tree, active abilities (Director's Eye, Grab Bag, Set Dresser's Touch), golden reels, sprint unlock.**
- **Stars**, star-based end-screen quotes, "You're Fired!" exit ending, zone hint barks, jump-spam / on-table / stare barks, idle hint outline at 90 s.
- **Mess stories** (pizza tower, chair throne...). The mess is random.
- **Save game** with Phase 1 results.
