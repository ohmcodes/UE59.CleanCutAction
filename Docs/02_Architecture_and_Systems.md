# Architecture & Core Systems

Covers checklist **P1**. All C++ lives in module `CleanCutAction` (runtime). Editor-only Python scripts live in `Scripts/Python/`. Every C++ class gets a thin Blueprint child (`BP_*`) where assets (meshes, sounds, widgets, curves) are assigned.

---

## 1. Class Map

| Class | Type | Responsibility |
|---|---|---|
| `ULTTGameInstance` | GameInstance | Owns subsystems; holds `PendingPhaseEntry` info between levels |
| `ULTTSaveSubsystem` | GameInstanceSubsystem | Load/save/reset `ULTTSaveGame` |
| `ULTTLevelFlowSubsystem` | GameInstanceSubsystem | Fade + OpenLevel between Menu / Phase 1 / Phase 2 |
| `ULTTTweenSubsystem` | GameInstanceSubsystem + FTickableGameObject | All tweens (actors + widgets) |
| `ULTTEasing` | BlueprintFunctionLibrary | Easing math |
| `ALTTGameMode_Menu/Phase1/Phase2` | GameModeBase | Spawn rules, HUD class, phase start/end |
| `ALTTPlayerController` | PlayerController | Input contexts, pause, widget stack |
| `ALTTPlayerCharacter` | Character | First-person movement, camera, hand stack, components below |
| `ULTTInteractionComponent` | ActorComponent | Focus trace, outline, prompts, hold progress |
| `ULTTCarryComponent` | ActorComponent | Carrying, stacking, drop, throw |
| `ULTTSkillComponent` | ActorComponent | Unlocks, passive stat mods, active ability cooldowns |
| `ULTTFeedbackComponent` | ActorComponent | One-call juice: tween + SFX + VFX |
| `ILTTInteractable` | Interface | Contract for anything the player can use |
| `ULTTBarkSubsystem` | WorldSubsystem | Director/actor line selection and playback |
| `ULTTCleanProgressSubsystem` | WorldSubsystem | Phase 1 totals, groups, zones, completion (see `03`) |
| `ALTTSceneDirector` | Actor | Phase 2 beat machine (see `04`) |

---

## 2. Level Flow

```
L_MainMenu ──New Game / Start Cleanup──▶ L_Phase1_MessySet ──Roll Camera!──▶ L_Phase2_CafeShoot
     ▲                                        │ (Main Menu)                         │ (Main Menu / end)
     └────────────────────────────────────────┴─────────────────────────────────────┘
```

`ULTTLevelFlowSubsystem`:

```cpp
UFUNCTION(BlueprintCallable) void GoToMainMenu();
UFUNCTION(BlueprintCallable) void GoToPhase1();
UFUNCTION(BlueprintCallable) void GoToPhase2();
// internal:
void TravelTo(FName MapName); // 1) bTravelling guard 2) unpause 3) fade to black 0.4s (WBP_Fade, tween) 4) OpenLevel 5) new map's GameMode fades in 0.6s on BeginPlay
```

Rules:
- Always unpause and restore `FInputModeGameOnly` (or UI-only for menu) on arrival.
- Fade widget is added at Z-order 1000 so it's above everything.
- Phase GameModes call `SaveSubsystem->Save()` only at phase end (not on load).

---

## 3. Save System

### Schema - `ULTTSaveGame`

```cpp
UPROPERTY() int32 SaveVersion = 1;

// Progress
UPROPERTY() bool  bPhase1Complete = false;
UPROPERTY() float Phase1BestTimeSeconds = 0.f;   // 0 = none
UPROPERTY() int32 Phase1BestStars = 0;
UPROPERTY() bool  bPhase2Complete = false;
UPROPERTY() int32 Phase2BestScore = 0;
UPROPERTY() int32 Phase2BestStars = 0;

// Persistent unlocks (survive Phase 1 replays; reset by New Game)
UPROPERTY() int32 UnspentCrewCredits = 0;
UPROPERTY() TArray<FName> UnlockedSkills;          // e.g. "Skill.StackEm.2"
UPROPERTY() TArray<FName> FoundGoldenReels;        // "Reel.1".."Reel.3"

// Settings (survive New Game)
UPROPERTY() float MasterVolume = 1.f, MusicVolume = 0.8f, SFXVolume = 1.f, VoiceVolume = 1.f, UIVolume = 0.8f;
UPROPERTY() float MouseSensitivity = 1.f;
UPROPERTY() bool  bInvertY = false;
UPROPERTY() float FieldOfView = 90.f;
UPROPERTY() bool  bHeadBob = true;
UPROPERTY() bool  bSubtitles = true;
UPROPERTY() int32 SubtitleSize = 1; // 0 small, 1 medium, 2 large
UPROPERTY() bool  bScreenShake = true;
UPROPERTY() int32 QualityPreset = 2; // maps to GameUserSettings overall scalability
```

### Subsystem API - `ULTTSaveSubsystem`

```cpp
static const FString SlotName = TEXT("LTT_Save"); static const int32 UserIndex = 0;
virtual void Initialize(FSubsystemCollectionBase&) override; // load or create
UFUNCTION(BlueprintCallable) void Save();          // AsyncSaveGameToSlot
UFUNCTION(BlueprintCallable) void NewGame();       // reset progress + unlocks, keep settings, Save()
UFUNCTION(BlueprintPure) bool IsPhase1Complete() const;
UFUNCTION(BlueprintPure) bool HasAnyProgress() const; // drives "Continue" button
UFUNCTION(BlueprintCallable) void ReportPhase1Result(float TimeSeconds, int32 Stars);
UFUNCTION(BlueprintCallable) void ReportPhase2Result(int32 Score, int32 Stars);
UFUNCTION(BlueprintCallable) void ApplySettings(); // pushes volumes to SoundMix, sensitivity to PC, FOV to camera, quality to GameUserSettings
DECLARE_MULTICAST: OnSettingsChanged
```

Rules:
- Load synchronously in `Initialize` (tiny file). Save asynchronously.
- If `SaveVersion` is older, migrate fields; if the file is corrupt, create fresh and log a warning.
- Save points: phase completion, skill purchase, golden reel found, settings Apply/Back, New Game.
- **Phase 1 mid-progress is never saved.** Quitting mid-phase restarts the mess next time (unlocks and reels are kept).

---

## 4. Player Character (First Person)

Components:
```
ALTTPlayerCharacter
 ├─ CapsuleComponent (r=34, hh=88)
 ├─ CameraComponent "FPCamera" (Z=+77 from capsule center → ~165 cm eyes)
 │    └─ SceneComponent "HandStack" (offset X=45, Y=18, Z=-28)   ← carried items attach here
 │    └─ SceneComponent "ToolSocket" (offset X=40, Y=25, Z=-30)  ← broom
 ├─ ULTTInteractionComponent
 ├─ ULTTCarryComponent
 ├─ ULTTSkillComponent
 └─ ULTTFeedbackComponent
```

Movement defaults (tunable in BP): walk 450, sprint 750 (requires `Skill.Sprint`), accel 2400, braking 2800, jump Z 420, air control 0.3, step height 45, walkable floor 46°.

Hand stack sway: `HandStack` relative rotation follows look delta with a spring (stiffness 120, damping 14), clamped ±6°.

Head bob: sine on camera Z (1.2 cm × speed ratio), frequency 1.8 Hz walk / 2.4 Hz sprint; off if `bHeadBob=false`.

---

## 5. Interaction Framework

### Interface - `ILTTInteractable`

```cpp
UINTERFACE(BlueprintType) class ULTTInteractable : public UInterface { GENERATED_BODY() };
class ILTTInteractable {
  GENERATED_BODY()
public:
  UFUNCTION(BlueprintNativeEvent) bool CanInteract(AActor* Instigator) const;
  UFUNCTION(BlueprintNativeEvent) FText GetPromptText(AActor* Instigator) const;   // "Pick up", "Sweep", "Aim light"
  UFUNCTION(BlueprintNativeEvent) ELTTInteractKind GetInteractKind() const;          // Press, Hold, Station
  UFUNCTION(BlueprintNativeEvent) float GetHoldDuration() const;                     // for Hold
  UFUNCTION(BlueprintNativeEvent) void OnFocusBegin(AActor* Instigator);
  UFUNCTION(BlueprintNativeEvent) void OnFocusEnd(AActor* Instigator);
  UFUNCTION(BlueprintNativeEvent) void OnInteract(AActor* Instigator);              // Press complete / Hold complete
  UFUNCTION(BlueprintNativeEvent) void OnHoldTick(AActor* Instigator, float Alpha); // live hold feedback
  UFUNCTION(BlueprintNativeEvent) void OnHoldCancelled(AActor* Instigator);
};
```

### `ULTTInteractionComponent`

- Every tick: line trace from camera, 300 cm, channel `Interact`, sphere radius 6 cm (forgiving aim).
- Focus target = first hit actor implementing `ILTTInteractable` whose `CanInteract` is true.
- On focus change: old `OnFocusEnd`, new `OnFocusBegin`; set custom depth stencil on all primitive components of the focused actor (`SetRenderCustomDepth(true)`, stencil value **1** = focus white).
- Broadcast `OnFocusChanged(Actor, PromptText, Kind)` → HUD prompt tween.
- `IA_Primary` (LMB) routing order:
  1. If carrying and aiming at a valid ghost → **Place**.
  2. If focused interactable is a pickup and carry has room → **Pick up**.
  3. If carrying and nothing valid → **Drop** in front (gentle place on surface hit, else drop).
- `IA_Secondary` (RMB) held while carrying → charge throw; release → throw top item.
- `IA_Interact` (E) → stations, holds (right a table, aim light, wipe lens, plug cable).
- Hold interactions: progress 0→1 over `GetHoldDuration()`; release early cancels. HUD shows a radial fill around the crosshair.

### Stencil values (shared with `PP_Outline`)

| Stencil | Color | Used for |
|---|---|---|
| 1 | White | Focused interactable |
| 2 | Gold | Ghost of the item you carry / correct target |
| 3 | Cyan | Director's Eye highlights |
| 4 | Red | Frame-violation flash / wrong spot |
| 5 | Green | Active cue station (Phase 2) |

---

## 6. Carry Component

```cpp
UPROPERTY(EditAnywhere) int32 BaseCapacity = 3;       // + Stack 'Em ranks + golden reel bonus
UPROPERTY(VisibleAnywhere) TArray<ALTTPropBase*> Stack;
bool CanPickUp(ALTTPropBase* Prop) const;   // Small: if Stack.Num() < Capacity and top isn't Medium; Medium: only if Stack empty; Large/Fixed: never
void PickUp(ALTTPropBase* Prop);            // see sequence below
ALTTPropBase* PopTop();
void DropTop(const FHitResult& AimHit);
void ThrowTop(float Charge01);              // impulse = Lerp(400, 1400, Charge) * LongThrowMult (cm/s), +15° pitch up
int32 GetCapacity() const;
DECLARE events: OnStackChanged(int32 Count, int32 Capacity)
```

**Pick-up sequence (polish matters here):**
1. Disable physics, collision → `NoCollision`, set prop state `Carried`.
2. Pop tween on the prop in the world: scale 1 → 1.15 over 0.08 s (OutCubic).
3. Fly tween: world location → HandStack slot location over 0.18 s (OutCubic); scale → `CarryScale` (default 0.6 for Small, 0.8 for Medium).
4. Attach to `HandStack` (KeepWorld), then settle tween to the slot's relative transform 0.1 s (OutBack).
5. Play `SFX_Pickup_<PhysMat>`; crosshair pulse; HUD stack counter bounce.

**Stack layout:** item *i* sits at relative Z = Σ(previous heights × CarryScale) + 1 cm gap, with a random yaw ±8° fixed at pickup so the stack looks hand-made.

**Drop:** trace from camera 250 cm; if hit a walkable surface → tween to hit point + half height (0.15 s), then enable physics; else enable physics at hand with forward impulse 150.

**Large/Fixed props** are never carried; they use a Hold interaction ("Right it" / "Push back") defined in `03 §3.2`.

---

## 7. Tween System

Small, dependency-free. **Use it for everything** that moves for feedback; physics is only for messy loose props.

### Easing (`ULTTEasing`, t in [0,1])

```
Linear(t)      = t
InOutSine(t)   = -(cos(PI*t) - 1) / 2
OutCubic(t)    = 1 - (1-t)^3
OutBack(t)     = 1 + c3*(t-1)^3 + c1*(t-1)^2      with c1 = 1.70158, c3 = c1 + 1
OutElastic(t)  = t==0?0 : t==1?1 : 2^(-10t) * sin((t*10 - 0.75) * (2*PI/3)) + 1
OutBounce(t)   = standard 4-segment bounce (n1 = 7.5625, d1 = 2.75)
InBack(t)      = c3*t^3 - c1*t^2
Punch(t, f)    = sin(t * PI * f) * (1 - t)            // for wobble/shake, returns -1..1 decaying
```

### Subsystem API

```cpp
UENUM() enum class ELTTEase : uint8 { Linear, InOutSine, OutCubic, OutBack, OutElastic, OutBounce, InBack };
UENUM() enum class ELTTTweenClock : uint8 { Game, Real };  // Real = keeps running while paused (UI)

int32 TweenScale   (USceneComponent* C, FVector From, FVector To, float Dur, ELTTEase E, ELTTTweenClock Clock = Game, TFunction<void()> OnDone = nullptr);
int32 TweenLocation(USceneComponent* C, FVector From, FVector To, float Dur, ELTTEase E, bool bWorld=true, ...);
int32 TweenRotation(USceneComponent* C, FRotator From, FRotator To, float Dur, ELTTEase E, ...); // uses Quat slerp
int32 TweenFloat   (UObject* Owner, float From, float To, float Dur, ELTTEase E, TFunction<void(float)> OnUpdate, ...);
int32 TweenWidgetScale  (UWidget* W, FVector2D From, FVector2D To, float Dur, ELTTEase E, ELTTTweenClock Clock = Real, ...);
int32 TweenWidgetOpacity(UWidget* W, float From, float To, float Dur, ELTTEase E, ...);
int32 TweenWidgetTranslation(UWidget* W, FVector2D From, FVector2D To, float Dur, ELTTEase E, ...);
void  Cancel(int32 Handle);
void  CancelAllFor(UObject* Target);  // called before starting a new tween of the same property on the same target (prevents fighting)
// Blueprint wrappers return handle + latent "Completed" exec pin.
```

Implementation notes:
- Store tweens in a `TArray<FLTTTween>` with weak pointers to targets; skip/remove if target is gone.
- `Tick(DeltaTime)`: Game clock uses world delta (×time dilation, stops when paused). Real clock uses `FApp::GetDeltaTime()` unscaled.
- `IsTickableWhenPaused()` returns true; Game-clock tweens check `World->IsPaused()` and skip.
- **Auto-cancel conflicting tweens:** starting a Scale tween on a component cancels its previous Scale tween (snap to current value, not target).

### Feedback presets (`ULTTFeedbackComponent`)

| Preset | Spec |
|---|---|
| `Pop` | scale ×1 → ×1.15 (0.08 s OutCubic) → ×1 (0.15 s OutBack) |
| `Squash` (placement) | scale (1,1,1) → (1.18,1.18,0.82) 0.06 s OutCubic → (0.95,0.95,1.06) 0.08 s → (1,1,1) 0.22 s OutElastic |
| `Wobble` (wrong / denied) | yaw Punch ±7°, 4 cycles, 0.35 s |
| `Bob` (idle collectibles) | Z ±6 cm InOutSine loop 1.6 s + yaw 45°/s |
| `PowerOn` (stage lamps) | intensity 0 → 120% (0.1 s) → 30% (0.05 s) → 100% (0.25 s OutCubic), with a flicker sound |

All presets use the original scale/rotation as base (cached on first use) so repeated triggers never drift.

---

## 8. Gameplay Tags

```
Prop.Type.Chair, Prop.Type.Stool, Prop.Type.Table, Prop.Type.Lamp, Prop.Type.Cup, Prop.Type.Plate, Prop.Type.Vase,
Prop.Type.Plant, Prop.Type.Menu, Prop.Type.Frame, Prop.Type.Cushion, Prop.Type.Book, Prop.Type.Sign, Prop.Type.Misc
Prop.Size.Small, Prop.Size.Medium, Prop.Size.Large, Prop.Size.Fixed
Task.Place, Task.Trash, Task.Sweep, Task.Cable, Task.Light, Task.Reel, Task.Costume, Task.Lens, Task.Clapper
Zone.Entrance, Zone.Seating, Zone.Kitchen, Zone.Stairs, Zone.Terrace, Zone.Crew, Zone.Craft
Skill.Sneakers, Skill.StackEm, Skill.WideBroom, Skill.DirectorsEye, Skill.GrabBag, Skill.SetDresser,
Skill.CueSense, Skill.SteadyHands, Skill.LongThrow, Skill.Patience, Skill.DodgeShot, Skill.Sprint, Skill.ReelCarry, Skill.ReelCooldown
Cue.Slate, Cue.Traffic, Cue.Door, Cue.Steam, Cue.Napkins, Cue.Thunder, Cue.Sunset, Cue.Wind, Cue.Boom, Cue.Rain, Cue.Umbrella, Cue.Roses, Cue.Moonlight
Station.Slate, Station.CarRail, Station.DoorBell, Station.Espresso, Station.PropTable1F, Station.PropTable2F, Station.LightRig, Station.Thunder, Station.Fan, Station.Boom, Station.Rain
Bark.* (see 05 §0)
```

---

## 9. Director Bark System (shared by both phases)

### Data - `FLTTDirectorLine` (DataTable row, `DT_DirectorLines`)

| Column | Type | Notes |
|---|---|---|
| RowName | FName | Unique ID, e.g. `P1_Place_03` |
| Trigger | FGameplayTag | e.g. `Bark.P1.Place.Correct` |
| Speaker | enum | Marlowe, Dash, Juniper, Walkie_Dash, Walkie_Juniper |
| Text | FText | Subtitle |
| Voice | SoftObjectPtr USoundBase | Optional VO; if null → voice blips |
| Priority | int32 | 0 Idle, 1 Reaction, 2 Milestone, 3 Story/Tutorial (interrupts lower) |
| Chance | float | 0–1, roll per trigger |
| Cooldown | float | Seconds before this *trigger* may fire again |
| bOneShot | bool | Plays once per session |
| FollowUp | FName | Next line to auto-queue (dialogue chains) |
| MinProgress / MaxProgress | float | Optional % gates (Phase 1) |

### Rules
- `ULTTBarkSubsystem::Trigger(FGameplayTag Trigger, int32 PriorityOverride = -1)`.
- Global reaction cooldown **6 s** after any line ends (Priority ≤ 1 only).
- Per-trigger cooldown from the row; per-line no-repeat: shuffle bag per trigger (don't repeat a line until all lines of that trigger have played).
- Priority 3 interrupts anything lower (stop current audio, fade subtitle). Priority 2 waits in a queue of size 1 (newest replaces older queued).
- Line duration = VO length, or `max(1.6, words × 0.32 s)` for blips.
- Megaphone lines route to `SMX_Megaphone`; walkie lines get squelch in/out SFX + band-pass.
- Speaking events: `OnLineStarted(Speaker, Text, Duration)` → subtitle widget + Marlowe megaphone montage + Phase 2 actor talk montage.
- Duck music −6 dB while any line plays.

---

## 10. Pause

- `IA_Pause` in every mapping context (including Station and UI) → `ALTTPlayerController::TogglePause()`.
- Pause blocked during level travel fades and during the final 1 s of an end-screen stamp sequence (prevents broken tweens).
- On pause: `SetGamePaused(true)`, push `IMC_UI`, show `WBP_PauseMenu`, enable `SMX_Pause` low-pass, set input mode `GameAndUI` with mouse cursor.
- On resume: reverse; restore previous mapping contexts (stack-based so Station mode resumes correctly).
- Barks pause with the game (audio components set `bIsUISound=false` so they pause); UI sounds use `bIsUISound=true`.
