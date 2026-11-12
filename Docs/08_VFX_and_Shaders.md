# VFX & Shaders

Covers checklist **P10**. Goal: a few signature effects that make the game look polished and on-theme, without heavy GPU cost. All Niagara systems in `Content/CleanCutAction/VFX/`, materials in `Materials/`.

**Shared global parameters - `MPC_World` (Material Parameter Collection):**

| Param | Type | Range | Driven by |
|---|---|---|---|
| `Wetness` | Scalar | 0–1 | Phase 2 rain (lags rain by 4 s) |
| `RainIntensity` | Scalar | 0–1 | Rain valve/coverage |
| `LightningFlash` | Scalar | 0–1 | Thunder flash pulse |
| `WindStrength` | Scalar | 0–1 | Fan |
| `BackdropTime` | Scalar | 0–2 | Gel cues (day → sunset → night) |
| `GhostPulseTime` | Scalar | seconds | Game time for synced pulses |
| `PlayerPos` | Vector | world | For proximity reveals |

---

## 1. Niagara Systems

### 1.1 Phase 1 - Cleaning feedback

| System | Description | Spec | Pri |
|---|---|---|---|
| `NS_PlaceSparkle` | Gold sparkle burst at placement | 20–30 sprite sparks, gold → white, burst radius 40 cm, gravity −200, lifetime 0.4–0.7 s, plus 1 expanding ring sprite (ground-aligned, 0 → 80 cm, 0.3 s) | M |
| `NS_WrongPuff` | Small red-grey puff on wrong drop | 8 soft smoke sprites, 0.4 s | M |
| `NS_DustPuff` | Dust on landing, right-it, respawn | 12 smoke sprites, ring emission at ground, beige, 0.6 s | M |
| `NS_SweepDebris` | Kernels/confetti kicked by broom | Mesh particles (kernel mesh) in a 60° cone in sweep direction, 10–25 per stroke, bounce on collision (CPU, depth buffer) | M |
| `NS_TrashSwish` | Swish confetti from bin mouth | 15 confetti quads, upward, gravity, 0.8 s, team colors (gold/white) | M |
| `NS_PlugSpark` | Electrical spark when plugging cable | 10 ribbon/sprite sparks, blue-white, 0.2 s + light flash 0.1 s | S |
| `NS_LensGlint` | Star glint on clean lens | 1 four-point star sprite scaling 0 → 1 → 0, 0.4 s, rotating | M |
| `NS_GroupLamp` | Little warm bloom when a group lamp turns on | 1 soft glow sprite + 5 motes | M |
| `NS_ZoneComplete` | Zone celebration | Volumetric-looking light shaft cone (mesh with additive material), 40 floating dust motes catching light, 30 confetti from above the zone, 2.0 s | M |
| `NS_GoldenReelShimmer` | Idle shimmer around golden reels | 6 orbiting sparkles + soft glow, looping | M |
| `NS_GoldenReelGet` | Pickup burst | 40 gold sparks + ring | M |
| `NS_CreditFly` | (optional) world-to-HUD trail | Short ribbon toward camera before UI takes over | C |

### 1.2 Phase 1 - Abilities

| System | Description | Pri |
|---|---|---|
| `NS_DirectorsEye_Cast` | Cyan ring at player's feet expanding with the post-process scan wave | S |
| `NS_GrabBag_Trail` | Cyan ribbon trail attached to each pulled prop | S |
| `NS_SetDresser_Trail` | Gold ribbon + sparkles following each prop home | S |
| `NS_AbilityReady` | Tiny screen-space sparkle at the HUD icon (UI Niagara if available, else widget animation) | C |

### 1.3 Phase 2 - Effects (the showpieces)

| System | Description | Spec | Pri |
|---|---|---|---|
| `NS_RainSprinkler` | Rain from the rig over the terrace | **GPU** sprites, spawn box above terrace (6 × 4 m), 2000–6000/s scaled by `RainIntensity`, velocity-aligned stretched streaks, depth-buffer collision → kill + event | M |
| `NS_RainSplash` | Splashes on surfaces | Spawned from collision events (or a separate box spawning on surface heights): tiny crown sprites + 3 droplets, 0.2 s | M |
| `NS_HoseSpray` | Visible spray from the hose nozzle | Cone of streaks + mist sprites toward aim point, follows nozzle; drives where `NS_RainSprinkler` is centered | M |
| `NS_RainDrizzle` | Miss version | 300/s, small area | M |
| `NS_EspressoSteam` | Steam from machine | Soft smoke sprites rising, curl noise; parameter `Pressure` scales spawn and size; over-steam variant fills the room (500 sprites, lifetime 6 s, fades) | M |
| `NS_ThunderFlash` | Flash flicker | Not particles: drives `LightningFlash` + a rect light pulse; optional 1 big additive "flash card" sprite over the backdrop | M |
| `NS_WindDebris` | Leaves/napkins on wind | Mesh particles (leaf, napkin) with velocity from fan direction × `WindStrength`, curl noise; over-wind spawns the scarf mesh flying away | S |
| `NS_RosePetals` | Petals on roses catch/toss | 20 petal mesh particles, flutter (curl noise), 2 s | M |
| `NS_CarRailDust` | Dust behind the cardboard car | Small puffs from wheels, rate by speed | S |
| `NS_UmbrellaDrops` | Rain bouncing off umbrella | Sprites spawned on umbrella surface while in rain | C |
| `NS_WrapConfetti` | Confetti from catwalks at "That's a wrap!" | 800 confetti quads, multicolor, flutter, 4 s | M |
| `NS_MicBump` | Comic star burst at the boom-mic bonk | 5 cartoon stars circling | S |
| `NS_Bonk` | Prop bonk on actor head | Same cartoon stars | M |

### 1.4 Performance rules
- Rain: GPU sim, fixed bounds, max 8k particles, scalability: Low = 30% spawn, no splashes.
- Turn off collision-heavy CPU emitters on Low.
- Cull all Phase 1 FX when off-screen > 2 s (default Niagara culling via effect type `ET_Gameplay`).
- Pool frequently spawned systems (`NS_PlaceSparkle`, `NS_DustPuff`) via Niagara component pooling.

---

## 2. Materials & Shaders

### 2.1 `M_GhostTapeMark` - the placement ghost (signature look)
Translucent, unlit, used on the ghost copy of each prop.
- **Base:** `TapeYellow` emissive at 0.6, opacity 0.18.
- **Fresnel edge:** exponent 3, adds +0.5 opacity and +2 emissive at the silhouette.
- **Scanlines:** world-space Z stripes (`frac(WorldPos.Z / 6 + Time × 0.5)`), thin lines adding emissive.
- **Pulse:** opacity × (0.8 + 0.2 × sin(Time × 4)).
- **Hover state (param `Active` 0/1):** color lerp → `Gold`, pulse speed ×2, opacity ×2.
- **Depth fade** to avoid hard intersections with the floor.
- **Proximity reveal:** extra opacity when `distance(PlayerPos, WorldPos) < 300 cm`.
- Also: `M_TapeDecal` (deferred decal): yellow gaffer-tape "X" or "L" corners with worn edges; param `Done` swaps to a check mark and tints green-gold.

### 2.2 `PP_Outline` - stencil outlines
Post-process material (Before Tonemapping or After, test both) reading `CustomStencil`.
- Edge detection: sample stencil at 4–8 neighbors (offset by `OutlineWidth` = 2 px at 1080p, scale by resolution). Pixel is an edge if its stencil differs from neighbors and one of them is non-zero.
- Color by stencil value (see `02 §5`): 1 white, 2 gold, 3 cyan, 4 red, 5 green.
- Stencil 3 (Director's Eye) draws **through walls**: use CustomDepth vs SceneDepth comparison to render occluded parts with 50% opacity + a hatch pattern.
- Animated thickness pulse for stencil 2 and 5.

### 2.3 `PP_DirectorsEye_Scan`
Post-process blendable enabled during the ability cast:
- A ring expanding from the player world position: `abs(distance(WorldPos, Origin) − Radius) < Thickness` → cyan emissive band.
- Behind the ring: slight desaturation (30%) + cool tint for the ability's duration; interactables keep color (stencil mask).

### 2.4 `MF_Wetness` - wet set surfaces (Phase 2)
Material function inserted into the master set materials (`M_Set_Master`, `M_Prop_Master`):
- Darken base color by `lerp(1, 0.6, Wetness)`.
- Roughness → `lerp(R, 0.08, Wetness × PuddleMask)` (puddle mask = world-aligned noise texture, flat surfaces only via world normal Z > 0.8).
- Raindrop ripples: flipbook ripple normal texture on horizontal surfaces, intensity × `RainIntensity`.
- Vertical surfaces: streaking drops (panning normal map) × `RainIntensity`.
- Restrict to the terrace area with a world-position box mask (params `WetBoxMin/Max`) so the 1F stays dry.

### 2.5 `M_FrameCone` + `M_FrameFloorDecal`
- Frustum mesh material: translucent, unlit, two-sided.
  - Fill opacity 0.035; edges (UV-based edge mask on the frustum mesh) opacity 0.35.
  - Animated diagonal stripes moving away from the camera (`frac((U + V) × 20 − Time × 0.5)`).
  - Depth fade 30 cm.
  - Param `State`: 0 neutral (white), 1 warning (orange), 2 violation (red, stripes speed ×3), 3 slate (green).
- Floor decal: projects the frustum footprint as hatched lines in `TapeYellow`, matching `State`.

### 2.6 `M_BackdropTranslite`
Large emissive plane behind the street flat.
- Three textures: day skyline, sunset skyline, night skyline (paint them or source/generate; must be license-clean).
- `BackdropTime` blends 0 → 1 → 2 (lerp day→sunset, then sunset→night).
- Emissive strength ×(1 + `LightningFlash` × 4).
- Slight canvas texture overlay to sell "painted backdrop".

### 2.7 `M_LensSmudge`
On the film camera's front glass.
- Base glass (translucent, low opacity, high specular).
- Smudge mask texture (fingerprints + a big cartoony lick mark) multiplied by `Smudge` (1 → 0 as you wipe): adds opacity, roughness, and a slight brown tint.
- Optional: wiped area reveal using a radial mask around the last wipe UV (keep simple: global fade is fine).

### 2.8 `M_CardboardCar`
Painted cardboard look: base color texture with visible brush strokes, a corrugated cardboard edge texture on the cutout's thin sides, wobble via World Position Offset driven by a `Wobble` param (sine on X along height).

### 2.9 `M_StageLight_Beam`
Additive cone mesh in front of each stage light: soft volumetric look (radial gradient × length falloff × slight noise flicker), intensity tied to the light's on/aim state. Color follows the light (warm when aimed, cool when not).

### 2.10 Character wetness (Phase 2) [C]
Actor materials get a `Wetness` input: darker cloth, glossier hair/skin, driven by `MPC_World.Wetness` only while the actor is inside the rain box (per-actor dynamic material instance param).

---

## 3. Post-Process & Screen Effects

### 3.1 Global look
- Slight warm grade, mild bloom, filmic tonemapper defaults, light vignette (0.25).
- Phase 1: work-light look, a bit flat and cool, saturation 0.9. As zones complete, a post-process volume's blend weight increases toward a warm "set ready" grade (saturation 1.05, warmer white balance). Gives the whole level a visible progress curve.

### 3.2 `PP_Cinematic` (Phase 2 while rolling)
Blendable with weight tween 0 → 1 on ACTION (0.6 s), back on CUT/wrap:
- Film grain (intensity 0.25), slight chromatic aberration at edges (0.1), stronger vignette (0.45), teal-orange grade (shadows teal, highlights warm).
- Letterbox handled by UI bars (cheaper, crisper).
- Combined with `LightningFlash`: exposure bias spike +2 EV for the flash frames.

### 3.3 Freeze-frame (CUT interstitial / Patience)
Post-process param `Freeze` 0 → 1: desaturate 100%, add sepia tint 20%, slight halftone overlay (optional), vignette 0.6. Use time dilation 0 for CUT (behind the UI) and 0.0 play rate on actors for Patience.

### 3.4 Frame-violation vignette
UI image (red radial gradient) pulsing; no post-process cost.

### 3.5 Screen droplets [C]
When the player stands in the rain box: post-process with a droplets normal-map overlay distorting scene color, fading in/out over 1 s.

---

## 4. Lighting Moments (cheap, high impact)

| Moment | Implementation |
|---|---|
| Zone complete | Stage spotlight intensity PowerOn tween (`02 §7`), `NS_ZoneComplete` shaft, post-process warmth step |
| Phase 1 complete | All stage lights PowerOn in a wave (0.15 s stagger), then exposure settles |
| Sunset / Moonlight | Key light color + temperature tween over 1.2 s, backdrop blend 2 s |
| Thunder | Rect light behind the backdrop: double flicker (0 → 1 → 0.2 → 1 → 0 over 0.3 s) + `LightningFlash` |
| Final kiss | All lights dim to 15% over 1 s, one lightning flash silhouettes the actors against the backdrop, then cut to black |
| Wrap | All set lights full + confetti + a warm white "work light" bloom |
