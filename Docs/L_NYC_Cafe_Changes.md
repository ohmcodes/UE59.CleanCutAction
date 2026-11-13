# L_NYC_Cafe — Change Summary

Level: `/Game/StylizedNewYorkCity/Level/L_NYC_Cafe` (the only map cooked for the Wasm build).

This compares the level as first committed (`3cc9014 init`, the stock StylizedNewYorkCity cafe) with the current version (`4beed6a package polish UI`). The method is at the bottom.

## At a glance

| | Original (`init`) | Current |
|---|---|---|
| Actors | 470 | 462 |
| Interactable props (`BP_Prop_*`) | 0 | **234** (71 prop types) |
| Custom gameplay actors | none | `LTTMessDirector`, `LTTMusicPlayer` |
| Sky / sun / fog / clouds | yes | **removed** (interior-only lighting) |
| Outdoor sidewalk dressing | yes | **removed** |
| Game mode override | `BP_FirstPersonGameMode` | unchanged |

Unchanged: all 14 `Cameras/Interior` and 3 `Cameras/Exterior/AdditionalShots` actors, both PostProcessVolumes, the SkyLight, and the interior point, rect, and spot lights (apart from the two spotlights listed below).

---

## Timeline by commit

### 1. `a31f51d` "intereaction": props made interactable
- **244 actors changed from static meshes to interactable props.** 242 `StaticMeshActor`s plus `BP_CakeDisplay` and `BP_Microwave` became `BP_Prop_*` Blueprints with the same mesh and transform (74 prop classes at this point). These cover chairs, bar chairs, tables, cups, plates, cutlery, books, vases, pictures, boxes, coffee bags, syrups, mokapots, laptops, pastries, lamps, the espresso machine, and others.
- **12 new pastry props on the bakery display** (around X 295–356, Y −770…−790, Z 112–143): Cheesecake, Pie, Pie Slice, CookiePlate, 4× Croissant, 2× Donut_01, 2× Donut_02.
- **Removed:** `SM_BoardCafe` (the sidewalk A-frame sign) and `VolumetricCloud`.
- One `BlockingVolume` was moved.
- Some actor names and labels were reused while actors were swapped. Most of the "label changed" noise in a raw diff comes from this, not from real edits.
- A backup of the earlier state was saved as `L_NYC_Cafe_Backup.umap`.

### 2. `6ecb490` "placing and UI": cleanup game added
- **Added `LTTMessDirector`** at (68.7, −143.1, 41.4). It runs the Phase 1 loop: records each prop's home, shakes the props loose (the "earthquake"), and handles scoring, the countdown, the HUD, the briefing, and the end screen.
- `SM_Manhole_02` changed from a prop back to a plain `StaticMeshActor` (`SM_Manhole_3`), so it's no longer pickable.
- `SM_Bin6` was moved 27 units on X.

### 3. `a308c3e` "final draft": room fit and music
- **Added `LTTMusicPlayer`**, which plays a shuffled background-music loop with crossfades.
- **West wall pushed out 8 units** (X −550 → −558). Everything mounted on it moved with it: 8 wall pieces, 2 columns, 5 sockets, 2 wall lamps, 4 pictures, `SM_ShopSign_02`, and the upper shelf with its books, vases, and cookie jars (about 11 units).
- **Floor extended:** 7 new `SM_Floor_interior_300x300` tiles at the east and west edges, and 4 front-row floor tiles shifted about 139 units.
- 2 flower pots near the west entrance were nudged.
- `BlockingVolume3` was moved X 710 → 664, so the east boundary is tighter.

### 4. `2b8baa9` "draft playable": interior-only set
- **Removed the outdoor environment:** `DirectionalLight`, `SkyAtmosphere`, and `ExponentialHeightFog`.
- **Removed the sidewalk / front-of-house dressing**, i.e. everything in front of the front wall (Y > −150), 26 actors in all (21 of them props):
  - Coffee table set: table, 2 chairs, pillow, 4 coffee cups, mokapot, plate, croissant, book, cookie plate, camera, paper bag
  - Street clutter: 2 trash cans, 3 spray cans, flower pot
  - Signage and lights: `SM_ShopSign`, `SM_BoardCafe_3`, 2 exterior wall lamps (`SM_Wall_Lamp_4/5`), and the manhole
- **Front door:** `BP_Exterior_Door_02` (Blueprint door) was replaced by two static meshes, `SM_Wall_Exterior_Door_02` and `SM_Wall_Exterior_Door_02_Wall`.
- **PlayerStart moved inside the cafe:** Y 37 → −241 (now faces into the room, yaw −90).
- **Lighting:** `SpotLight9` was moved to (348, −143.7, 267.4), re-aimed, and raised from 1 → 10 intensity. A matching **`SpotLight10`** was added at (−260.2, −143.7, 267.4). Both sit along the front wall in `InteriorLights`.
- `LTTMusicPlayer` was nudged to X 51.7.
- A backup of the earlier state was saved as `L_NYC_Cafe_Backup2.umap`.

### 5. `4beed6a` "package polish UI": no visible change
- The `.umap` changed by about 650 bytes, but actors, transforms, meshes, materials, and light values match the previous commit exactly. This is probably a property tweak the diff doesn't capture, or a plain resave.

---

## Current prop inventory (234 `BP_Prop_*` actors)

| Count | Props |
|---|---|
| 18 | CoffeeBag |
| 14 | Chair_01 |
| 9 | Vase, Book_02 |
| 8 | PaperBag |
| 7 | Mokapot, Table_02 |
| 6 | Croissant, Box_02 |
| 5 | Plate, Pencil, MilkCup, Knife, Fork, CoffeeSyrup_01/02/03, CoffeeCup_02/03, Book_01 |
| 4 | Picture_01, Picture_03, Laptop, CookiePlate, CoffeeCup_01, Bar_Chai |
| 3 | Picture_02, Picture_04, CookieJamp_01/02, Box_01, Cupcake_03, Pie_Slice |
| 2 | Tray, FlowerPot_01, Donut_01/02, CuttingBoard_01/02, Cupcake_01/02/04, CoffeeCupSet, ModernLamp, PlateSet |
| 1 | WoodBoard_00, TipBox, Spoon, Soap, Pillow, PaymentDevice, Menu, Headphone, FlowerPot_04/05/06, FiltreCoffee, EspressoPortafilter, EspressoMachine, Envelope_02, Cupcake_Display, Clock, Cheesecake, Camera, Box_03, Bin, BirthdayCake, CakeDisplay, DishDrying, Microwave, Pie |

All of them except one are in the `InteriorCafe` outliner folder.

---

## How this was produced

Each committed version of the `.umap` was loaded in the editor, and every actor was exported to JSON (class, label, folder, transform, mesh, material overrides, and light intensity and colour). The versions were then diffed. Actors were matched by **mesh + location**, because the prop conversion reused actor names. Property changes outside that set (for example post-process settings or per-prop Blueprint variables) are not covered.

Raw exports: `Saved/LevelDiff/<commit>.json` and `current.json`.
