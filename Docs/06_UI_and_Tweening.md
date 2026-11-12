# UI & Tweening

Covers checklist **P8, P11**. Plain UMG. Every animated widget uses `ULTTTweenSubsystem` with `ELTTTweenClock::Real` (so it animates while paused) or UMG Widget Animations where noted. **Set every animated widget's Render Transform Pivot to (0.5, 0.5)** unless stated, so scales bounce from the center.

---

## 1. Style Guide

**Theme:** a film production's paperwork and gear: clapperboards, call sheets, tape marks, film strips, REC lights.

| Token | Hex | Use |
|---|---|---|
| `Ink` | `#1B1B1F` | Text, clapper black |
| `Paper` | `#F4EEDF` | Call-sheet panels |
| `TapeYellow` | `#F2C230` | Highlights, tape marks, progress fill |
| `RecRed` | `#E5383B` | REC, warnings, frame violations |
| `Gold` | `#E8B04B` | Placement, stars, golden reels |
| `Cyan` | `#4CC9F0` | Director's Eye, info |
| `CueGreen` | `#57CC99` | Active cue, Perfect |
| `Slate` | `#2E3440` | Dark panels |
| `White` | `#FFFFFF` | Clapper stripes |

**Fonts (verify licenses; OFL suggested):** Headings in a condensed bold display face (e.g. Bebas Neue), body and subtitles in a typewriter face (e.g. Courier Prime). Subtitle sizes: 22 / 28 / 34 pt (Small/Medium/Large) at 1080p, with a 60% black backing box.

**Motifs:** clapperboard stripe borders on headers, paper-clip on panels, hand-drawn tape strips holding cards, film-strip bars for meters, rubber stamps for results.

---

## 2. Global Tween Specs (apply everywhere)

| Element | Event | Tween |
|---|---|---|
| Button | Hover | scale 1 → 1.06, 0.12 s OutBack; tape-strip underline width 0 → 100%, 0.15 s OutCubic; SFX hover tick |
| Button | Unhover | scale → 1, 0.1 s OutCubic; underline → 0 |
| Button | Press | scale → 0.94, 0.05 s OutCubic |
| Button | Release / click | scale 0.94 → 1.08 → 1, 0.25 s OutElastic; SFX click |
| Button | Disabled click | Wobble: render angle Punch ±6°, 0.3 s; SFX denied |
| Panel / card | Open | opacity 0 → 1 (0.15 s) + scale 0.85 → 1, 0.3 s OutBack; slight translation Y +30 → 0 |
| Panel / card | Close | scale 1 → 0.9 + opacity → 0, 0.15 s InBack; remove on complete |
| List items | Open stagger | each item: translation X −40 → 0 + opacity, 0.2 s OutCubic, stagger 0.04 s |
| Toast | Show | slide from top Y −80 → 0, 0.35 s OutBack; hold 2.5 s; slide out Y → −80, 0.25 s InBack |
| Counter (numbers) | Value change | roll up via TweenFloat over 0.4 s OutCubic, text updates per frame; on finish scale 1 → 1.2 → 1, 0.2 s OutBack |
| Icon | Gain / ready | scale 0 → 1.25 → 1, 0.35 s OutElastic |
| Star | Stamp | scale 2.2 → 1, 0.18 s InBack… then 1 → 1.12 → 1, 0.2 s OutBack; rotation −12° → random(−6°, 6°); opacity 0 → 1; SFX stamp thunk pitched +2 semitones per star; tiny screen shake |
| Prompt | Focus gained | opacity 0 → 1 + scale 0.8 → 1, 0.15 s OutBack |
| Prompt | Text change | quick squash X 1 → 0.9 → 1, 0.12 s |
| Crosshair | Focus | dot scale 1 → 1.8, 0.12 s OutBack; color → white |
| Crosshair | Can place | color → Gold; ring pulse loop (scale 1 → 1.3, opacity 1 → 0, 0.8 s) |
| Crosshair | Hold progress | radial fill material param 0 → 1 |
| Meter | Change | fill width tween 0.25 s OutCubic, and a "ghost" trailing fill that catches up after 0.3 s delay (shows the loss/gain) |

Implementation helper: `UWBP_LTTButton` (C++ `ULTTButtonWidget` subclass of UUserWidget wrapping a UButton) implements all button tweens and sounds once; every menu uses it.

---

## 3. Main Menu (`WBP_MainMenu`, map `L_MainMenu`)

### Layout
- **Background:** live 3D view of the clean cafe set. A CineCameraActor slowly dollies along a spline (60 s loop, InOutSine) past table 3 and up toward the terrace. Soft music. Marlowe's empty director's chair in view.
- **Left:** a clipboard holding a call sheet (Paper panel, tilted −3°), with the buttons as "call sheet lines".
- **Top-right:** title logo as a clapperboard: "Clean! Cut! Action!" on the slate, with "Scene 42 · Take 2 · Dir. M. Finch".
- **Bottom:** random Marlowe quip (`05 §3`) in small italic.

### Buttons (top to bottom)

| Button | Visible when | Action |
|---|---|---|
| **Continue** | `HasAnyProgress()` | Phase 1 complete → `GoToPhase2()`; else → `GoToPhase1()` |
| **New Game** | always | If progress exists → confirm "Start over? Your progress will be lost (settings are kept)." → `NewGame()` → `GoToPhase1()` |
| **Clean the Set** | Phase 1 complete | Replay Phase 1 |
| **Shoot the Scene** | always (locked until Phase 1 complete) | `GoToPhase2()`; locked: padlock + red tape "CLOSED SET", tooltip "Clean the set first, Rookie.", click = wobble |
| **Settings** | always | Open `WBP_Settings` |
| **Credits** | always | Open `WBP_Credits` (menu version, skippable) |
| **Quit** | always | Confirm → `QuitGame` |

Best results under Continue: "Set: ★★☆ 17:42 · Scene: ★★★" (small).

### Intro animation on open (≈ 1.6 s, skippable by any key)
1. Black → fade in the 3D background (0.6 s).
2. Clapperboard logo drops in from above: translation Y −400 → 0, 0.45 s OutBounce.
3. Clapper arm opens (rotation 0 → −25°, 0.2 s OutCubic) then **snaps shut** (−25 → 0, 0.08 s InBack) with CLAP SFX and a white flash overlay (opacity 0.6 → 0, 0.15 s); logo squash.
4. Clipboard slides in from the left (X −600 → 0, 0.4 s OutBack), buttons stagger in.
5. Quip types on (typewriter, 30 chars/s with tick sounds).

Input mode UI-only, mouse visible, first button focused (for keyboard/gamepad).

---

## 4. Pause (`WBP_PauseMenu`)

- **Open:** game paused, background blur widget (strength 0 → 6, 0.2 s), dark tint 0 → 45%, clipboard slides up from the bottom (Y +500 → 0, 0.35 s OutBack), "PAUSED - TAKING FIVE" header stamp (Star stamp tween, no shake). Audio: tape-stop "wow" SFX + pause low-pass on.
- **Buttons:** Resume · Settings · Main Menu · Quit Game.
  - Main Menu → `WBP_ConfirmDialog`: "Return to the main menu? Cleanup progress in this session will be lost." (Phase 1) / "Leave the shoot? This take will be lost." (Phase 2). Yes → `GoToMainMenu()`.
  - Quit → confirm "Quit the game?" → `QuitGame`.
- **Close:** reverse tweens (0.2 s), tape-start SFX, low-pass off over 0.3 s.
- Esc toggles. Esc inside Settings or a confirm dialog closes that first.
- Random Marlowe pause quip under the header.

### Confirm dialog (`WBP_ConfirmDialog`)
A small paper card taped on a dark tint. Open: scale 0.7 → 1 OutBack 0.25 s. Buttons Yes (RecRed) / No (Slate); default focus No. Callback delegate.

---

## 5. Phase 1 HUD (`WBP_HUD_Phase1`)

| Element | Position | Behavior |
|---|---|---|
| Crosshair | Center | See §2; turns into a radial fill for holds |
| Interaction prompt | Below center | "[LMB] Pick up · Chair", "[LMB] Place", "[E] Hold - Right the table", "[RMB] Throw" |
| Carry stack | Bottom-right | Icons of carried items (top item largest), "3 / 5" capacity; bounce on change; shakes red when full and you try to pick up |
| Set Ready % | Top-center | Mini clapperboard whose stripes fill left→right with TapeYellow; % text with counter roll; clapperboard icon appears when found |
| Zone strip | Under Set Ready | 7 small zone pips (grey → gold when done), pip pops when complete |
| Crew Credits | Top-right | Film-reel coin icon + number; credit icons fly in from world positions |
| Golden reels | Top-right under credits | 3 slots, filled gold when found |
| Ability bar | Bottom-center | 3 slots (1/2/3), icon + radial cooldown overlay + key label; locked slots show a padlock; ready → Icon gain tween + ping |
| Combo | Right of crosshair | "COMBO ×5" pop (scale 0 → 1.3 → 1) then fades after 1.5 s |
| Subtitles | Bottom-center above ability bar | Speaker name colored (Marlowe = TapeYellow, Dash = brown `#C08552`, Juniper = blue `#7FA7E0`, walkie = grey with 📻 icon); slide up 20 px + fade in 0.2 s; megaphone icon pulses while Marlowe speaks |
| Tutorial card | Top-left | Slide in from left, 4 s, slide out |
| Toasts | Top-center | Zone complete ("SEATING - SET!"), ability unlocked, golden reel |
| Hint list | Right edge | Only at ≥ 95%: "Remaining: Kitchen (1), Crew (1)" |

**Zone-complete toast** is special: a full-width clapperboard bar slams down from the top (0.25 s OutBounce) with the zone name, holds 1.8 s, flies up.

---

## 6. Notebook (`WBP_Notebook`, Tab)

- Opens as a spiral notebook filling 70% of the screen (open: scale 0.6 → 1 + rotation −8° → 0, 0.35 s OutBack, page-flip SFX). Game is **not** paused (it's a cozy game, but time passes) - show a small "Time's ticking!" note; *alternative if testers dislike it: pause.*
- **Tabs (sticky notes on the edge):** Skills · Set Map · Stats.
- **Skills page:** three columns (Hustle / Prop Master / Special FX) drawn as hand-written lists with doodle icons. Each node: name, rank pips, cost, description. States: Locked (pencil grey), Available (ink, pulsing border), Owned (stamped "GOT IT").
  - Buy: click → node stamp tween + "ka-ching" film-reel SFX + credits counter roll down; Save.
  - Can't afford: wobble + "Need 2 more credits".
- **Set Map page:** both floors as drawn blueprints; zones shaded by completion; pin icons on golden reels found.
- **Stats page:** time, props placed, trash, swishes, piles, longest combo.
- Close with Tab/Esc: reverse tween, page-close SFX.

---

## 7. Phase 1 End Screen (`WBP_EndScreen_Phase1`)

Sequence (≈ 6 s, any key skips to the final state):
1. Screen darkens to 60%; a big slate slams down: "SET READY!" (OutBounce 0.4 s, CLAP SFX, shake).
2. Call-sheet results card slides in from the right (OutBack 0.35 s).
3. Stat rows type in one by one with counter rolls (0.3 s stagger):
   - Time on set ........ 17:42
   - Props returned ........ 80
   - Trash binned ........ 54 (swishes 12)
   - Floor swept ........ 12 piles
   - Golden reels ........ 2 / 3
   - Crew credits earned ........ 46
4. Stars stamp in one at a time (§2 Star stamp, 0.35 s apart). Missing stars show as faint outlines.
5. "NEW BEST!" ribbon if applicable (rotation wiggle loop).
6. Marlowe's quote for the star count (`05 §1.21`) types on.
7. Buttons slide in: **Roll Camera!** (primary, TapeYellow, pulses gently) → `GoToPhase2()`; **Main Menu**.

---

## 8. Phase 2 HUD (`WBP_HUD_Phase2`)

| Element | Position | Behavior |
|---|---|---|
| Letterbox bars | Top & bottom | Slide in 0 → 8% height, 0.6 s InOutSine on ACTION; out on CUT/wrap |
| REC + timecode | Top-left (inside the bar) | Red dot blinking 1 Hz, "REC 00:01:23:14", "SC 42 · TK 1" |
| Take meter | Top-center | Film-strip bar 0–100 (§2 meter with ghost trail); <25 flashes red; gain shows sprocket sparkle |
| Cue card | Center-top, large | On call: card spins in (rotation 25° → 0, scale 0 → 1.15 → 1, 0.35 s OutBack), icon + "RAIN!" + floor badge "2F ↑"; countdown ring around the icon (green → yellow → red by Perfect/Good/Late thresholds); while the player is in station mode, it shrinks to the corner (0.25 s) |
| Off-screen arrow | Screen edge | Points to the active station when not on screen; bob animation; distance "12 m" |
| Grade popup | Center | "PERFECT!" (CueGreen) / "GOOD" (TapeYellow) / "LATE" (orange) / "MISS" (RecRed): scale 0 → 1.4 → 1 OutElastic 0.4 s, float up 40 px and fade over 0.8 s; points "+100" small below |
| Next cue (Cue Sense) | Top-right | "NEXT: THUNDER · 2F", slides in |
| Frame warning | Full-screen edge | Red vignette pulse 2 Hz + "⚠ IN SHOT" replacing REC |
| Patience button | Bottom-center | Icon with "1" key; used = greyed with a "USED" stamp |
| Station widget | Bottom-center | Swapped per station (see below) |
| Act title card | Center | "ACT II - THE TERRACE": typewriter on, hold 1.5 s, fade |
| Subtitles | Bottom-center | As Phase 1; off-script lines in italics + "(off-script)" tag |

### Station widgets

| Station | Widget |
|---|---|
| Car Rail | Horizontal speed meter with green band, needle wobble, progress "A ▶▶▶ B" |
| Door Bell | Timing ring closing on a target ring |
| Espresso | Semicircle pressure gauge with green/yellow arcs, needle shake by pressure |
| Toss | Arc preview in-world + charge bar near crosshair |
| Thunder | Rumble meter (vertical) → "FLASH! [F]" big prompt bounce |
| Light Rig | Three gel swatches (1/2/3), selected pops up 10 px |
| Fan | RPM dial with sweet spot, "3.0 s" sustain counter filling |
| Boom | Vertical bar showing frame line (red) and audio band (green), mic marker; sustain counter |
| Rain | Valve ring fill → coverage % meter + sustain counter |

Each widget opens with the Panel Open tween and closes with Panel Close.

---

## 9. CUT, Phase 2 End & Credits

### 9.1 CUT interstitial (`WBP_CutInterstitial`)
- Freeze-frame effect (post: desaturate 0 → 1, 0.2 s).
- Clapperboard slams in from the top (OutBounce) with "CUT!" and "TAKE 3" chalk text, CLAP SFX, shake.
- Marlowe CUT line subtitle.
- After 2.5 s: wipe to black (horizontal bar wipe 0.3 s), reset happens behind it, wipe out.

### 9.2 Phase 2 End Screen (`WBP_EndScreen_Phase2`)
1. Final thunder flash fades to black. Big slate: "THAT'S A WRAP!" (OutBounce + clap + confetti Niagara in UI or world).
2. "Dailies" card (paper) slides in. Cue list: each row = cue icon + name + grade stamp, revealed 0.15 s apart with a little stamp sound per row.
3. Totals: Score (counter roll), Takes used, Time in frame, Perfects ×N.
4. Stars stamp in. "NEW BEST!" if applicable.
5. The final clapperboard flips over (scaleX 1 → 0 → 1 trick with a texture swap at 0) to reveal **"DIRECTOR'S CUT"**.
6. Marlowe quote (`05 §2.6`).
7. Buttons: **Roll Credits** (primary) · **Retake Scene** (`GoToPhase2()`) · **Main Menu**.

### 9.3 Credits (`WBP_Credits`)
- Black with film grain; credits scroll upward (constant speed tween, 45 s), names in typewriter font. Sections: Made by, Tools, Audio, Fonts, Third-party assets (from `CREDITS.md`), Special thanks, then the jokey roles (`05 §3`).
- Hold Space / click to speed up ×4; Esc skips.
- End card: "Clean! Cut! Action! - a Director's Cut" + "Thanks for playing, Assistant Director." → Main Menu after 4 s.

---

## 10. Settings (`WBP_Settings`)

| Section | Controls |
|---|---|
| Audio | Master, Music, SFX, Voice, UI volume sliders (0–100, step 5) with a preview sound on release |
| Controls | Mouse sensitivity (0.2–3.0), Invert Y |
| Display | Field of view (75–110), Quality preset (Low / Medium / High / Epic), Head bob, Screen shake |
| Accessibility | Subtitles on/off, Subtitle size, Show camera frame (Phase 2) |

- Changes apply live; **Back** saves (`ApplySettings()` + `Save()`); **Reset to defaults** with confirm.
- Slider handle bounces slightly on release (scale 1.2 → 1, 0.2 s OutBack) with a tick per step.

---

## 11. Shared Widgets

| Widget | Notes |
|---|---|
| `WBP_Fade` | Full-screen black image, opacity tween, Z 1000, used by level flow |
| `WBP_Subtitle` | Queue of 1 visible + 1 fading line; supports italics; respects size setting |
| `WBP_Toast` | Generic toast with icon + title + subtitle |
| `WBP_Prompt` | Key glyph + verb + object name |
| `WBP_LTTButton` | All button tweens + sounds (§2) |
| `WBP_StarRow` | Three stars with stamp sequence API `PlayStamp(NumStars, Delay)` |
| `WBP_Counter` | Number with roll-up API |

---

## 12. Implementation Notes

- Create HUD widgets in the GameMode/PlayerController BeginPlay; keep references; never recreate per frame.
- All UI tweens use `Real` clock so pause menus animate. Gameplay world tweens use `Game` clock.
- Before starting a tween on a widget property, cancel the previous tween on that property (`CancelAllFor`) to avoid fighting (rapid hover/unhover).
- Mouse cursor: hidden in gameplay; visible in pause, menus, notebook, end screens. Restore focus to the first button when a panel opens.
- Test at 1280×720, 1920×1080, 2560×1440, 3840×2160 using DPI scaling rules (Shortest Side, default curve).
- No raw `Print String` in Shipping; route debug text through the debug HUD only.
