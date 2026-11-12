# Audio List

Covers checklist **P9**. **Pri:** [M]/[S]/[C]. **Var:** number of variations to source (randomize pitch ±5% and volume ±2 dB on top).

---

## 1. Where to Get Audio

| Source | What it's good for | License notes |
|---|---|---|
| **Home Foley** (phone recorder) | Cups, plates, cardboard, broom, paper, cushions, hangers, keys, a squeegee on glass, blowing on a mic for wind | You own it. Fastest way to get *exactly* the right prop sounds |
| **Freesound.org** | Specific SFX (espresso machine, thunder sheet, film projector, megaphone feedback) | License varies per sound (CC0, CC-BY, CC-BY-NC). **Avoid NC** for anything you might sell; credit CC-BY |
| **Sonniss GameAudioGDC bundles** | High-quality ambiences, impacts, whooshes, rain, thunder | Royalty-free for commercial use per their license; check the bundle's terms |
| **Kenney.nl audio packs** | UI clicks, interface sounds, impacts | CC0 |
| **Pixabay (music & SFX)** | Background music, stingers | Pixabay Content License; check per item |
| **Incompetech (Kevin MacLeod)** | Jazz/cafe music | CC-BY: attribution required |
| **AI generation** (music/SFX/TTS) | Marlowe VO, music stems | Allowed only if the jam rules permit; check the tool's terms; disclose in credits |

Log **every** file in `CREDITS.md` as you add it: file name, source URL, author, license.

---

## 2. Sound Effects

### 2.1 Player

| ID | Description | Var | Pri |
|---|---|---|---|
| `SFX_Foot_Wood` | Footstep on wooden set floor | 6 | M |
| `SFX_Foot_Tile` | Footstep on cafe tile | 6 | M |
| `SFX_Foot_Carpet` | Soft step on rug/runner | 4 | S |
| `SFX_Foot_Metal` | Step on crew stair / scaffold | 5 | M |
| `SFX_Foot_Concrete` | Soundstage floor | 5 | M |
| `SFX_Jump` | Light cloth whoosh + breath | 3 | S |
| `SFX_Land` | Thud, scaled by fall speed | 3 | M |
| `SFX_Foot_Wet` | Splashy steps for the wet terrace (Phase 2) | 4 | S |

### 2.2 Props & Interaction

| ID | Description | Var | Pri |
|---|---|---|---|
| `SFX_Focus_Tick` | Very soft tick when focusing an interactable | 1 | M |
| `SFX_Pickup_Wood` / `_Ceramic` / `_Fabric` / `_Paper` / `_Metal` / `_Plastic` / `_Glass` | Lift sound per material | 3 each | M |
| `SFX_Pickup_FilmReel` | Metallic reel rattle | 2 | M |
| `SFX_Place_<Material>` | Solid "thunk" per material for snap | 3 each | M |
| `MS_PlaceChime` | Bright mallet/celesta note (C major ladder, MetaSound pitch input) | 1 base | M |
| `SFX_Place_Wrong` | Soft muted "bonk" | 2 | M |
| `SFX_Drop_<Material>` | Prop dropped/landed loose | 3 each | M |
| `SFX_Impact_<Material>` | Hard collision (throws), velocity-scaled | 4 each | M |
| `SFX_Throw_Whoosh` | Throw whoosh, pitch by charge | 3 | M |
| `SFX_Stack_Shuffle` | Stack cycling (scroll) | 2 | S |
| `SFX_Stack_Full` | Denied "nope" when full | 1 | M |
| `SFX_RightIt` | Heavy table tipping upright + wood thud | 2 | M |
| `SFX_PushBack` | Furniture scrape | 2 | M |
| `SFX_Poof` | Lost prop respawn | 1 | S |
| `SFX_TrickShot` | Sparkly "ta-da" | 1 | S |

### 2.3 Cleaning Tasks

| ID | Description | Var | Pri |
|---|---|---|---|
| `SFX_Trash_Deposit` | Item dropping into a bin | 4 | M |
| `SFX_Bin_Lid` | Lid flap | 2 | M |
| `SFX_Bin_Swish` | Swish + quick "mild cheer" crowd (a tiny polite clap) | 2 | M |
| `SFX_Bin_Clank` | Missed shot hitting the bin rim | 2 | M |
| `SFX_Bin_Spit` | Bin rejecting a set piece ("ptoo") | 1 | S |
| `SFX_Broom_Pickup` | Wooden handle clack | 1 | M |
| `SFX_Broom_Stroke` | Bristle sweep | 4 | M |
| `SFX_Popcorn_Crunch` | Popcorn/confetti scatter layer | 3 | M |
| `SFX_Pile_Cleared` | Satisfying puff + mini chime | 1 | M |
| `SFX_Cable_Zip` | Cable straightening "zzzip" | 2 | S |
| `SFX_Plug_In` | Click + small spark | 2 | S |
| `SFX_Light_Rotate` | Creaky light-head rotation loop (pitched by speed) | 1 loop | M |
| `SFX_Light_Lock` | "Kachunk" lock | 1 | M |
| `SFX_Light_Hum` | Ballast hum loop | 1 loop | S |
| `SFX_Reel_Slot` | Reel sliding into shelf | 2 | M |
| `SFX_Projector_Rattle` | Short projector clatter on shelf complete | 1 | S |
| `SFX_Hanger_Slide` | Hanger on rail "shhk" | 3 | S |
| `SFX_Lens_Squeak` | Microfiber squeak | 4 | M |
| `SFX_Lens_Ding` | Glint ding | 1 | M |
| `SFX_Fridge_Open` | Fridge door + hum (clapperboard reveal) | 1 | S |
| `SFX_Clapper_Clap` | Clapperboard clap (also used in UI) | 3 | M |

### 2.4 Progression & Rewards

| ID | Description | Var | Pri |
|---|---|---|---|
| `SFX_Group_Ding` | Two-note ascending chime + tally light click | 1 | M |
| `SFX_Zone_Kathunk` | Big stage-light switch "KA-THUNK" + electrical swell + a warm chord | 1 | M |
| `SFX_Credit_Gain` | Film-reel "click-tick" coin sound | 2 | M |
| `SFX_Skill_Buy` | Stamp + "ka-ching" | 1 | M |
| `SFX_Ability_Unlock` | Short fanfare | 1 | M |
| `SFX_Ability_Eye` | Shimmering scan whoosh | 1 | M |
| `SFX_Ability_GrabBag` | Suction whoosh + plinks per item | 1 + 3 | M |
| `SFX_Ability_SetDresser` | Magical swoosh + tiny thunks | 1 | M |
| `SFX_Ability_Ready` | Ping | 1 | M |
| `SFX_Ability_Denied` | Soft nope | 1 | M |
| `SFX_GoldenReel` | Shimmer loop near the reel + pickup jingle | 1 + 1 | M |
| `SFX_Combo` | Combo pop (pitched up per tier) | 1 | S |
| `SFX_Phase1_Complete` | Lights-on wave sequence (staggered kathunks) + swell | 1 | M |

### 2.5 Director, Walkie & Set Ambience

| ID | Description | Var | Pri |
|---|---|---|---|
| `SFX_Megaphone_On` | Megaphone click on | 2 | M |
| `SFX_Megaphone_Squeal` | Feedback squeal (rare, 5% of lines) | 2 | S |
| `SFX_Walkie_Squelch_In/Out` | Radio squelch | 2 each | M |
| `AMB_Soundstage` | Big room tone, distant HVAC | 1 loop | M |
| `AMB_CrewWalla` | Distant crew chatter, muffled | 1 loop | S |
| `AMB_Generator` | Faint generator hum outside | 1 loop | C |
| `AMB_CafeFake` | Phase 2: light cafe murmur (it's "for the scene") | 1 loop | S |
| `AMB_CityFake` | Phase 2: distant city bed behind street flat | 1 loop | S |

### 2.6 Voices (VO or blips)

| ID | Description | Pri |
|---|---|---|
| `VO_Marlowe_*` | All Marlowe lines (TTS or recorded), routed through `SMX_Megaphone` | S |
| `VO_Dash_*`, `VO_Juniper_*` | Actor lines (Phase 2) and walkie lines (Phase 1) | S |
| `SFX_Blip_Marlowe` | 6 short syllable sounds (low), for voice blips | M |
| `SFX_Blip_Dash` | 6 syllables (mid) | M |
| `SFX_Blip_Juniper` | 6 syllables (high) | M |
| `SFX_Actor_Cough` | Over-steam reaction | S |
| `SFX_Actor_Ow` | Bonk reaction (both actors) | M |
| `SFX_Actor_Laugh` | Juniper's laugh (rain moment) | S |

**Voice blips:** on each word of a subtitle, play a random syllable from the speaker's set at the speaker's base pitch ±8%, spaced 0.09–0.13 s. Skip punctuation; louder on words in CAPS. This gives every character a voice without recording anything. Use full VO later if time allows.

### 2.7 Phase 2 Stations & Effects

| ID | Description | Var | Pri |
|---|---|---|---|
| `SFX_Action_Beep` | "Rolling" camera beep + motor start | 1 | M |
| `SFX_Camera_Motor` | Film camera whirr loop while rolling | 1 loop | M |
| `SFX_Cue_Appear` | Cue card pop stinger (short brass "bwap") | 2 | M |
| `SFX_Grade_Perfect` / `Good` / `Late` / `Miss` | Result jingles (Miss = a playful deflating sound) | 1 each | M |
| `SFX_FrameAlarm` | Soft beeping loop while in frame | 1 loop | M |
| `SFX_Car_Roll` | Wheel squeak + rumble loop (speed-pitched) | 1 loop | M |
| `SFX_Cardboard_Flap` | Cardboard wobble | 2 | S |
| `SFX_Car_Horn` | Toy-like honk "meep meep" | 1 | M |
| `SFX_Car_TipOver` | Cardboard collapse (miss) | 1 | S |
| `SFX_DoorBell` | Cafe door bell | 2 | S |
| `SFX_Steam_Hiss` | Pressure hiss loop (pitched by pressure) | 1 loop | M |
| `SFX_Steam_Burst` | Release burst; big version for over-steam | 2 | M |
| `SFX_Napkin_Flutter` | Paper flutter during toss | 1 | M |
| `SFX_Catch` | Soft pat into hands | 2 | M |
| `SFX_Bonk` | Comedic bonk | 2 | M |
| `SFX_Thunder_Sheet` | Metal sheet wobble (layered per shake) | 4 | M |
| `SFX_Thunder_Crack` | Real thunder crack + rolling rumble | 3 | M |
| `SFX_Flash_Pop` | Flash bulb pop | 2 | M |
| `SFX_Thunder_Weak` | Sad "bwomp" (miss) | 1 | M |
| `SFX_Gel_Slide` | Gel frame slide + clack | 2 | S |
| `SFX_Fan_Crank` | Ratchet clicks (rate by RPM) | 1 loop | S |
| `SFX_Wind_Loop` | Wind loop (volume by strength) | 1 loop | S |
| `SFX_Boom_Creak` | Boom pole creak | 2 | M |
| `SFX_Mic_Bump` | Mic thump on head (miss) | 1 | M |
| `SFX_Valve_Squeak` | Rusty valve wheel | 3 | M |
| `SFX_Water_Spray` | Hose spray loop | 1 loop | M |
| `SFX_Rain_Loop_Light` / `_Heavy` | Rain bed on terrace | 1 each | M |
| `SFX_Rain_OnUmbrella` | Rain drumming on umbrella | 1 loop | S |
| `SFX_Umbrella_Open` | Whoosh pop | 1 | M |
| `SFX_Crowd_Aww` | Crew "awww" at the roses | 1 | S |
| `SFX_Crew_Cheer` | Crew cheer + applause at wrap | 1 | M |
| `SFX_Patience_Freeze` | Record-scratch freeze + tape-stop | 1 | M |
| `SFX_Cut_Slam` | CUT clapper slam + whoosh | 1 | M |
| `SFX_Confetti` | Confetti cannon pop at the wrap | 1 | S |

### 2.8 UI

| ID | Description | Var | Pri |
|---|---|---|---|
| `UI_Hover` | Soft paper tick | 3 | M |
| `UI_Click` | Pencil tap / typewriter key | 3 | M |
| `UI_Back` | Lower-pitched tick | 1 | M |
| `UI_Denied` | Muted buzz | 1 | M |
| `UI_Slider_Tick` | Tiny tick per step | 1 | M |
| `UI_Panel_Open` / `Close` | Paper whoosh in/out | 1 each | M |
| `UI_Stamp` | Rubber stamp thunk | 2 | M |
| `UI_Star_Stamp` | Stamp + sparkle (pitched up per star) | 1 | M |
| `UI_Counter_Tick` | Fast ticks during number rolls | 1 | M |
| `UI_PageFlip` | Notebook page flip | 3 | M |
| `UI_Toast` | Bright pop | 1 | M |
| `UI_Typewriter` | Typewriter keys for typing text | 4 | S |
| `UI_Pause_In` / `Out` | Tape-stop / tape-start | 1 each | M |
| `UI_Confirm` | Positive click | 1 | M |

---

## 3. Music

### 3.1 Track list

| ID | Use | Mood / reference | Length | Pri |
|---|---|---|---|---|
| `MUS_MainMenu` | Main menu | Light jazzy waltz, brushed drums, upright bass, piano | 1:30 loop | M |
| `MUS_P1_Stem1_Base` | Phase 1 layer 1 (starts at first placement) | Walking bass + brushes, 100–110 BPM | 2:00 loop | M |
| `MUS_P1_Stem2_Keys` | Layer 2 (after zone 2) | Piano comping | same loop | M |
| `MUS_P1_Stem3_Strings` | Layer 3 (after zone 4) | Pizzicato strings / vibraphone | same loop | M |
| `MUS_P1_Stem4_Melody` | Layer 4 (after zone 6) | Clarinet or muted trumpet melody | same loop | M |
| `MUS_P1_Complete` | Phase 1 completion | Big jazzy "ta-daa" with a cymbal swell | 0:08 | M |
| `MUS_P2_PreRoll` | Pre-shoot / pre-roll | Nervous light percussion, ticking | 1:00 loop | S |
| `MUS_P2_ActA` | Act I | Awkward-cute bossa nova | 1:30 loop | M |
| `MUS_P2_ActB` | Act II | Tense, dramatic strings + soft piano | 1:30 loop | M |
| `MUS_P2_ActC` | Act III | Romantic swell with rain-friendly piano | 1:30 loop | M |
| `MUS_P2_Finale` | Final lean-in | Held string chord → silence at the flash | 0:15 | M |
| `MUS_Wrap` | "That's a wrap!" | Triumphant brass hit + jazzy outro | 0:12 | M |
| `MUS_Credits` | Credits | Reprise of the menu theme, warmer | 1:00+ | S |
| `MUS_Sting_Cut` | CUT interstitial | Record scratch + comedic brass "wah-wah" (original) | 0:02 | M |
| `MUS_Sting_Fired` | "You're Fired!" ending | Sad trombone-style descending phrase (original) | 0:03 | S |
| `MUS_Sting_ActTitle` | Act title cards | Short jazzy hit | 0:02 | S |

### 3.2 Phase 1 layering (implementation)
- All four stems are the same length/tempo and start together (stem 1 audible, others at 0 volume) via one MetaSound with 4 inputs, or 4 synced Audio Components started on the same frame.
- Zone completions raise layers: zones 1–2 → stem 2 fades in (2 s), zones 3–4 → stem 3, zones 5–6 → stem 4, zone 7 → all + slight high-shelf "brightness" boost.
- Duck all music −6 dB during barks (Sound Mix or a sidechain-style volume tween).

### 3.3 Phase 2 transitions
- Crossfade per act (2 s) at the act title card.
- Music volume drops to 50% during cue windows (so effects sounds read) and comes back after the grade.
- Final thunder → music cuts to silence on the flash (0.0 s), then `MUS_Wrap` on "That's a wrap!".

---

## 4. Implementation Checklist

- [ ] Sound Classes: Master → Music, SFX, Voice, UI, Ambience; volumes driven by Sound Mix overrides from settings.
- [ ] Submixes: `SMX_Megaphone` (band-pass, slight saturation), `SMX_Walkie` (narrower band-pass + noise), `SMX_Pause` (low-pass while paused).
- [ ] Attenuation: `ATT_Prop` (200 → 2500 cm), `ATT_Station` (300 → 3000 cm), `ATT_Ambience` (non-spatial or wide).
- [ ] Concurrency: `CON_Bark` (1), `CON_PropImpact` (6, stop quietest), `CON_Footstep` (2), `CON_Chime` (3).
- [ ] Random containers / MetaSounds for variations with pitch ±5%, volume ±2 dB.
- [ ] `MS_PlaceChime` with an integer input "Step" mapped to semitones.
- [ ] Loops (rain, hiss, car roll, fan, light rotate) use Audio Components owned by their actors, with parameters for pitch/volume driven every tick.
- [ ] UI sounds marked as UI sounds so they play while paused.
- [ ] Normalize loudness: dialogue −16 LUFS-ish target; SFX peaks below −3 dBFS; music sits ~6 dB below dialogue.
