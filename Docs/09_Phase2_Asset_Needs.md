# Phase 2 – Asset Status

What Phase 2 has and what it still needs from outside the project. Each row names the slot it plugs into, so a new file can be connected without code changes. Level: `L_NYC_Cafe_Phase2_Michael`. Status checked 2026-11-15 against the content folder and the level's asset references.

Keep the itch.io 200 MB limit in mind: short mono SFX (OGG/WAV, 22–44 kHz), low-poly props, no 4K textures.

## Sound effects

### Station effects – done

All in `/Game/_CleanCutAction/SFX/stations/` and referenced by the level (commit "Added sound effects for phase 2"). Sources were the CC0 Freesound picks below. The pairing per station is from the asset names. The wiring itself is in the binary `.umap`.

| Station actor | Cue | `EffectSound` | `MissSound` |
|---|---|---|---|
| `Station_Slate` | Slate | `SW_ClapperboardClap` | – |
| `Station_CarRail` | Traffic | `SW_CarBeepBeep` | `SW_BicycleBell` |
| `Station_DoorBell` | Door | `SW_ShopDoorBell` | `SW_CowBell` |
| `Station_Espresso` | Steam | `SW_Espresso` | `SW_CoffeeSputter` |
| `Station_PropTable1F` | Napkins | `SW_PaperSlide` | `SW_LoudBonk` |
| `Station_Thunder` | Thunder | `SW_ThunderClap` | `SW_SheetWobble` |
| `Station_LightRig` | Sunset / Moonlight | `SW_SwitchOn` | `SW_SwitchOff` |
| `Station_WindFan` | Wind | `SW_WindFan` | `SW_FanTurnOff` |
| `Station_BoomMic` | Boom | `SW_Creak` | `SW_MicFeedback` |
| `Station_RainRig` | Rain | `SW_LightRain` (looping, stays on) | `SW_WaterDrop` |
| `Station_PropTable2F` | Umbrella / Roses | `SW_UmbrellaOpen` | `SW_LoudBonk` |

Also imported and still unused: `SW_Whoosh`, `SW_UiClick`, `SW_UiHover`.

<details><summary>Freesound sources (CC0)</summary>

| Station | EffectSound | MissSound |
|---|---|---|
| Slate | [JM_MOVIE_Clapperboard](https://freesound.org/people/Julien_Matthey/sounds/484323/) | – |
| CarRail | [passing car](https://freesound.org/people/NachtmahrTV/sounds/553259/) + [Car horn beep beep](https://freesound.org/people/AmishRob/sounds/423990/) | [bicycle-bell_19](https://freesound.org/people/newagesoup/sounds/383631/) |
| DoorBell | [shop_door_bell](https://freesound.org/people/3bagbrew/sounds/57743/) | [Cowbell2](https://freesound.org/people/Neotone/sounds/75339/) |
| Espresso | [Coffee Maker – Water Pour, Steam Hiss](https://freesound.org/people/MegaPenguin13/sounds/118202/) | [Coffee machine brewing, boiling](https://freesound.org/people/TRP/sounds/717444/) |
| PropTable1F | [PaperSlide](https://freesound.org/people/eyesonlegs/sounds/464302/) | [Comedic Loud Bonk](https://freesound.org/people/cb9382/sounds/871191/) |
| Thunder | [Thunder Clap](https://freesound.org/people/DaniloSFX/sounds/672776/) | [Sheet-metal wobble](https://freesound.org/people/Oldome/sounds/862785/) |
| LightRig | [Light Switch ON / OFF](https://freesound.org/people/FillSoko/sounds/257958/) | – |
| WindFan | [Electric mini fan turn on](https://freesound.org/people/lokmansalikoon/sounds/735113/) | [electric fan + shut off](https://freesound.org/people/kyles/sounds/452485/) |
| BoomMic | [short creaks](https://freesound.org/people/HowardV/sounds/393800/) | [Microphone feedback](https://freesound.org/people/JavierSerrat/sounds/470111/) |
| RainRig | [Light Rain](https://freesound.org/people/soundrecorder7/sounds/167034/) | [Drop – Water](https://freesound.org/people/mattfinarelli/sounds/533146/) |
| PropTable2F | [Umbrella open/close](https://freesound.org/people/ValentinPetiteau/sounds/628659/) | (reuses the 1F bonk) |

</details>

### Director (`ALTTSceneDirector`) – done

Defaults set in the constructor (`/Game/_CleanCutAction/SFX/`); override per level in Details.

| Slot | Sound |
|---|---|
| `CueCallSound` | `SW_notification` |
| `CueGoodSound` | `SW_success` |
| `CueMissSound` | `SW_miss` |
| `CutSound` | `SW_cut` |
| `WrapSound` | `SW_applause` |

Phase 1 (`ALTTMessDirector`): `SetReadySound` = `SW_next_phase`, `TimeUpSound` = `SW_miss`, played when the end screen opens.

### Music – in

The level has an `ALTTMusicPlayer` using the `/Game/_CleanCutAction/MusicFX/kf*` tracks (shared with Phase 1).

### Still missing (no slot yet; each needs a small hook)

- Ambient cafe room tone / crew walla.
- Footsteps for Dash and Juniper (no Anim Blueprint, so this needs anim notifies on the blend-space clips or a code timer).
- Marlowe and actor voice blips. The slot is `FLTTDialogueLine::Voice` per line, or a `ReceiveLineSpoken` Blueprint on the film actor.
- An "in shot" alarm beep.

## Models

| What | Slot | Status |
|---|---|---|
| Napkin stack | `Station_PropTable1F` → `TossMeshes[Napkins]` | **Missing.** `TossMeshes` isn't set, so it's the grey engine cube |
| Umbrella (closed) | `Station_PropTable2F` → `TossMeshes[Umbrella]` | **Missing.** Cube |
| Rose bouquet | `Station_PropTable2F` → `TossMeshes[Roses]` | **Missing.** Cube |
| Clapperboard | `Station_Slate_Visual` mesh | **Missing** (placeholder). A `clapperboard` UI icon exists, no mesh |
| Rain rig / sprinkler pipe | `Station_RainRig_Visual` mesh | **Missing** (placeholder) |
| Standing fan | `Station_WindFan_Visual` mesh | **Missing** (placeholder) |
| Thunder sheet | `Station_Thunder_Visual` mesh | **Missing** (placeholder; BroadcastStudio panel) |
| Bell rope / door bell | `Station_DoorBell_Visual` mesh | **Missing** (placeholder) |
| Cardboard car cut-out (optional) | New actor in `Station_CarRail.EffectActors` | **Missing.** Only `FX_TrafficHeadlights` fires |
| Film camera | `ALTTFilmCamera.BodyMesh` | **In:** `/Game/BroadcastStudio/Meshes/SM_BS_Camera` + `SM_BS_CameraStand` |
| Light rig | `Station_LightRig_Visual` | **In:** Hollywood_Vol3 light stands (`SM_Light_Stand_*`, `SM_Production_Light_01a`) |
| Dash / Juniper | `ALTTFilmActor` mesh | **In:** `/Game/ST_Characters_JC/Models/SKM_StylizedMale`, `SKM_StylizedFemale` |

The placeholder status of the `*_Visual` meshes is from the previous version of this list. The level doesn't reference any new prop meshes for them, so they are assumed unchanged.

## Animations (Dash and Juniper)

**In:** `/Game/_CleanCutAction/Characters/BS_Dash_Locomotion` and `BS_Juniper_Locomotion` (idle/walk/run from `ST_Characters_JC/Animation/Male/MM_*`). `ALTTFilmActor` drives them as a single-node blend space, with no Anim Blueprint.

**Missing:** every montage. The `Montages` and `TalkMontages` maps are empty in the level, so script lines play no gestures. Montages must be on the `ST_Characters_JC` skeletons (`SK_StylizedMale`, `SK_StylizedFemale_UE`) or retargeted to them. Montages normally play through a slot node in an Anim Blueprint. The meshes currently run a single-node blend space, so test that a montage really shows. If it doesn't, add a minimal Anim Blueprint (speed blend space + `DefaultSlot`). Add each montage to the actor's `Montages` map with exactly these names:

| Name | Used in (Scene 42) | Count |
|---|---|---|
| `Laugh` | Post-cue reactions | 5 |
| `Startled` | Toss misses (bonk), knocked cup | 4 |
| `StormOff` | Juniper storms upstairs (also switches to RunSpeed, no montage needed for that) | 3 |
| `LeanIn` | Final thunder | 3 |
| `Catch` | Successful toss | 3 |
| `Angry` | Argument beats | 2 |

Also a few generic `TalkMontages` per actor.

## VFX

| Effect | Asset | Status |
|---|---|---|
| Rain | `/Game/_CleanCutAction/VFX/NS_Rain` → `FX_Rain` (Rain Rig) | **In** |
| Wind | `/Game/_CleanCutAction/VFX/NS_Wind` → `FX_Wind` (Wind Fan) | **In** |
| Steam | `/Game/_CleanCutAction/VFX/NS_Steam` → `FX_Steam` (Espresso) | **In** |
| Thunder flash | `FX_ThunderFlash` light actor (Thunder) | **In** |
| Traffic headlights | `FX_TrafficHeadlights` light actor (Car Rail) | **In** |
| Sunset / moonlight | `KeyLight_Gel` colour from the Light Rig choice | **In** (key light colour only) |
| Frame cone | `/Game/_CleanCutAction/Materials/M_FrameCone` | **In** (cooked via `MapsToCook`) |
| `MPC_World` (rain wetness, sky time of day, wind on foliage) | `ALTTSceneDirector.WorldMPC` | **Missing.** No asset exists; the director's parameter tweens do nothing |
| Raindrop / smoke sprite textures | `NS_Rain`, `NS_Steam` | Optional. They use the default soft sprite |
| Missed-cue "drizzle" versions | `bShowEffectOnMiss` on a station | Not used. Misses only play the miss sound |
