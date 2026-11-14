# Phase 2 – Assets to Find

Everything Phase 2 still needs from outside the project. Each row names the slot it plugs into, so once a file is imported it can be connected without code changes. Level: `L_NYC_Cafe_Phase2_Michael`.

Keep the itch.io 200 MB limit in mind: short mono SFX (OGG/WAV, 22–44 kHz), low-poly props, no 4K textures.

## Sound effects

### Station effects (`EffectSound` on success, `MissSound` on a missed cue)

| Station actor | Cue | EffectSound | MissSound |
|---|---|---|---|
| `Station_Slate` | Slate | Clapperboard clap | – |
| `Station_CarRail` | Traffic | Car pass-by with a short "meep meep" horn | Sad bicycle bell |
| `Station_DoorBell` | Door | Cafe door bell jingle | Bell clunk / off-time jingle |
| `Station_Espresso` | Steam | Espresso steam hiss (2–3 s) | Sputter / gurgle |
| `Station_PropTable1F` | Napkins | Soft paper whoosh + catch | Bonk |
| `Station_Thunder` | Thunder | Thunder crack + rumble | Weak sheet-metal wobble |
| `Station_LightRig` | Sunset / Moonlight | Light switch clack + hum | Wrong-gel buzz |
| `Station_WindFan` | Wind | Fan spin-up + wind gust (4 s) | Fan sputter |
| `Station_BoomMic` | Boom | Mic boom creak (subtle) | Mic thump / feedback squeal |
| `Station_RainRig` | Rain | Rain loop (loopable, starts and stays on) | Drizzle / single drip |
| `Station_PropTable2F` | Umbrella / Roses | Whoosh + umbrella pop | Bonk |

### Freesound picks for the stations (all CC0, no attribution needed)

Trim long recordings to the part described; import as mono. `+` = layer the two clips.

| Station | EffectSound | MissSound |
|---|---|---|
| Slate | [JM_MOVIE_Clapperboard](https://freesound.org/people/Julien_Matthey/sounds/484323/) 0.5 s (alt [Clapboard sound](https://freesound.org/people/uEffects/sounds/207867/)) | – |
| CarRail | [passing car](https://freesound.org/people/NachtmahrTV/sounds/553259/) 13.7 s, trim to ~3 s + [Car horn beep beep](https://freesound.org/people/AmishRob/sounds/423990/) 0.7 s | [bicycle-bell_19](https://freesound.org/people/newagesoup/sounds/383631/) 3.4 s |
| DoorBell | [shop_door_bell](https://freesound.org/people/3bagbrew/sounds/57743/) 8 s, trim (alt [Shop door bell](https://freesound.org/people/775noise/sounds/494565/)) | [Cowbell2](https://freesound.org/people/Neotone/sounds/75339/) 0.35 s |
| Espresso | [Coffee Maker – Water Pour, Steam Hiss](https://freesound.org/people/MegaPenguin13/sounds/118202/) 9.7 s, take the hiss (alt [Steaming Milk](https://freesound.org/people/Sounds_by_Cois/sounds/553927/)) | [Coffee machine brewing, boiling](https://freesound.org/people/TRP/sounds/717444/) 6 min, cut a 1–2 s sputter |
| PropTable1F | [PaperSlide](https://freesound.org/people/eyesonlegs/sounds/464302/) 1.6 s | [Comedic Loud Bonk](https://freesound.org/people/cb9382/sounds/871191/) 0.45 s |
| Thunder | [Thunder Clap](https://freesound.org/people/DaniloSFX/sounds/672776/) 16 s, trim to ~5 s (alt [Thunder Clap OWB](https://freesound.org/people/Dave%20Welsh/sounds/194364/)) | [Sheet-metal wobble](https://freesound.org/people/Oldome/sounds/862785/) 44 s, cut ~1.5 s |
| LightRig | [Light Switch ON / OFF](https://freesound.org/people/FillSoko/sounds/257958/) 12.8 s, one click + [Fluorescent hum](https://freesound.org/people/soundandmelodies/sounds/776265/) | [Wrong Answer Buzzer](https://freesound.org/people/-Andreas/sounds/650842/) 0.3 s (or [electric hum buzz short](https://freesound.org/people/kyles/sounds/451860/)) |
| WindFan | [Electric mini fan turn on](https://freesound.org/people/lokmansalikoon/sounds/735113/) 28 s, take the spin-up + [Wind Gust](https://freesound.org/people/crashoverride6/sounds/146932/) 7.7 s | [electric fan + shut off](https://freesound.org/people/kyles/sounds/452485/) 1:47, cut the spin-down |
| BoomMic | [short creaks](https://freesound.org/people/HowardV/sounds/393800/) 15 s, one creak | [Tapping the Microphone](https://freesound.org/people/qubodup/sounds/741035/) 1.6 s or [Microphone feedback](https://freesound.org/people/JavierSerrat/sounds/470111/), cut ~1 s |
| RainRig | [Light Rain](https://freesound.org/people/soundrecorder7/sounds/167034/) 26 s, loop it (alt [Rain LOOP](https://freesound.org/people/speakwithanimals/sounds/525046/), 14 min, cut 20 s) | [Drop – Water](https://freesound.org/people/mattfinarelli/sounds/533146/) 0.6 s |
| PropTable2F | [Whoosh](https://freesound.org/people/qubodup/sounds/60013/) 0.4 s + [Umbrella open/close](https://freesound.org/people/ValentinPetiteau/sounds/628659/), one open | [Bonk.wav](https://freesound.org/people/Harrisando/sounds/466202/) 1.8 s (or reuse the 1F bonk) |

### Director (`LTTSceneDirector` actor) – done

Defaults set in the `ALTTSceneDirector` constructor (`/Game/_CleanCutAction/SFX/`); override per level in Details.

| Slot | Sound |
|---|---|
| `CueCallSound` | `SW_notification` |
| `CueGoodSound` | `SW_success` |
| `CueMissSound` | `SW_miss` |
| `CutSound` | `SW_cut` |
| `WrapSound` | `SW_applause` |

Phase 1 (`LTTMessDirector`): `SetReadySound` = `SW_next_phase`, `TimeUpSound` = `SW_miss`, played when the end screen opens.

### Optional, no slot yet (would need a small hook)

- Ambient cafe room tone / crew walla for the set.
- Footsteps on wood for Dash and Juniper (anim notifies).
- Marlowe and actor voice blips (gibberish per line, `ReceiveLineSpoken` event).

## Models

| What | Slot | Notes |
|---|---|---|
| Napkin stack | `Station_PropTable1F` → `TossMeshes[Napkins]` | Currently a grey cube |
| Umbrella (closed) | `Station_PropTable2F` → `TossMeshes[Umbrella]` | Currently a grey cube |
| Rose bouquet | `Station_PropTable2F` → `TossMeshes[Roses]` | Currently a grey cube |
| Clapperboard | Replace `Station_Slate_Visual` mesh | Currently a cardboard box |
| Rain rig / sprinkler pipe | Replace `Station_RainRig_Visual` mesh | Currently a ladder |
| Standing fan | Replace `Station_WindFan_Visual` mesh | Currently an electric box |
| Thunder sheet (metal sheet on a stand) | Replace `Station_Thunder_Visual` mesh | Currently a broadcast panel |
| Bell rope / door bell | Replace `Station_DoorBell_Visual` mesh | Currently an electric box |
| Cardboard car cut-out (optional) | New effect actor under `Station_CarRail.EffectActors` | Headlight light already fires |

## Animations (Dash and Juniper)

The characters use the `ST_Characters_JC` skeletons (`SK_StylizedMale`, `SK_StylizedFemale_UE`), so montages must be on those skeletons or retargeted to them. Add each to the actor's `Montages` map with exactly these names:

| Name | Used in | Count |
|---|---|---|
| `Laugh` | Post-cue reactions | 5 |
| `Startled` | Toss misses (bonk) | 4 |
| `StormOff` | Off-script walk-outs (uses RunSpeed) | 3 |
| `LeanIn` | Romantic beats | 3 |
| `Catch` | Successful toss | 3 |
| `Angry` | Argument beats | 2 |

Also a few generic `TalkMontages` (gesture while talking) per actor.

## VFX status

Built in `/Game/_CleanCutAction/VFX/` and wired to their stations: `NS_Rain` (`FX_Rain`), `NS_Wind` (`FX_Wind`), `NS_Steam` (`FX_Steam`). They use the default soft sprite material. A raindrop/smoke texture would improve them but isn't required.
