# UI & Animation

Describes the UI as built at `windows` HEAD (87eadcc). Plain UMG, no CommonUI. Source of truth: `Source/CleanCutAction/UI/`, `Interaction/LTTCleanupHUD*`, `Interaction/LTTEndScreen*`, `Shoot/LTTShootHUD*`, `Shoot/LTTShootEndScreen*`.

---

## 1. How the widgets are built

- **C++ layouts.** Every `ULTT*` widget has a `BuildDefaultLayout()` that builds its tree in code when the widget has no designer tree, so the bare C++ class works on its own.
- **`WBP_` children.** Each class has a Widget Blueprint child in `/Game/_CleanCutAction/Blueprints/Widgets/` whose tree was copied from the C++ layout by `ULTTWidgetBaker` (editor-only: `BakeLayoutIntoWidgetBlueprint`, `BakeWidgetIntoWidgetBlueprint`, `RemoveWidgetFromWidgetBlueprint`). Widgets keep the names the C++ `BindWidgetOptional` properties expect, so designers can restyle without breaking code. When a newer C++ part is missing from an older `WBP_`, the code adds it at runtime (e.g. pause menu Leaderboard/Restart/Unstuck buttons, briefing crouch rows, HUD gamepad legend).
- **Which class is used.** `LTTTheme::PreferWBP<T>(Class, "WBP_Name")` swaps an unset or bare C++ class for the `WBP_` (music toggle, ending overlay, shoot HUD, shoot end screen, virtual keyboard). The leaderboard panel loads `WBP_Leaderboard` by path. The pause menu class is set on `BP_FirstPersonPlayerController`; the briefing, cleanup HUD and end screen classes are set on the mess director placed in `L_NYC_Cafe_Michael`. Widgets loaded only by path are listed in `MapsToCook` (`Config/DefaultGame.ini`).
- **Animation.** No tween subsystem and no UMG widget animations. Each widget keeps an `AnimTime` and eases in `NativeTick` with small inline functions (OutCubic, OutBounce, etc.). UMG ticks in real time, so menus animate while the game is paused.

### Widget list

| Widget asset | C++ class | Created by | Shown |
|---|---|---|---|
| `WBP_Briefing` | `ULTTBriefingScreen` | `ALTTMessDirector` (`BriefingClass`) | Phase 1 start, over black |
| `WBP_CleanupHUD` | `ULTTCleanupHUD` | `ALTTMessDirector` (`HUDClass`) | Phase 1 play |
| `WBP_InteractPrompt` | Blueprint only | `BP_FirstPersonPlayerController` | Both phases; labels set by the PC (§3) |
| (code layout) | `ULTTBarkWidget` | `ALTTCleanupBarker` | Phase 1 Marlowe lines |
| `WBP_EndingOverlay` | `ULTTEndingOverlay` | `ALTTMessDirector` | Phase 1 ending shots + confetti |
| `WBP_EndScreen` | `ULTTEndScreen` | `ALTTMessDirector` (`EndScreenClass`) | Phase 1 end |
| `WBP_ShootHUD` | `ULTTShootHUD` | `ALTTSceneDirector` | Phase 2 play |
| `WBP_ShootEndScreen` | `ULTTShootEndScreen` | `ALTTSceneDirector` | Phase 2 end |
| `WBP_PauseMenu` | `ULTTPauseMenu` | `ACleanCutActionPlayerController` | Both phases |
| `WBP_Leaderboard` | `ULTTLeaderboardPanel` | Briefing, pause menu, Phase 1 end screen | Over the menu that opened it |
| `WBP_VirtualKeyboard` | `ULTTVirtualKeyboard` | Briefing name box on touch | Over everything |
| `WBP_MusicToggle` | `ULTTMusicToggle` | `ACleanCutActionPlayerController` | Touch HUD only |
| `UI_TouchSimple` (`/Game/Input/Touch/`) | Blueprint | PC `MobileControlsWidgetClass` | Touch, hidden while a gamepad is used |

`/Game/_CleanCutAction/UI/` also holds older copies of `WBP_Briefing`, `WBP_CleanupHUD`, `WBP_EndScreen` and `WBP_InteractPrompt`. The maps and the controller reference the `Blueprints/Widgets/` versions.

---

## 2. Style (`UI/LTTUITheme.h`)

The shipped look is a **"cozy cafe"** theme, not the clapperboard/call-sheet look first planned: cream paper panels with gold trim, a wooden ribbon banner behind titles, beige pill buttons, warm brown text. Colours are sRGB hex in `LTTTheme`:

| Token | Hex | Use |
|---|---|---|
| `Paper` | `#FBF7EC` | Panel fill |
| `PaperDeep` | `#F2E9D6` | Insets (notes, rows, HUD chips) |
| `GoldTrim` | `#C9A15A` | Panel outline, ornaments |
| `Wood` / `WoodDark` | `#9A6633` / `#5A3415` | Title banner / its outline and title text outline |
| `Ink` / `InkMuted` | `#4A3018` / `#8C7356` | Body / secondary text |
| `TitleCream` | `#FFF8E6` | Title letters |
| `HeaderBrown` | `#7A4E22` | Section headers |
| `ButtonCream` / `ButtonGold` / `ButtonRed` | `#F5EAD3` / `#F4C45A` / `#E88B72` | Normal / primary / quit buttons |
| `ButtonEdge` | `#B08248` | Button outline |
| `Good` / `Bad` / `Coin` | `#5E943C` / `#C04A30` / `#C88A10` | Positive numbers / broken props, time up / points |
| `Backdrop` | `#2B1A0C` @ 60% | Dim behind menus |

Helpers: `StylePanel`, `StyleInset`, `StyleRibbonBanner` + `FillRibbonBanner`, `StyleBackdrop`, `StyleText`, `StyleTitle`, `StyleOverlayText` (cream with brown outline over the 3D view), `ButtonStyle` / `StyleButton`, `StyleTextBox`, `FitToScreen` (shrinks a card to fit small screens). Cartoon icons come from `/Game/_CleanCutAction/UI/Widget/` (clapperboard, megaphone, coffee, timer, lightbulb...), button icons from `/Game/_CleanCutAction/UI/Icons/` (`T_Icon_*`).

---

## 3. Input-device-aware prompts

All screens follow `ULTTInputDeviceSubsystem` (see `02 §4.3`): keyboard/mouse, gamepad (Xbox / Steam Deck names) or touch.

| Widget | What changes |
|---|---|
| `WBP_InteractPrompt` | PC `ApplyInputDevice`: `KeyText` "E" ↔ "X", mouse icons hidden on gamepad, an "RT" cap (`ThrowPadCap`) added to `RowThrow`; `bTouchMode` cleared when a gamepad drives a touch device |
| `ULTTCleanupHUD` | Controls card shows `ControlsList` (keyboard) or `ControlsListPad` (gamepad, baked into `WBP_CleanupHUD` in 87eadcc); hint prompt "[F] Hint" ↔ "[Y] Hint"; hidden on touch (touch has a hint button) |
| `ULTTBriefingScreen` | Controls page shows `ControlsDesktop` (Action / Keyboard / Gamepad columns) or `ControlsTouch` (icon rows); start hint text per device |
| `ULTTPauseMenu` | Footer "P / Esc to resume" ↔ "Menu / B to resume     A to select" |
| Menus | `EnsureGamepadFocus` puts focus on a button when a gamepad is used, so D-pad / stick + A work; B closes panels |
| Touch overlay | Hidden (with the music toggle) while a gamepad is used on a touch device |

---

## 4. Briefing (`WBP_Briefing`, `ULTTBriefingScreen`)

The start screen of the game (there is no main menu). Shown over black while the mess is made behind it.

- **Card** fades and rises in. Ribbon title, then a three-page **carousel** (`Pages` widget switcher): 1 "DIRECTOR'S NOTE" (Marlowe's story note), 2 "HOW TO PLAY", 3 "CONTROLS & SCORING" (controls table + scoring rows with coffee / broken cup / stopwatch icons, values from the mess director). Flip with the arrow buttons, Left/Right, the D-pad, or a swipe; page dots and "2 / 3  -  HOW TO PLAY" label.
- **Name row**: `NameInput` (starts as a random name), dice button (`RandomNameButton`) for a new random name, and a **Leaderboard** button. On touch, focusing the name box opens the virtual keyboard (§9).
- **Start buttons**: `StartButton` = time trial with the countdown (Enter / gamepad A); `NoTimerButton` = play without a clock (N / gamepad Y).

Controls listed (keyboard / gamepad): Move WASD/Arrows / left stick · Look mouse / right stick · Jump Space / A · Pick up/Place/Drop LMB or E / X · Throw RMB / RT · Crouch C or Ctrl / R3 · Hint F / Y · Dance G / D-pad Up · Pause P or Esc / Menu.

---

## 5. Phase 1 HUD (`WBP_CleanupHUD`, `ULTTCleanupHUD`)

| Element | Position | Behaviour |
|---|---|---|
| Stats chip | Top-left | `PlacedText` (placed / total), `DestroyedText`, `PointsText`, each with an icon |
| Countdown | Top-centre | `TimerText` + `TimerIcon`; the icon beats once a second, twice and bigger in the last `LowTimeSeconds`, and turns red when low. Hidden with No Timer |
| Hint prompt | Top-left, last row of the stats chip | `HintKeyText` "[F] Hint" ("[Y] Hint" on gamepad) with the `HintIcon` bulb, greyed with the seconds left while cooling down |
| Hint line | Under the crosshair | Prop name, distance and floor ("upstairs" / "downstairs") while a hint is up |
| Home arrow | Ring around the crosshair | Chevron toward the held prop's home, the hinted prop, the nearest puddle (holding the mop), or the electric box while the breaker is off; hidden when the target is in view |
| Pickup ring | Over a prop | Circle over the nearest pickable prop in range and view; pulses when near, steady when under the crosshair |
| Controls card | Bottom-left | Key list (keyboard or gamepad version); H / D-pad Down folds it to a "[H] Controls" tab |

The arrow, ring and countdown beat are painted in `NativePaint` / ticked in C++. The interact prompt (`WBP_InteractPrompt`) is a separate Blueprint widget.

**Bark widget** (`ULTTBarkWidget`, code layout): Marlowe's line in a gold pill at the bottom centre with a megaphone icon and speaker label ("MARLOWE", "JUNIPER (walkie)"); pops in on a new line and fades out when it ends. Restylable via `BarkPanel`, `BarkLabel`, `SpeakerLabel`.

---

## 6. Pause (`WBP_PauseMenu`, `ULTTPauseMenu`)

- Opened by the player controller (P / Esc / gamepad Menu, touch pause button); the game is paused and input is UI-only (`02 §10`).
- Card fades and drops in. Title "CUT!", subtitle "Game paused".
- Buttons: **Resume** (gold) · **Music: On/Off** · **Leaderboard** · **Unstuck (back to start)** · **Restart** · **Quit** (red). Small coffee icon (`CheatButton`) runs `LTT.PlaceAll` only for the player named `iPoNet`.
- P / Esc / Menu / B resume. Restart reloads the map without submitting the run.

---

## 7. Phase 1 ending and end screen

**Ending overlay** (`WBP_EndingOverlay`, `ULTTEndingOverlay`): a **SKIP** button while the ending camera shots play (Space / Enter / Esc / gamepad A / Menu also skip), then a confetti burst (`ConfettiCount` 220, `ConfettiLifetime` 4.5 s) from both bottom corners plus a shower from the top. Confetti is drawn in `NativePaint` (no textures, no Niagara) and keeps falling over the end screen.

**End screen** (`WBP_EndScreen`, `ULTTEndScreen`):
- Title "SET READY!" (set clean, `SW_next_phase`) or "TIME'S UP!" (`SW_miss`), with a subtitle line. The title drops in with a bounce (scale 2.5 → 1) and the points count up.
- Rows: Time on set · Props returned (placed / total) · Props destroyed (with points) · Hints used · Time bonus (+N, seconds left) · Points.
- `RankText`: "Sending …'s take to the leaderboard...", then "Leaderboard: #4 of 27 - New best score!" or an offline note. The run is submitted when the screen opens.
- Buttons: **Shoot the Scene!** (gold, set clean only → Phase 2) · **Play Again** (reload) · **Keep Playing** (close; after time's up the clock is switched off) · **Leaderboard** · **Full Leaderboard** (website).
- The game is **not** paused: `KeepGameInputWithCursor` lets the player walk the set with the cursor up for the buttons.

---

## 8. Leaderboard panel (`WBP_Leaderboard`, `ULTTLeaderboardPanel`)

Opened with `ShowPanel(From)` from the briefing, pause menu or end screen; hides the card underneath and returns focus to it on close.
- Card fades and rises in; rows slide in one after another.
- `StatusText` ("Loading...", "Top 10 of N Rookies", or the error), top `RowCount` (10) rows (rank chip, name, score, best time; your row highlighted), and `YouBox` with your own place when you're outside the top 10.
- Buttons: **Full Leaderboard** (opens the website in a new tab), **Retry**, **Close**, plus a round corner X (`CornerCloseButton`).

---

## 9. Virtual keyboard (`WBP_VirtualKeyboard`, `ULTTVirtualKeyboard`)

The Wasm build gets no phone keyboard when a Slate text box is tapped, so the briefing's name box opens this on touch.
- `ULTTVirtualKey` buttons (a `UButton` subclass with a `Value`; specials `{SHIFT}` `{BACK}` `{SPACE}` `{DONE}` `{CLEAR}`). Any key placed in the WBP is found at runtime.
- Types live into the target text box and shows its own preview line with a blinking caret (keys can cover the box). Shift starts on for the first capital; max 24 characters.
- The key panel slides up (`SlideDistance` 300, `SlideTime` 0.2 s). Done or a tap outside closes it and fires `OnClosed`. Physical keys work; on a gamepad a highlight cursor moves to the nearest key in the stick / D-pad direction.

---

## 10. Music toggle (`WBP_MusicToggle`, `ULTTMusicToggle`)

Touch only: a round speaker button left of `UI_TouchSimple`'s pause button (`T_Icon_MusicOn` / `T_Icon_MusicOff`, 70% opacity when muted). Follows `ALTTMusicPlayer::IsMusicMuted()`, so it stays in step with the pause menu's Music button.

---

## 11. Phase 2 HUD (`WBP_ShootHUD`, `ULTTShootHUD`)

| Element | Position | Widgets / behaviour |
|---|---|---|
| In-shot tint | Full screen | `Vignette`: red tint pulsing at 2 Hz while the player is in the camera's frame; `WarningText` |
| REC block | Top-left | `RecText` (REC light blinks once a second + timecode), `TakeText`, `ScoreText` |
| Take meter | Top-centre | `MeterBar` |
| Cue card | Top-right | `CuePanel`, `CueTitleText`, `CueWhereText`, `CueBar` countdown; "CUE: RAIN!" while active, "NEXT: …" while coming up |
| Centre texts | Centre | `ActCardText` (act titles), `CutText` (CUT interstitial), `ToastText` (grade + points) |
| Station | Centre / below crosshair | `InstructionText` and a painted mini-game meter (needle, band, sustain progress); `StationHintText` shows the station's name, distance and floor, with an arrow, while you're not at it |
| Bark + subtitles | Bottom | `BarkPanel` / `BarkLabel` (Marlowe, gold pill), then `SubtitlePanel` with `SpeakerText` / `LineText`; off-script lines are dimmer |

The station mini-games (Press, Timing, HoldRelease, Sustain, Shake, Choice, Toss) are in `ALTTEffectStation` (see `04`).

---

## 12. Phase 2 end screen (`WBP_ShootEndScreen`, `ULTTShootEndScreen`)

- "THAT'S A WRAP!", `StarsText` ("{n} / 3 STARS", "DIRECTOR'S CUT - 3 / 3 STARS" at three stars), Marlowe's `QuoteText`.
- Rows: cue grades (`CuesValue`), CUTs (`CutsValue`), time in shot (`FrameValue`), points (`PointsValue`).
- Buttons: **Shoot Again** (reload the shoot) · **Back to Cleanup** (`CleanupLevelName`; hidden if empty). No leaderboard submission.
- Not paused; same `KeepGameInputWithCursor` behaviour as Phase 1.

---

## 13. Planned but not built (cut for the jam)

Checked against `Source/` and the widget folder; none of these exist:

- Main menu (`WBP_MainMenu`, `L_MainMenu`, 3D dolly background, Continue / New Game). The briefing is the start screen.
- Notebook (`WBP_Notebook`: skills, set map, stats), Settings (`WBP_Settings`: volumes, sensitivity, FOV, quality, subtitles), Credits (`WBP_Credits`).
- `WBP_ConfirmDialog` (Restart and Quit act at once), `WBP_Fade`, `WBP_Toast`, `WBP_Subtitle`, `WBP_Prompt`, `WBP_LTTButton` / `ULTTButtonWidget`, `WBP_StarRow`, `WBP_Counter`, separate `WBP_CutInterstitial` (CUT lives in the shoot HUD), separate station widgets.
- `ULTTTweenSubsystem` with Real/Game clocks, the global button hover/press tween table, star stamp sequence.
- Phase 1 HUD items tied to cut systems: carry stack counter, Set Ready % clapperboard, zone strip, Crew Credits, golden reels, ability bar, combo, tutorial cards (tutorial lines are barks instead).
- Phase 2 HUD items not built: letterbox bars, the screen-edge arrow (replaced by the station hint line + arrow), Cue Sense as an unlockable skill (the cue card always shows the next cue), Patience button.
