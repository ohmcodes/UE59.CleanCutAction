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

### Director (`LTTSceneDirector` actor)

| Slot | Sound |
|---|---|
| `CueCallSound` | Megaphone squawk / short whistle |
| `CueGoodSound` | Positive ding |
| `CueMissSound` | Comedic "womp" |
| `CutSound` | "CUT!" stinger / record scratch |
| `WrapSound` | Applause + "That's a wrap" sting |

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
