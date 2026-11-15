# VFX & Shaders

Covers checklist **P10**. Goal: a few signature effects that make the game look polished and on-theme, without heavy GPU cost. Niagara systems live in `/Game/_CleanCutAction/VFX/`, materials in `/Game/_CleanCutAction/Materials/`, breakables in `BreakFX/` and `Destruction/`.

**Status** is checked against `Content/_CleanCutAction/` and the cooked maps on 2026-11-15:

- **in** — the asset exists and something uses it (path under `/Game/_CleanCutAction/`)
- **partial** — the code hook exists but the asset doesn't, or a simpler version shipped
- **missing** — not made yet
- **no mechanic** — the feature it was for isn't in the game

---

## 0. What's in the project

| Asset | Used by |
|---|---|
| `VFX/NS_MopSplash` | Puddle mopping (`ALTTWaterPuddle`, loaded by path, in `MapsToCook`) |
| `VFX/NS_Sparks` (material `M_Spark`) | Power box sparks (`BP_ElectricBox`) |
| `VFX/NS_SinkBubbles` (material `M_Bubble`) | Dish washing (`BP_Sink`) |
| `VFX/NS_Rain`, `VFX/NS_Steam`, `VFX/NS_Wind` | Phase 2 effect stations (in `L_NYC_Cafe_Phase2_Michael`) |
| `BreakFX/GC_Generic_Shatter` | Fallback Chaos geometry collection for props without their own (`ALTTInteractableBase`, in `MapsToCook`) |
| `Destruction/GC_*` (70 geometry collections) | Per-prop break pieces |
| `Materials/M_GhostSilhouette` | Placement ghost on every prop (in `MapsToCook`) |
| `Materials/M_BrokenProp` | Broken pieces (in `MapsToCook`) |
| `Materials/PostProcess/M_PP_Outline` | Focus/hint outline, CustomStencil 1 (`BP_FirstPersonCharacter`) |
| `Materials/M_FrameCone` | Film camera frame cone (`ALTTFilmCamera`, in `MapsToCook`) |
| `Materials/M_WaterPuddle`, `M_WaterSplash`, `M_WaterStream` | Puddles, sink water |
| `Materials/M_CoffeeGrounds`, `M_CoffeeLiquid`, `M_CoffeeStream` | Coffee machine (`ULTTCoffeeMachineComponent`) |
| `Materials/M_MopSolid`, `MI_Mop_Plastic`, `MI_Mop_Wood`, `MI_Mop_Yarn` | Mop |
| `Materials/M_RedDot` | Power box indicator (`BP_ElectricBox`) |
| `Materials/Mirror/M_Mirror`, `M_MirrorFrame`, `RT_MirrorDefault` | Mirror actor (`ALTTMirror`, in `MapsToCook`) |

---

## Shared global parameters — `MPC_World`

**Status: partial.** `ALTTSceneDirector.WorldMPC` is an optional slot and the director drives `RainIntensity`, `Wetness`, `LightningFlash` (0.3 s double flicker), `WindStrength` and `BackdropTime` when cues land. **No `MPC_World` asset exists**, so nothing reads them yet.

| Param | Type | Range | Driven by | Status |
|---|---|---|---|---|
| `Wetness` | Scalar | 0–1 | Phase 2 rain | driven by code, no asset |
| `RainIntensity` | Scalar | 0–1 | Rain cue | driven by code, no asset |
| `LightningFlash` | Scalar | 0–1 | Thunder cue | driven by code, no asset |
| `WindStrength` | Scalar | 0–1 | Fan cue | driven by code, no asset |
| `BackdropTime` | Scalar | 0–2 | Gel cues (day → sunset → night) | driven by code, no asset |
| `GhostPulseTime` | Scalar | seconds | Synced pulses | missing |
| `PlayerPos` | Vector | world | Proximity reveals | missing |

---

## 1. Niagara Systems

### 1.1 Phase 1 - Cleaning feedback

| System | Description | Pri | Status |
|---|---|---|---|
| `NS_PlaceSparkle` | Gold sparkle burst at placement | M | missing |
| `NS_WrongPuff` | Small red-grey puff on a wrong drop | M | missing (wrong drops break the prop: Chaos pieces from `Destruction/GC_*` or `BreakFX/GC_Generic_Shatter`, `M_BrokenProp`) |
| `NS_DustPuff` | Dust on landing, right-it, respawn | M | missing |
| `NS_SweepDebris` | Kernels/confetti kicked by broom | M | no mechanic |
| `NS_TrashSwish` | Confetti from bin mouth | M | no mechanic |
| `NS_PlugSpark` | Electrical spark | S | **in** as `VFX/NS_Sparks` (power box) |
| `NS_LensGlint` | Glint on clean lens | M | no mechanic |
| `NS_GroupLamp` | Warm bloom when a group lamp turns on | M | missing |
| `NS_ZoneComplete` | Zone celebration | M | missing |
| `NS_GoldenReelShimmer`, `NS_GoldenReelGet` | Golden reels | M | no mechanic |
| `NS_CreditFly` | World-to-HUD trail | C | no mechanic |
| (new) Mop splash | Splash while mopping a puddle | — | **in**: `VFX/NS_MopSplash` |
| (new) Sink bubbles | Dish washing | — | **in**: `VFX/NS_SinkBubbles` |

### 1.2 Phase 1 - Abilities

| System | Pri | Status |
|---|---|---|
| `NS_DirectorsEye_Cast`, `NS_GrabBag_Trail`, `NS_SetDresser_Trail`, `NS_AbilityReady` | S/C | no mechanic (no abilities in the game; the Hint highlights the nearest stray prop with the outline instead) |

### 1.3 Phase 2 - Effects (the showpieces)

| System | Description | Pri | Status |
|---|---|---|---|
| `NS_RainSprinkler` | Rain over the terrace | M | **in** as `VFX/NS_Rain` (VERIFY GPU sim and bounds against the spec below) |
| `NS_RainSplash` | Splashes on surfaces | M | missing |
| `NS_HoseSpray` | Spray from the hose nozzle | M | missing |
| `NS_RainDrizzle` | Miss version | M | missing |
| `NS_EspressoSteam` | Steam from the machine | M | **in** as `VFX/NS_Steam` |
| `NS_ThunderFlash` | Flash flicker | M | partial: `LightningFlash` is driven by code; no MPC asset or light hooked to it |
| `NS_WindDebris` | Leaves/napkins on wind | S | **in** as `VFX/NS_Wind` |
| `NS_RosePetals` | Petals on the roses toss | M | missing |
| `NS_CarRailDust` | Dust behind the cardboard car | S | missing |
| `NS_UmbrellaDrops` | Rain bouncing off umbrella | C | missing |
| `NS_WrapConfetti` | Confetti at "That's a wrap!" | M | missing |
| `NS_MicBump`, `NS_Bonk` | Cartoon stars | S/M | missing |

### 1.4 Performance rules (plan)

- Rain: GPU sim, fixed bounds, max 8k particles; Low scalability = 30% spawn, no splashes. The web build's `Wasm_Mobile` profile sets `sg.EffectsQuality=1`.
- Turn off collision-heavy CPU emitters on Low.
- Cull Phase 1 FX off-screen > 2 s (effect type `ET_Gameplay`; no effect types exist yet).
- Pool frequently spawned systems.

---

## 2. Materials & Shaders

| Planned | Status |
|---|---|
| 2.1 `M_GhostTapeMark` (placement ghost) | **in** as `Materials/M_GhostSilhouette` (VERIFY which of the spec's features it has: fresnel, scanlines, pulse, hover state, depth fade, proximity reveal). `M_TapeDecal` missing |
| 2.2 `PP_Outline` (stencil outlines) | **in** as `Materials/PostProcess/M_PP_Outline`. Code only uses stencil 1 (focus, hint target, light fixtures, cheat button); the other stencil colours aren't used |
| 2.3 `PP_DirectorsEye_Scan` | no mechanic |
| 2.4 `MF_Wetness` (wet set surfaces) | missing in `_CleanCutAction` (the ParisStreet pack has its own `ParisStreet/Materials/MF_Wetness`; not wired to `MPC_World`) |
| 2.5 `M_FrameCone` | **in**: `Materials/M_FrameCone` (`ALTTFilmCamera` drives it through a dynamic instance). `M_FrameFloorDecal` missing |
| 2.6 `M_BackdropTranslite` | missing |
| 2.7 `M_LensSmudge` | no mechanic |
| 2.8 `M_CardboardCar` | missing |
| 2.9 `M_StageLight_Beam` | missing |
| 2.10 Character wetness | missing |
| (new) `M_BrokenProp` | **in**: broken prop pieces |
| (new) Water, coffee, mop, mirror, power box materials | **in** (section 0) |

### Spec notes (kept for anything still to build)

- **Ghost:** translucent unlit; `TapeYellow` emissive 0.6, opacity 0.18; fresnel edge (exp 3); world-Z scanlines; pulse `0.8 + 0.2 × sin(Time × 4)`; `Active` param → gold, ×2 pulse and opacity; depth fade; extra opacity within 300 cm of the player.
- **Outline:** edge detect on `CustomStencil` at 4–8 neighbours, 2 px at 1080p; colour by stencil (1 white, 2 gold, 3 cyan, 4 red, 5 green).
- **Wetness:** darken base colour by `lerp(1, 0.6, Wetness)`; roughness → 0.08 in puddle mask on flat surfaces; ripple normals × `RainIntensity`; limit to the terrace with a world box mask.
- **Frame cone:** fill opacity 0.035, edges 0.35, stripes moving away from the camera, depth fade 30 cm; `State` 0 neutral, 1 warning, 2 violation, 3 slate.
- **Backdrop:** day/sunset/night textures blended by `BackdropTime`; emissive × (1 + `LightningFlash` × 4).
- **Cardboard car:** painted base, corrugated edges, WPO wobble.
- **Stage light beam:** additive cone, radial gradient × length falloff, follows the light's colour.

---

## 3. Post-Process & Screen Effects

| Planned | Status |
|---|---|
| 3.1 Global look (warm grade, bloom, vignette; Phase 1 grade warms as zones complete) | missing as a system. Project defaults have bloom, auto exposure and motion blur **off**; the Windows build turns default bloom on (`Config/Windows/WindowsEngine.ini`) |
| 3.2 `PP_Cinematic` (film grain, CA, vignette, teal-orange while rolling) | missing |
| 3.3 Freeze-frame (CUT / Patience) | partial: film actors freeze with `CustomTimeDilation = 0` (`ALTTFilmActor`); no post-process freeze look |
| 3.4 Frame-violation vignette | **in** as a UI red tint over the screen while in shot (`ULTTShootHUD`, "Vignette" border) |
| 3.5 Screen droplets | missing |

---

## 4. Lighting Moments

| Moment | Status |
|---|---|
| Zone complete | missing (no zones) |
| Phase 1 complete | missing (end screen + `SW_next_phase`) |
| Sunset / Moonlight | partial: `BackdropTime` driven by code, nothing reads it |
| Thunder | partial: `LightningFlash` double flicker driven by code (0.3 s), nothing reads it |
| Final kiss | missing |
| Wrap | missing (applause + end screen) |
| (new) Light switches and flicker | **in**: `ULTTLightSwitchComponent`, `ULTTFlickerComponent`, `ALTTLightFixture`, `ALTTPowerBox` + `ULTTPowerSubsystem` |
