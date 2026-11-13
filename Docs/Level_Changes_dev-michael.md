# L_NYC_Cafe: changes on `dev/michael`

Everything changed in `/Game/StylizedNewYorkCity/Level/L_NYC_Cafe` on branch `dev/michael` (commits 5e47e53, 2ec3ffa, 71cb6fd), measured against `2b8baa9 draft playable`. Use it to re-apply or reconcile the changes when merging, since `.umap` files can't be merged by git.

Units are Unreal units (cm). Axes: **+X = east** (right when facing the back wall), **−Y = back wall**, +Y = street front, Z up. Ground floor is Z 0, mezzanine floor Z 300, ceiling Z 600.

Most new actors are in World Outliner folders named `Expansion/*`, which makes them easy to find and copy.

---

## ⚠️ Merge conflict to expect

`main` moved on after this branch was cut: **4beed6a "package polish UI"** (Ohmcodes) also changed `L_NYC_Cafe.umap` (1,092,400 → 1,093,051 bytes, a small edit) and added one line to `Config/DefaultGame.ini`:

```ini
+MapsToCook=(FilePath="/Game/_CleanCutAction/Materials/M_BrokenProp")
```

The `.ini` merges cleanly. The `.umap` **will conflict**, and git can't combine the two versions.

**Recommended merge:**
1. Merge `main` into `dev/michael`. When the `.umap` conflicts, keep **this branch's version**: `git checkout --ours Content/StylizedNewYorkCity/Level/L_NYC_Cafe.umap`. This branch's edits are much larger: hundreds of actors added, moved or converted.
2. Ask the author of 4beed6a what they changed in the level, and redo that small edit by hand in the editor.
3. Play-test in PIE, then commit.

If you'd rather keep `main`'s map and replay this branch on top, use the sections below as the checklist. That is a lot of work; option 1 is far less.

## Other files on this branch

| File | Change |
|---|---|
| `Content/StylizedNewYorkCity/Meshes/Buildings/Stairs/SM_Stair_Interior_01.uasset` | Collision complexity set to **Use Complex Collision As Simple** (`CTF_USE_COMPLEX_AS_SIMPLE`). The simple boxes skewed under the stair's non-uniform scale and left an invisible ledge sticking out past the top step. |
| `.mcp.json` | New. Points Claude Code at the Unreal MCP server (`http://127.0.0.1:8000/mcp`). Not used by the game. |
| `Config/DefaultEngine.ini` | **Unchanged.** Static-lighting settings were tried temporarily and fully reverted. |

No lightmaps or baked lighting data are committed. The baking experiment was reverted completely (see §6).

---

## 1. Bigger room (commit 5e47e53)

**Footprint:** the interior went from about X −558..500, Y −1050..−150 to **X −558..1100, Y −1050..150** (+600 east, +300 toward the street).

- **Shifted:** every non-floor actor east of X 470 moved **+600 X** (east wall, its shelves and decor, and everything on that side). Every mesh and light in front of Y −160 moved **+300 Y** (front facade, windows, door, front dressing).
- **Floor and ceiling:** deleted 8 misaligned outdoor floor tiles that ended up inside the room. Added 300×300 floor tiles at Z 0 and ceiling tiles at Z 600 along the new strip (Y 0, X −550…950) and the east wing (X 650/950 × Y −900/−600/−300). Folders `Expansion/Floor` and `Expansion/Ceiling`.
- **Walls** (`Expansion/Walls`):
  - East wall extended to Y 0, at Z 0 and Z 300.
  - Front wall: new shop-window piece at X 650 and window wall at X 950, plus the upper storey.
  - Back wall: X 650 and 950 at both storeys, with curtains.
  - West wall: extra segment at Y −150.
- **Mezzanine edge** (`Expansion/Mezzanine`): beams at X 500 and 800 (Z 290 and 580) plus a short beam at X 1025, three extra railing pieces, the railing end moved to X 995, and a column pair at X 500.
- **Volumes:**

  | Volume | Location | Scale |
  |---|---|---|
  | BlockingVolume3 | (1264, −420, 130) | (9.85, 1, 5.5) |
  | BlockingVolume2 | (−700, −420, 130), later moved to −925 in §4 | (9.85, 1, 5.5) |
  | BlockingVolume | (60, 566, 130) | (12.05, 1, 5.5) |
  | BlockingVolume4 | (60, −1380, 130) | (12.05, 1, 5.5) |
  | BlockingVolume5 / 6 | (220, −520, 800 / −400) | (11.25, 11.5, 1) |

  PostProcessVolume_Interior was resized at this stage; its final transform is in §4.

### Dressing for the new space
- **Lights** (`Expansion/Lights`):
  - 9 new ceiling lamps (`SM_CeilingLamp_17` copies) with spotlights at Z 500. The four in the east block cast shadows; the five-lamp front row at Y 0 doesn't.
  - Two pendant lamps with point lights under the mezzanine at X 700 and 950, Y −880. These were later swapped to the west side in §5.
  - Wall lamps with point lights on the east wall (Y −440 and −100) and on the west wall (Y −60).
  - Rect lights for the new windows, plus one spotlight at (950, 156, 267).
- **Table groups:** table, chairs and tabletop props, duplicated from existing tables (`Expansion/Props`):
  - East wing: four groups.
  - One group right of the door, with an extra chair.
  - A reading nook under the mezzanine.
  - Two groups on the mezzanine.
- **Window bars:** two copies of the west bar with stools, rotated to face the front windows, at X −250 and 945, Y 92.
- **Reading nook:** two bookshelves at X 600/730 on the back wall, stocked with copies of the existing shelf contents, plus boxes. These were later moved west in §5.
- **Plants, lamps and pictures:** extra plants, floor lamps and pictures on the east and back walls.

## 2. Wider stairs and deeper mezzanine (commit 5e47e53)

- **Stairs:** `SM_Floor_interior304` (mesh `SM_Stair_Interior_01`) is scaled **(1.96, 1, 1)** so each flight is twice as wide. It sits at (−400, −748.5, 0), yaw −90, with bounds X −551..−245 and Y −1048..−449. The landing is at Z 160.
- **Mezzanine:** every Z 300 floor tile in the row Y −960..−840 was duplicated one row forward (+300 Y), so the mezzanine now spans **Y −1050..−450**.
  - New edge beams run at Y −450, with an extra beam along X −250 at Y −600.
  - The edge columns moved to Y −440/−455, and the edge railings moved +300 Y.
  - Stairwell guard rails sit at X −245 (`SM_Fence_01_Middle19` moved, plus one new duplicate).
- **Ceiling lights:** the old ceiling lamp row at Y −440 moved to Y −300 so it doesn't clip the new edge beam.
  - New lamp row: 5 lamps with spotlights at Y −600.
  - New pendants: 4 lamps with point lights under the new strip at Y −600 (intensity 3, radius 450, no shadows).
- **Mezzanine strip dressing:** three more table groups at Y about −600, plus a plant. `SM_ModernLamp2` ended up at (1030, −760, 300).
- **West bar:** the bar, its stools and the props on it moved +320 Y to clear the wider lower flight. The pictures and sockets above it moved with it.
- **Plants:**

  | Plant | New location |
  |---|---|
  | `SM_FlowerPot_28` | (−130, −150, 0) |
  | `SM_FlowerPot_06` | (1045, −380, 0) |
  | `SM_FlowerPot_29` | (−490, −420, 0) |
  | `SM_FlowerPot_04` | (1045, −600, 300) |

- **Tour camera:** `Interior_16` moved to (−450, −30, 150).

## 3. Cleanup props capped at 100 (commit 5e47e53)

The earthquake in `ALTTMessDirector` throws **every** `BP_Prop_*` actor, even ones with `bIsCleanupTask = false`. To cut the count from 378 to 100, the extra props were **converted to plain StaticMeshActors**.

- Converted actors keep the same mesh, materials, transform and label. They're set to Static mobility and moved to a folder named `Decor/<original folder>`.
- The 100 props that stay are in folder **`Cleanup`**: 16 chairs, 8 stools and 76 small items, spread across both floors.
- Decor props that something else rests on (shelf stock, counter items, display cases) were never chosen, so nothing floats after the quake.

**Merge note:** if `main` added or edited `BP_Prop_*` actors in this map, check that the cleanup count is still what you want.

## 4. Stair landing and collision (commit 2ec3ffa)

- **Stair collision:** see the `.uasset` row in the table above.
- **Stair alcove:** the west wall beside the stairs (Y −1050..−450) moved **225 west, to X −783**, so the landing is now about 300 deep (`Expansion/StairAlcove`).
  - **Walls:**
    - `SM_Wall_300x780` / `SM_Wall_300x781` (Z 0 / 300) now sit at (−783, −600), with duplicates at (−783, −900).
    - `SM_Wall_300x917` / `SM_Wall_300x916` moved to (−558, −375), Z 0 / 300.
    - `SM_Wall_300x733` and `SM_Wall_300x734` were **deleted**.
    - New short return walls (scale X 0.75) at (−670.5, −450) and (−670.5, −1050), at both storeys.
  - **Columns:** `SM_Column_02_20x392` / `393` moved to (−775, −1050), with copies at (−558, −450).
  - **Landing slab:** two `SM_Floor_İnterior9` copies at X −662, Y −900 / −600, **Z 160** (scale X 0.753), so the slab sits flush with the stair landing. The ceiling above uses two tile copies at X −629, Z 600.5.
  - **Wall decor moved −225 X:** `SM_Shelf` and its items, `SM_ShopSign_02` (neon), `RectLight6` and tour camera `Interior_18`.
  - **Wall lamp:** `SM_WallLamp_01` is at (−773, −1000, 385) and `PointLight7` at (−751, −1000, 403), above head height.
- **Volumes:** BlockingVolume2 is at (−925, −420, 130). PostProcessVolume_Interior is at (175, −450, 390), scale (9.75, 6.5, 4.75).
- **Stair exit on the mezzanine:** `SM_CoffeeTable8`, `SM_Chair16`, `SM_Chair24` and `SM_Vase10` moved +60 X. `SM_Chair24` also moved to (−5, −1012, 300) at yaw 86.
- **Pictures:** `SM_Picture_16` is at (1095, −690, 200) and `SM_Picture_17` at (1095, −615, 215).

## 5. Kitchen moved to the east corner (commit 71cb6fd)

- **Kitchen block +600 X:** 136 actors, placed flush against the east wall. They include:
  - `SM_InteriorPlatform` (white platform, now X 450..1100)
  - back counters and drawers (`SM_Dolap*`, `SM_Counter_*`)
  - the front pastry counter (`SM_Counter_Middle5/6/7`) and display cases
  - `SM_Refrigerator3`
  - every item on the counters
  - `SM_MenuBoard_2`, `SM_Clock2`, `SM_Picture_8/9/10`, sockets
  - wall lamps `SM_Wall_Lamp_Exterior_3/4` with `PointLight9/10`, and `SpotLight`
  - the three pendants (`SM_CeilingLamp_02/7/9` with `PointLight3`, `PointLight` and `PointLight2`, now at X 690/790/890)
  - the bin and plant
  - cameras `Closeup_3/4/5` and `Interior_9`
- **Backsplash:** the `SM_CeramicWall2..7` tiles moved **+608 X** so they end at the wall. `SM_CeramicWall` (the leftmost tile) was **deleted**.
- **Reading nook moved west** (85 actors):
  - **Bookshelves:** `SM_Shelf_3` → X −76 and `SM_Shelf_4` → X 64, each with its stock. Together with `SM_Shelf_2` that makes three shelves in a row on the back wall.
  - **Table group 1:** `SM_CoffeeTable14` with `SM_Chair31`, `SM_Chair32` and its tabletop props, −600 X (table at 120, −660).
  - **Table group 2:** `SM_CoffeeTable18` with `SM_Chair38`, `SM_Chair17` and `SM_Vase18`, −650 X and +40 Y (table at 200, −860).
  - **Boxes:** `SM_Box_15/16` (−680 X, beside the platform) and `SM_Box_17` → (390, −780).
  - **Plant and picture:** `SM_FlowerPot_8` → (400, −585, 0) and `SM_Picture_15` → (330, −1042, 170).
  - **Pendants:** `SM_CeilingLamp_11/12` with `PointLight4/5` → X 100 and 350, Y −880.
- **Floor:** the old kitchen tiles `SM_Floor_İnterior7` and `SM_Floor_İnterior8` changed material slot 0 from `MI_Floor_01` to **`MI_WoodenFloor`**.

## 6. Lighting: net state

Baked lighting, a sunset directional light, a sky atmosphere, a Lightmass Importance Volume and reflection captures were all tried, then **removed**. What remains compared with `2b8baa9`:

- All lights are **Movable**. Lumen is unchanged, and `DefaultEngine.ini` is unchanged.
- **Shadows:** new lights from §1 and §2 follow the shadow settings listed there. The Y −600 lamp row and pendants don't cast shadows. About 34 lights in the level cast shadows.
- **Intensities:** intensities were scaled down for the bake and then divided back. Front spot and rect lights in front of Y 100 were also divided by 1.5 and set to white, so a few values may differ from the original by rounding.
- **SkyLight:** Movable, intensity 2.0, **Real Time Capture = on**. In PIE this prints the on-screen warning *"SkyLight with real-time capture enabled … requires a SkyAtmosphere…"*. If `2b8baa9` had Real Time Capture **off**, turn it back off, which also fixes the warning.
- **PostProcessVolume_Interior:** Auto Exposure Bias is 1.3. The min/max brightness overrides are off.
- **World Settings → Lightmass:** indirect bounces 3, quality 1.0, smoothness 1.0, volumetric lightmap detail cell size 200. These are the engine defaults and have no effect without static lighting.

## 7. Known nits

- The alcove landing slab uses the main floor's herringbone, which is at a different scale from the stair landing's planks.
- Two wall sockets near Y −361..−377 on the west wall overlap duplicates.
- A cleanup box (`SM_Box_19`) sits on the kitchen platform behind the pastry counter. That's intentional mess.
