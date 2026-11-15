# Audio List

Covers checklist **P9**. **Pri:** [M]/[S]/[C]. **Status** is checked against `Content/_CleanCutAction/` and the two cooked maps on 2026-11-15:

- **in** — the asset exists (path under `/Game/_CleanCutAction/`) and something uses it
- **in, unused** — the asset exists but nothing references it yet
- **missing** — no asset yet
- **no mechanic** — the feature it was planned for isn't in the game, so the sound isn't needed unless the feature comes back

---

## 0. What's in the project

| Folder | Contents | Used by |
|---|---|---|
| `SFX/Props/` | `SW_Prop_<Material>_Pickup` / `_Place` / `_Break` for 8 materials (Wood, Metal, Paper, Ceramic, Glass, Plastic, Fabric, Food), `DA_PropSounds` (the bank), `ATT_Props` (attenuation) | Every prop via `ULTTPropSoundSet` (pitch 0.92–1.08, break ×1.2 volume). `DA_PropSounds` is in `MapsToCook` |
| `SFX/` | `SW_CoffeeBuzz`, `SW_CoffeeFill`, `SW_CoffeeGrind`, `SW_LightClick`, `SW_MicrowaveDing`, `SW_MicrowaveDoor`, `SW_MicrowaveTick`, `SW_MopSwish`, `SW_PowerClick`, `SW_PuddleClean`, `SW_PuddleSplash`, `SW_SinkWash`, `SW_SinkWater`, `SW_Spark`, `SW_applause`, `SW_cut`, `SW_miss`, `SW_next_phase`, `SW_notification`, `SW_success` | Phase 1 appliances, puddles, power box; Phase 1/2 directors (see below) |
| `SFX/stations/` | 23 station sounds (section 2.7) | Phase 2 map effect stations |
| `MusicFX/` | 6 tracks: `kf011815-sunny-day`, `kf011821-ghostly-bossa`, `kf014010-nova-hall`, `kf017407-only-you`, `kf027101-carefree`, `kf027103-more-of-the-other` (about 14 min) | `ALTTMusicPlayer` in both cafe maps, shuffled, crossfaded, mute toggle (`WBP_MusicToggle`, pause menu) |
| `EmoteMusic/` | `paroparo` (4.8 min) | Dance emote, loaded by path (in `MapsToCook`) |

No Sound Classes, Submixes, Concurrency assets, MetaSounds or voice files exist yet.

---

## 1. Where to Get Audio

| Source | What it's good for | License notes |
|---|---|---|
| **Home Foley** (phone recorder) | Cups, plates, cardboard, broom, paper, cushions, hangers, keys, a squeegee on glass, blowing on a mic for wind | You own it |
| **Freesound.org** | Specific SFX (espresso machine, thunder sheet, film projector, megaphone feedback) | License varies per sound. **Avoid NC**; credit CC-BY |
| **Sonniss GameAudioGDC bundles** | Ambiences, impacts, whooshes, rain, thunder | Royalty-free per their license |
| **Kenney.nl audio packs** | UI clicks, interface sounds, impacts | CC0 |
| **Pixabay (music & SFX)** | Background music, stingers | Pixabay Content License |
| **Incompetech (Kevin MacLeod)** | Jazz/cafe music | CC-BY: attribution required |
| **AI generation** (music/SFX/TTS) | Marlowe VO, music stems | Only if the jam rules permit; disclose in credits |

Log every file in a `CREDITS.md` (file, source URL, author, license). **It doesn't exist yet**, and the sources of the current SFX and music aren't recorded anywhere in the repo.

---

## 2. Sound Effects

### 2.1 Player

| ID | Description | Pri | Status |
|---|---|---|---|
| `SFX_Foot_Wood` / `_Tile` / `_Carpet` / `_Metal` / `_Concrete` | Footsteps per surface | M/S | missing |
| `SFX_Jump` | Light cloth whoosh + breath | S | missing |
| `SFX_Land` | Thud, scaled by fall speed | M | missing |
| `SFX_Foot_Wet` | Splashy steps (Phase 2 terrace) | S | missing |

### 2.2 Props & Interaction

| ID | Description | Pri | Status |
|---|---|---|---|
| `SFX_Focus_Tick` | Soft tick when focusing an interactable | M | missing |
| `SFX_Pickup_<Material>` | Lift sound per material | M | **in**: `SFX/Props/SW_Prop_<Material>_Pickup` (8 materials, Food added) |
| `SFX_Pickup_FilmReel` | Metallic reel rattle | M | no mechanic |
| `SFX_Place_<Material>` | Solid "thunk" per material for snap | M | **in**: `SFX/Props/SW_Prop_<Material>_Place` |
| (new) Break per material | Prop shatters on a bad drop or throw | — | **in**: `SFX/Props/SW_Prop_<Material>_Break` |
| `MS_PlaceChime` | Rising mallet note per placement (MetaSound) | M | missing |
| `SFX_Place_Wrong` | Soft muted "bonk" | M | missing (a bad drop breaks the prop and plays its Break sound) |
| `SFX_Drop_<Material>` | Prop dropped/landed loose | M | missing |
| `SFX_Impact_<Material>` | Hard collision (throws) | M | missing (throws break the prop → Break sound) |
| `SFX_Throw_Whoosh` | Throw whoosh | M | in, unused: `SFX/stations/SW_Whoosh` |
| `SFX_Stack_Shuffle` / `SFX_Stack_Full` | Stack cycling / full | S/M | missing |
| `SFX_RightIt` / `SFX_PushBack` | Table tipping upright / furniture scrape | M | missing |
| `SFX_Poof` | Lost prop respawn | S | missing |
| `SFX_TrickShot` | Sparkly "ta-da" | S | missing |

### 2.3 Cleaning Tasks

| ID | Description | Pri | Status |
|---|---|---|---|
| `SFX_Trash_Deposit`, `SFX_Bin_Lid`, `SFX_Bin_Swish`, `SFX_Bin_Clank`, `SFX_Bin_Spit` | Trash bin | M/S | no mechanic |
| `SFX_Broom_Pickup`, `SFX_Broom_Stroke`, `SFX_Popcorn_Crunch`, `SFX_Pile_Cleared` | Sweeping piles | M | no mechanic (puddle mopping replaced it, below) |
| `SFX_Cable_Zip` | Cable straightening | S | no mechanic |
| `SFX_Plug_In` | Click + small spark | S | **in** as the power box: `SFX/SW_PowerClick`, `SFX/SW_Spark` (`ALTTPowerBox`, `BP_ElectricBox`) |
| `SFX_Light_Rotate`, `SFX_Light_Lock`, `SFX_Light_Hum` | Aiming stage lights | M/S | no mechanic |
| `SFX_Reel_Slot`, `SFX_Projector_Rattle` | Film reel shelf | M/S | no mechanic |
| `SFX_Hanger_Slide` | Costume rack | S | no mechanic |
| `SFX_Lens_Squeak`, `SFX_Lens_Ding` | Lens wiping | M | no mechanic |
| `SFX_Fridge_Open` | Fridge door (clapperboard reveal) | S | missing |
| `SFX_Clapper_Clap` | Clapperboard clap | M | **in**: `SFX/stations/SW_ClapperboardClap` (Phase 2 slate) |
| (new) Puddle mopping | Mop swish, splash, clean | — | **in**: `SFX/SW_MopSwish`, `SW_PuddleSplash`, `SW_PuddleClean` (`ALTTWaterPuddle`) |
| (new) Light switch | Switch click | — | **in**: `SFX/SW_LightClick` (`ULTTLightSwitchComponent`) |
| (new) Coffee machine | Grind, fill, done buzz | — | **in**: `SFX/SW_CoffeeGrind`, `SW_CoffeeFill`, `SW_CoffeeBuzz` (`BP_Prop_EspressoMachine`) |
| (new) Microwave | Door, tick, ding | — | **in**: `SFX/SW_MicrowaveDoor`, `SW_MicrowaveTick`, `SW_MicrowaveDing` (`BP_Prop_Microwave`) |
| (new) Sink | Washing dishes, water | — | **in**: `SFX/SW_SinkWash`, `SW_SinkWater` (`BP_Sink`) |

### 2.4 Progression & Rewards

| ID | Description | Pri | Status |
|---|---|---|---|
| `SFX_Group_Ding`, `SFX_Zone_Kathunk` | Group / zone complete | M | missing |
| `SFX_Credit_Gain`, `SFX_Skill_Buy` | Credits and skill shop | M | no mechanic |
| `SFX_Ability_*` (Unlock, Eye, GrabBag, SetDresser, Ready, Denied) | Abilities | M | no mechanic |
| `SFX_GoldenReel` | Golden reel shimmer + pickup | M | no mechanic |
| `SFX_Combo` | Combo pop | S | no mechanic |
| `SFX_Phase1_Complete` | Set ready | M | **in**: `SFX/SW_next_phase` ("SET READY!" end screen, `ALTTMessDirector.SetReadySound`) |
| (new) Time's up | Countdown ran out | — | **in**: `SFX/SW_miss` (`ALTTMessDirector.TimeUpSound`) |

### 2.5 Director, Walkie & Set Ambience

| ID | Description | Pri | Status |
|---|---|---|---|
| `SFX_Megaphone_On`, `SFX_Megaphone_Squeal` | Megaphone click / feedback | M/S | missing (slot exists: `ULTTCleanupBarker.LineSound`) |
| `SFX_Walkie_Squelch_In/Out` | Radio squelch | M | missing (slot exists: `ULTTCleanupBarker.WalkieSound`) |
| `AMB_Soundstage`, `AMB_CrewWalla`, `AMB_Generator` | Room tone, crew, generator | M/S/C | missing |
| `AMB_CafeFake`, `AMB_CityFake` | Phase 2 cafe murmur / city bed | S | missing |

### 2.6 Voices (VO or blips)

| ID | Description | Pri | Status |
|---|---|---|---|
| `VO_Marlowe_*`, `VO_Dash_*`, `VO_Juniper_*` | Spoken lines | S | missing (all lines are on-screen text) |
| `SFX_Blip_Marlowe` / `_Dash` / `_Juniper` | Syllable blips per speaker | M | missing |
| `SFX_Actor_Cough`, `SFX_Actor_Ow`, `SFX_Actor_Laugh` | Actor reactions | S/M | missing |

**Voice blips (plan):** on each word of a subtitle, play a random syllable from the speaker's set at the speaker's base pitch ±8%, spaced 0.09–0.13 s.

### 2.7 Phase 2 Stations & Effects

Station sounds are set per `ALTTEffectStation` in `L_NYC_Cafe_Phase2_Michael` (`EffectSound` / `MissSound`). Director sounds are `ALTTSceneDirector` defaults.

| ID | Description | Pri | Status |
|---|---|---|---|
| `SFX_Action_Beep`, `SFX_Camera_Motor` | Rolling beep / camera whirr | M | missing |
| `SFX_Cue_Appear` | Cue call stinger | M | **in**: `SFX/SW_notification` (`CueCallSound`) |
| `SFX_Grade_Perfect` / `Good` / `Late` / `Miss` | Result jingles | M | **in**: `SFX/SW_success` for Perfect, Good and Late (`CueGoodSound`); `SFX/SW_miss` for Miss (`CueMissSound`) |
| `SFX_FrameAlarm` | Beeping while in frame | M | missing |
| `SFX_Car_Roll` | Car rail rumble loop | M | missing |
| `SFX_Cardboard_Flap`, `SFX_Car_TipOver` | Cardboard wobble / collapse | S | missing |
| `SFX_Car_Horn` | Toy honk | M | **in**: `SFX/stations/SW_CarBeepBeep` |
| `SFX_DoorBell` | Cafe door bell | S | **in**: `SFX/stations/SW_ShopDoorBell` |
| `SFX_Steam_Hiss`, `SFX_Steam_Burst` | Espresso steam | M | **in**: `SFX/stations/SW_Espresso`, `SW_CoffeeSputter` |
| `SFX_Napkin_Flutter`, `SFX_Catch` | Prop toss | M | missing |
| `SFX_Bonk` | Comedic bonk | M | **in**: `SFX/stations/SW_LoudBonk` |
| `SFX_Thunder_Sheet` | Metal sheet wobble | M | **in**: `SFX/stations/SW_SheetWobble` |
| `SFX_Thunder_Crack` | Thunder crack + rumble | M | **in**: `SFX/stations/SW_ThunderClap` |
| `SFX_Flash_Pop`, `SFX_Thunder_Weak` | Flash bulb / weak thunder | M | missing |
| `SFX_Gel_Slide` | Gel frame slide | S | VERIFY: `SFX/stations/SW_PaperSlide` is in the map; which station uses it isn't confirmed |
| `SFX_Fan_Crank`, `SFX_Wind_Loop` | Wind fan | S | **in**: `SFX/stations/SW_WindFan`, `SW_FanTurnOff` |
| `SFX_Boom_Creak` | Boom pole creak | M | **in**: `SFX/stations/SW_Creak` |
| `SFX_Mic_Bump` | Mic thump on head | M | missing (`SFX/stations/SW_MicFeedback` is in the map) |
| `SFX_Valve_Squeak`, `SFX_Water_Spray` | Rain valve / hose | M | missing |
| `SFX_Rain_Loop_Light` / `_Heavy` | Rain bed | M | **in** (light): `SFX/stations/SW_LightRain`; heavy missing |
| `SFX_Rain_OnUmbrella` | Rain on umbrella | S | missing |
| `SFX_Umbrella_Open` | Whoosh pop | M | **in**: `SFX/stations/SW_UmbrellaOpen` |
| `SFX_Crowd_Aww` | Crew "awww" | S | missing |
| `SFX_Crew_Cheer` | Cheer + applause at wrap | M | **in**: `SFX/SW_applause` (`WrapSound`) |
| `SFX_Patience_Freeze` | Record-scratch freeze | M | missing |
| `SFX_Cut_Slam` | CUT clapper slam | M | **in**: `SFX/SW_cut` (`CutSound`) |
| `SFX_Confetti` | Confetti pop at the wrap | S | missing |
| (new) Other station sounds in the map | — | — | **in**: `SFX/stations/SW_BicycleBell`, `SW_CowBell`, `SW_SwitchOn`, `SW_SwitchOff`, `SW_WaterDrop`, `SW_MicFeedback` |

### 2.8 UI

| ID | Description | Pri | Status |
|---|---|---|---|
| `UI_Hover` | Soft paper tick | M | in, unused: `SFX/stations/SW_UiHover` |
| `UI_Click` | Pencil tap / typewriter key | M | in, unused: `SFX/stations/SW_UiClick` |
| `UI_Back`, `UI_Denied`, `UI_Slider_Tick`, `UI_Panel_Open` / `Close`, `UI_Stamp`, `UI_Star_Stamp`, `UI_Counter_Tick`, `UI_PageFlip`, `UI_Toast`, `UI_Typewriter`, `UI_Pause_In` / `Out`, `UI_Confirm` | Menu and HUD sounds | M/S | missing |

---

## 3. Music

### 3.1 What ships

The planned stem system was replaced by a playlist: `ALTTMusicPlayer` (`Source/CleanCutAction/Audio/`) is placed in both cafe maps with the six `MusicFX/` tracks, shuffles them, plays each once and crossfades to the next. The player can mute music (`WBP_MusicToggle`, pause menu, touch button). The dance emote plays `EmoteMusic/paroparo`, kept out of `MusicFX/` so the playlist doesn't pick it up.

### 3.2 Planned track list

| ID | Use | Pri | Status |
|---|---|---|---|
| `MUS_MainMenu` | Main menu | M | missing (no main menu map; the game starts on the briefing screen) |
| `MUS_P1_Stem1_Base` … `MUS_P1_Stem4_Melody` | Phase 1 layered stems | M | replaced by the `MusicFX` playlist |
| `MUS_P1_Complete` | Phase 1 completion | M | missing (`SW_next_phase` plays instead) |
| `MUS_P2_PreRoll`, `MUS_P2_ActA`, `MUS_P2_ActB`, `MUS_P2_ActC` | Phase 2 per-act music | S/M | replaced by the `MusicFX` playlist |
| `MUS_P2_Finale` | Final lean-in | M | missing |
| `MUS_Wrap` | "That's a wrap!" | M | missing (`SW_applause` plays instead) |
| `MUS_Credits` | Credits | S | missing |
| `MUS_Sting_Cut` | CUT interstitial | M | missing (`SW_cut` plays instead) |
| `MUS_Sting_Fired`, `MUS_Sting_ActTitle` | Stingers | S | missing |

### 3.3 Phase 1 layering / Phase 2 transitions (plan, not built)

- Phase 1: four synced stems raised by zone completion; duck music −6 dB during barks.
- Phase 2: crossfade per act; music to 50% during cue windows; silence on the final flash, then `MUS_Wrap`.

---

## 4. Implementation Checklist

- [ ] Sound Classes (Master → Music, SFX, Voice, UI, Ambience) with Sound Mix volumes. None yet; music mute is a code toggle.
- [ ] Submixes: `SMX_Megaphone`, `SMX_Walkie`, `SMX_Pause`.
- [x] Prop attenuation: `SFX/Props/ATT_Props` (used by `DA_PropSounds`). `ATT_Station`, `ATT_Ambience` missing.
- [ ] Concurrency: `CON_Bark`, `CON_PropImpact`, `CON_Footstep`, `CON_Chime`.
- [x] Pitch variation for prop sounds (`ULTTPropSoundSet.PitchRange` 0.92–1.08). No random containers / MetaSounds.
- [ ] `MS_PlaceChime` with a "Step" input.
- [x] Looping appliance/station sounds on Audio Components owned by their actors (`ULTTUsableComponent::PlayOwnerSound`, stations restart their audio on each cue).
- [ ] UI sounds that play while paused.
- [ ] Loudness pass (dialogue −16 LUFS-ish, SFX peaks below −3 dBFS, music ~6 dB under dialogue).
- [ ] Per-platform audio cook quality if a build is too big (music is the largest audio; currently quality 80).
- [ ] `CREDITS.md` with every sound and track source.
