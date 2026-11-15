# Architecture & Core Systems

Describes the game as built at `windows` HEAD (87eadcc). The source in `Source/CleanCutAction/` is the truth; file names are given so every claim can be checked.

All C++ lives in the runtime module `CleanCutAction`. The template base classes keep the `CleanCutAction*` names; everything written for the jam uses the `LTT` prefix. Many classes build their own defaults in C++ and work without a Blueprint; where a Blueprint exists it only assigns assets or a designer layout.

---

## 1. Class Map

### Framework (template classes, extended)

| Class | File | Responsibility |
|---|---|---|
| `ACleanCutActionCharacter` | `CleanCutActionCharacter.h/.cpp` | First-person character: arms mesh + camera on the `head` bone, move/look/jump, crouch toggle, gamepad look rates, emote (dance) |
| `ACleanCutActionPlayerController` | `CleanCutActionPlayerController.h/.cpp` | Mapping contexts, pause menu, touch overlay, input-device prompt swapping, Unstuck teleport, input-mode helpers for end screens |
| `ACleanCutActionGameMode` | `CleanCutActionGameMode.h` | Empty template game mode |
| `ACleanCutActionCameraManager` | `CleanCutActionCameraManager.h` | Template camera manager (pitch limits) |

Blueprints in use (`/Game/FirstPerson/Blueprints/`): `BP_FirstPersonGameMode` (the `GlobalDefaultGameMode` in `Config/DefaultEngine.ini`, also set on both maps), `BP_FirstPersonCharacter`, `BP_FirstPersonPlayerController`. The game instance is the engine's plain `UGameInstance` (`GameInstanceClass=/Script/Engine.GameInstance`).

### Interaction and Phase 1 (`Interaction/`)

| Class | Type | Responsibility |
|---|---|---|
| `ILTTInteractable` | Interface | Contract for anything the player can focus and use (§5) |
| `ULTTInteractionComponent` | ActorComponent | Focus trace, place hints, Interact / Throw input routing |
| `ULTTCarryComponent` | ActorComponent | Holds one prop, place / drop / throw, hand IK targets |
| `ULTTFirstPersonAnimInstance` | AnimInstance | Parent of the arms Anim BP; reads hand IK from the carry component |
| `ALTTInteractableBase` | Actor | Every pickup prop: home transform, ghost silhouette, trash / dish / mop flags, breaking. Master BP `BP_MasterInteractables` |
| `ALTTMessDirector` | Actor | Phase 1 flow: records homes, "earthquake" mess, score, countdown, hints, briefing, HUD, ending shots, end screen, next level |
| `ALTTCleanupBarker` | Actor | Marlowe's Phase 1 megaphone lines (lines are in C++, shown in `ULTTBarkWidget`) |
| `ALTTWaterPuddle` | Actor | Spill: slows the player, cleaned with the mop, counts as a cleanup job |
| `ULTTWashStationComponent` | SceneComponent | Sink basin that washes dishes (`BP_Sink`) |
| `ULTTUsableComponent` → `ULTTApplianceComponent`, `ULTTCoffeeMachineComponent` | ActorComponent | Makes a placed prop usable (microwave, espresso machine) |
| `ALTTLightFixture`, `ULTTLightSwitchComponent`, `ULTTFlickerComponent` | Actor / components | Switchable wall and ceiling lamps, LED sign flicker |
| `ULTTPowerSubsystem`, `ALTTPowerBox` | WorldSubsystem / Actor | Mains power and the electric box (§9) |
| `ULTTPropSoundSet` | DataAsset | Pick-up / place / break sounds per prop material (`/Game/_CleanCutAction/SFX/Props/DA_PropSounds`) |
| `ALTTCheatButton` | Actor | Dev button that runs `LTT.PlaceAll` |
| `ULTTCleanupHUD`, `ULTTEndScreen` | UserWidget | Phase 1 HUD and end screen (see `06`) |

### Phase 2 (`Shoot/`)

`ALTTSceneDirector` (beat machine, score, take meter, CUT), `ULTTSceneData` (scene script; built-in Scene 42 when none is assigned), `ALTTEffectStation` (one class for every station mini-game), `ALTTFilmCamera` (frame cone), `ALTTFilmActor` + `ALTTActorMark`, `ULTTShootHUD`, `ULTTShootEndScreen`. Details are in `04`.

### UI, audio, world (`UI/`, `Audio/`, `World/`)

| Class | Type | Responsibility |
|---|---|---|
| `ULTTInputDeviceSubsystem` | GameInstanceSubsystem | Tracks keyboard/mouse vs gamepad vs touch; key labels; gamepad menu focus (§4.3) |
| `ULTTLeaderboardSubsystem` | GameInstanceSubsystem | Online leaderboard client and player name / id (§11) |
| `ULTTBriefingScreen`, `ULTTPauseMenu`, `ULTTLeaderboardPanel`, `ULTTVirtualKeyboard`, `ULTTEndingOverlay`, `ULTTMusicToggle`, `ULTTBarkWidget` | UserWidget | See `06` |
| `LTTTheme` (namespace) | Header helpers | Shared colours, brushes, `PreferWBP` (`UI/LTTUITheme.h`) |
| `ULTTWidgetBaker` | BlueprintFunctionLibrary | Editor tool: copies a C++ widget layout into its `WBP_` child |
| `ALTTMusicPlayer` | Actor | Shuffled background music with crossfades; global mute |
| `ALTTMirror` | Actor | Scene-capture mirror |

---

## 2. Levels and Level Flow

| Map | Role |
|---|---|
| `/Game/StylizedNewYorkCity/Level/L_NYC_Cafe_Michael` | Phase 1 (cleanup). `GameDefaultMap` and the editor startup map |
| `/Game/StylizedNewYorkCity/Level/L_NYC_Cafe_Phase2_Michael` | Phase 2 (the shoot), in `MapsToCook` |

There is no main-menu map. The game boots straight into Phase 1, where the briefing screen acts as the title/start screen.

```
L_NYC_Cafe_Michael ──end screen "Shoot the Scene!" (set clean only)──▶ L_NYC_Cafe_Phase2_Michael
   ▲  Play Again / pause Restart reload the same map                        │ Shoot Again reloads it
   └───────────────────────────── end screen "Back to Cleanup" ─────────────┘
```

- Phase 1 → 2: `ALTTMessDirector::NextLevelName` (default `L_NYC_Cafe_Phase2_Michael`), opened by `LoadNextLevel()` from `ULTTEndScreen`. Empty name hides the button.
- Phase 2 → 1: `ALTTSceneDirector::CleanupLevelName` (default `L_NYC_Cafe_Michael`), opened from `ULTTShootEndScreen`.
- Travel is a plain `UGameplayStatics::OpenLevel` (no fade subsystem). `ACleanCutActionPlayerController::ResetGameInput` clears pause, ignore-input flags and UI-only input left over from the end screen; it runs before travel (`ALTTMessDirector::LoadNextLevel`, `ULTTShootEndScreen`) and again in `ALTTSceneDirector::BeginPlay`.
- No progress carries between levels (no save game, see §12).

---

## 3. Player Character

`ACleanCutActionCharacter` (`CleanCutActionCharacter.cpp`), with `BP_FirstPersonCharacter` holding the meshes (`SKM_Rookie`, `ABP_FP_Copy_Rookie`, `ABP_Unarmed_Rookie`), input actions, `ULTTInteractionComponent`, `ULTTCarryComponent` and the `M_PP_Outline` post-process.

- Capsule 34 × 96. First-person arms mesh (owner-only) with the camera on its `head` bone; first-person FOV 70, first-person scale 0.6. The full body mesh is hidden from the owner but seen by mirrors and other cameras.
- Air control 0.5, falling braking 1500.
- **Crouch** toggle: half height 48, crouched speed 250. The arms mesh is lowered so the view drops (`CrouchViewSpeed`). Keys from `CrouchKeys` (C, Left Ctrl, right-stick click) on a runtime action/context unless `CrouchAction` is set; touch calls `DoCrouchToggle()`.
- **Gamepad look** (`GamepadLookInput`): right stick scaled to `GamepadYawRate` 140°/s and `GamepadPitchRate` 95°/s with a response curve (`GamepadLookExponent` 1.8), compensating for `bEnableLegacyInputScales`. Mouse look is unchanged.
- **Emote (dance)**: `IA_Emote` (`/Game/_CleanCutAction/Input/IA_Emote`) on a runtime context (G, D-pad Up; touch button injects it). Plays `A_Emote_Rookie` on the body mesh in single-node mode, pauses `ALTTMusicPlayer` and plays the `paroparo` song; pressing again cancels. Both assets are loaded by path and listed in `MapsToCook`.

Not built from the plan: sprint, head bob, hand-stack sway, skill/feedback components.

---

## 4. Input

### 4.1 Contexts and actions

| Asset / runtime object | Where added | Contents |
|---|---|---|
| `/Game/Input/IMC_Default` | `BP_FirstPersonPlayerController.DefaultMappingContexts` | `IA_Move` (WASD / arrows, left stick), `IA_Look` (right stick), `IA_Jump` (Space, gamepad A), `IA_Interact` (E, LMB, gamepad X), `IA_Throw` (RMB, RT) |
| `/Game/Input/IMC_MouseLook` | `MobileExcludedMappingContexts` (skipped on touch) | `IA_MouseLook` (Mouse2D) |
| `IMC_Pause_Runtime` (priority 2) | `ACleanCutActionPlayerController::SetupInputComponent` | Runtime `IA_Pause_Runtime` (`bTriggerWhenPaused`) for `PauseKeys` P, Escape, gamepad Menu |
| `IMC_Crouch_Runtime` (priority 1) | `ACleanCutActionCharacter::SetupPlayerInputComponent` | C, Left Ctrl, right-stick click |
| `IMC_Emote_Runtime` (priority 1) | same | `IA_Emote` on G, D-pad Up |

The Interact/Throw key list above was read from the `IMC_Default` asset and matches the in-game controls list (`LTTBriefingScreen.cpp`, `AddControlRow` calls). There are no Station or UI mapping contexts.

Keys read directly (no Input Action asset):
- Hint: `ALTTMessDirector::HintKey` F / `HintGamepadKey` gamepad Y (runtime action unless `HintAction` is set).
- HUD controls card toggle: `ULTTCleanupHUD::ToggleControlsKey` H / D-pad Down.
- Phase 2 stations poll keys with `IsInputKeyDown` (`LTTEffectStation.cpp`): LMB / RT = action, E / RMB / gamepad B = leave, W / left stick up = push, A/D or LB/RB or the stick = shake, 1–5 / mouse wheel / D-pad / bumpers = pick a choice, F / Y = thunder flash.
- Menus handle their own keys in `NativeOnKeyDown` (Enter/A, Escape/B, etc.).

### 4.2 Touch

`ShouldUseTouchControls()` is true on touch platforms (`SVirtualJoystick::ShouldDisplayTouchInterface`) or with `bForceTouchControls`. The PC then spawns `MobileControlsWidgetClass` (`/Game/Input/Touch/UI_TouchSimple`: sticks, hand, throw, jump, crouch, hint, dance, pause) and `WBP_MusicToggle`. `RestoreGameInput()` keeps Game+UI input and the cursor on touch so the buttons still take taps.

### 4.3 Input device tracking (`UI/LTTInputDevice.h/.cpp`)

`ULTTInputDeviceSubsystem` registers a Slate input pre-processor and records the last device used: `KeyboardMouse`, `Gamepad` or `Touch` (process-wide, `GetCurrent()`), broadcasting `OnInputDeviceChanged`.

- Start guess: touch if the touch interface is shown, gamepad if one is attached, else keyboard/mouse.
- Mouse movement alone never switches away from the gamepad (Steam Input trackpads move the mouse).
- **Sticky gamepad** (87eadcc): for 1.5 s after any gamepad input, mouse clicks and keys don't switch away from the gamepad (Steam Input sends some buttons and trackpad clicks as mouse/keyboard). Every switch is logged with its cause.
- Helpers: `GamepadKeyLabel` (Xbox / Steam Deck names: "A", "RT", "D-pad Up", "Menu"), `KeyLabelFor`, `Pick`, `FocusFirstButton`, `EnsureGamepadFocus` (menus call it from `NativeTick` so the D-pad / stick and A work), `IsBackKey` (B).

`ACleanCutActionPlayerController::ApplyInputDevice` (on change and on a 0.25 s timer) hides the touch overlay while a gamepad is used on a touch device and swaps the labels in `WBP_InteractPrompt` (E + mouse icons ↔ X, plus an RT cap on the throw row). The HUD, briefing and pause menu swap their own key lists (see `06`).

---

## 5. Interaction Framework

### `ILTTInteractable` (`Interaction/LTTInteractable.h`)

BlueprintNativeEvents: `CanInteract`, `GetPromptText`, `GetInteractKind`, `GetHoldDuration`, `OnFocusBegin`, `OnFocusEnd`, `OnInteract`. `ELTTInteractKind` = `Press`, `Hold`, `Station` (Phase 2 effect stations), `Pickup` (carried). There is no hold-tick or hold-cancel event.

Implemented by `ALTTInteractableBase`, `ALTTPowerBox`, `ALTTLightFixture`, `ALTTWaterPuddle`, `ALTTCheatButton`, `ALTTEffectStation`.

### `ULTTInteractionComponent`

- Each tick: sphere trace from the view, `TraceDistance` 300, `TraceRadius` 6, `ECC_Visibility`. The trace passes through up to 8 already-placed props. `OnFocusChanged(Actor, PromptText)` feeds the prompt.
- Focus outline: custom depth stencil **1** on the focused mesh, drawn by `M_PP_Outline`. Stencil 1 is the only value in use.
- `IA_Interact` and `IA_Throw` are bound on the pawn's input component (`TryBindInput`).
- **Interact** routing (`Interact()`), using `ELTTPlaceHint`:
  1. Holding a prop: `Place` (aiming at its home silhouette within `PlaceDistance` 400) → lock it home; `Mop` → one mop stroke on the aimed puddle (mop stays in hand, reach 260); `Trash` → into the aimed bin; `Wash` → into the aimed sink; otherwise **Drop**. `NeedsSupport` (its table isn't back) drops and triggers a "wrong order" bark; `Fumble` (aim within `NearMissMargin` 35 cm outside the snap) drops and arms the prop to break on landing.
  2. Not holding: a `Pickup` focus is picked up; anything else gets `OnInteract`.
- **Throw** (`ThrowHeld`) throws the held prop. Impacts break props through the Blueprint `BreakFromImpact` event → `ALTTInteractableBase::BreakAway` (the controls list says "Throw (breaks it!)"). A prop thrown slowly into its own silhouette is placed instead (trick shot, `TrickShotMaxSpeed`).

---

## 6. Carry Component

`ULTTCarryComponent` holds **one** prop (no stack, no capacity).

- `PickUp` flies the prop into the hold position over `PickupBlendTime` 0.18 s (OutCubic, inline math). Hold position: `BaseHoldDistance` 45, `HoldDownOffset` 18, one-handed props offset `OneHandSideOffset` 16 to the right.
- `Drop` (forward speed 150), `Throw` (`ThrowSpeed` 1100 × the prop's multiplier, 10° up), `PlaceHeld` (back home and locked), `PlayUseSwing` (mop stroke).
- The held mop's head aims at the targeted puddle point (`SetToolAim`).
- Hand IK: `GetRightHandIK` / `GetLeftHandIK` (`FLTTHandIK`) feed `ULTTFirstPersonAnimInstance`.
- Events: `OnPickedUp(Item)`, `OnReleased(Item, bThrown)` (the barker listens).

---

## 7. Phase 1 Flow (`ALTTMessDirector`)

Placed in `L_NYC_Cafe_Michael` with `WBP_Briefing`, `WBP_CleanupHUD` and `WBP_EndScreen` assigned. In order:

1. Starts on black, records every prop's home and support, then throws props around (the "earthquake"; `RandomSeed` 0 = new mess each play). Puddles count as extra cleanup jobs.
2. Shows the briefing (`BriefingClass`). Start = time trial (countdown `TimeLimitSeconds` 1800), No Timer = no clock (`SetTimerEnabled(false)`). Fades in.
3. Play: score = placed × 10 + broken × −15 + hints × `PointsPerHint` (0) + time bonus (1 per second left). Hint (F / Y) outlines the nearest placeable prop; repeat-hinted stuck props are rescued. A periodic rescue moves props that fell under the map or out of the cafe.
4. All placed (or time up): ending camera shots (Level Sequence, or level cameras), skippable with `ULTTEndingOverlay`, confetti, then `ULTTEndScreen`. The game is not paused behind the end screen: `KeepGameInputWithCursor` keeps walking enabled with the cursor up.

Details of the cleaning rules are in `03`.

---

## 8. Director Barks

**Phase 1:** `ALTTCleanupBarker` (placed next to the mess director). It listens to the mess director and the carry component, and to `NotifyEvent` (`Crash`, `TrashMiss`, `BinWrongItem`, `WrongOrder`). Lines are written in `LTTCleanupBarker.cpp` (LOCTEXT pools, not a DataTable), with priorities 0–3, chance rolls, cooldowns, no-repeat pools, chains, idle nags (`IdleSeconds` 40) and power reminders (`PowerNagSeconds` 45). A new line cuts in after `MinShowBeforeInterrupt` 1 s; cut-off story/milestone lines replay, cut-off reactions are dropped; `ReactionGap` 4 s between reactions. Duration = max(`MinLineSeconds` 2.4, words × 0.32) + 0.9 s. Speakers: `Marlowe`, `WalkieDash`, `WalkieJuniper`. Text only (no VO, no music ducking). `LTT.DisableBarks [1|0]` turns it off.

**Phase 2:** lines come from `ULTTSceneData` beats and Marlowe's cue calls, shown in `ULTTShootHUD` (subtitle + bark panel). See `04`/`05`.

---

## 9. Power and Lights

- `ULTTPowerSubsystem` (WorldSubsystem): `SetPowerOn(bool)`, `IsPowerOn()`, `OnPowerChanged`. Off darkens every point/spot/rect light and dims lamp bulbs; sky and directional lights are untouched. Lamp switches flipped during an outage apply when power returns.
- `ALTTPowerBox` (`BP_ElectricBox`): Interact toggles the mains with sparks, a relay click and crackle; a red indicator stays lit. `bStartsOff = true`, so each level starts with the breaker off and Marlowe nags until it's on.
- `ALTTLightFixture` (`BP_Lamp_*`) and props with `ULTTLightSwitchComponent` (e.g. `BP_Prop_ModernLamp`) switch on/off with Interact. `ULTTFlickerComponent` drives `BP_ShopSign_02_LED`.

---

## 10. Pause

- `ACleanCutActionPlayerController::TogglePauseMenu` on P / Escape / gamepad Menu (runtime action that triggers while paused) or the touch pause button.
- `OpenPauseMenu` does nothing if the game is already paused. It creates `PauseMenuClass` (`WBP_PauseMenu` on `BP_FirstPersonPlayerController`, default `ULTTPauseMenu`) at Z 20, calls `SetGamePaused(true)`, shows the cursor and sets UI-only input focused on the menu.
- `ClosePauseMenu` removes it, unpauses and calls `RestoreGameInput()`.
- Menu buttons: Resume, Music on/off, Leaderboard, Unstuck (`TeleportToSpawn`: back to the first possessed transform, uncrouched; a held prop comes along), Restart (reloads the map; the run isn't submitted), Quit. A small coffee icon runs `LTT.PlaceAll` only for the player named `iPoNet`.
- Background music (`ALTTMusicPlayer`) is a UI sound and keeps playing while paused; the emote song is not, so it pauses with the game.

---

## 11. Leaderboard and Persistence

`ULTTLeaderboardSubsystem` (`UI/LTTLeaderboardSubsystem.h/.cpp`):

- Server `BaseUrl` `https://cca.ohmcodes.com` (config, or `-LeaderboardUrl=`), page `/leaderboard`, `bEnabled` to switch off.
- Player identity: a GUID player id plus a name (random "Sleepy Barista 42"-style, or typed on the briefing; max 24 chars, sanitised). Stored in browser `localStorage` on Wasm, otherwise in a `ULTTPlayerProfileSave` save slot.
- `SubmitRun(Score, Time, bSetClean, bTimeTrial, Placed, Total, Broken, Hints)` is sent by `ULTTEndScreen` when the Phase 1 end screen opens. The server keeps the best score and the best clean-set time. Phase 2 results are not submitted.
- `FetchLeaderboard(Limit)` → `OnLeaderboardFetched` (top N + your row); `OpenFullLeaderboard()` opens the website.
- On Wasm the engine HTTP module is a stub, so requests go through the browser's `fetch()` on the main thread (`MAIN_THREAD_EM_ASM`) and are polled from the game thread; elsewhere `FHttpModule`.

The profile save is the only save data. There is no progress save.

---

## 12. Console Commands

| Command | File | Effect |
|---|---|---|
| `LTT.PlaceAll [LeaveCount=5]` | `LTTMessDirector.cpp` | Places every cleanup prop except LeaveCount (test the ending) |
| `LTT.DisableBarks [1\|0]` | `LTTCleanupBarker.cpp` | Turns Phase 1 barks off / on |
| `LTT.SkipCue [Perfect\|Good\|Late\|Miss]` | `LTTSceneDirector.cpp` | Resolves the active Phase 2 cue |
| `LTT.JumpToAct <A\|B\|C>` | same | Restarts the scene from an act |
| `LTT.Cut` | same | Calls CUT |

---

## 13. Planned but not built (cut for the jam)

Checked by grepping `Source/`; none of these exist:

- `ULTTGameInstance`, `ULTTLevelFlowSubsystem` (fades, `GoToMainMenu/Phase1/Phase2`), `L_MainMenu`.
- `ULTTSaveGame` / `ULTTSaveSubsystem` (completion flags, best times, unlocks, settings).
- `ULTTTweenSubsystem`, `ULTTEasing`, `ULTTFeedbackComponent`: widgets and props animate with inline easing in `Tick`/`NativeTick`.
- `ULTTSkillComponent`, Crew Credits, golden reels, abilities, sprint.
- `ULTTBarkSubsystem` and `DT_DirectorLines` (replaced by `ALTTCleanupBarker`), voice-over, music ducking.
- `ULTTCleanProgressSubsystem` and zones (the mess director does the counting).
- Gameplay tags (none are used), carry stacking, hold interactions with progress, stencil values 2–5, Station/UI mapping contexts, `ALTTPropBase`/`ALTTPlayerCharacter` names.
