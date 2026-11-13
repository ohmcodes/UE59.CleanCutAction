# L_NYC_Cafe — Changes in `4beed6a` "package polish UI"

Level: `/Game/StylizedNewYorkCity/Level/L_NYC_Cafe`
Compared against the previous commit, `2b8baa9` "draft playable".

## Summary

No geometry, lighting, or layout changed. **No actors were added, removed, or moved.**

The only edit in the map file is on **`LTTMessDirector`**. Its three UI widget slots now point to the new Widget Blueprints instead of the plain C++ widget classes.

## The change in detail

`LTTMessDirector` (at 68.7, −143.1, 41.4):

| Property | Before (C++ default) | After |
|---|---|---|
| `HUDClass` | `ULTTCleanupHUD` | `/Game/_CleanCutAction/Blueprints/Widgets/WBP_CleanupHUD` |
| `BriefingClass` | `ULTTBriefingScreen` | `/Game/_CleanCutAction/Blueprints/Widgets/WBP_Briefing` |
| `EndScreenClass` | `ULTTEndScreen` | `/Game/_CleanCutAction/Blueprints/Widgets/WBP_EndScreen` |

In play, the cleanup HUD, the pre-game briefing, and the end screen now use the styled Blueprint widgets. Because the level references these widgets, they get cooked into the Wasm build.

Unchanged: the other `LTTMessDirector` settings (including its 5 resolved label cameras), the world settings (game mode is still `BP_FirstPersonGameMode`), all 462 actors, transforms, meshes, materials, and lights.

## Changes elsewhere in this commit that affect the level

These assets aren't saved in the map, but they change how its actors behave:

- **Breakable props:** 15 prop Blueprints were updated, each with a new Geometry Collection in `/Game/_CleanCutAction/Destruction/`. **29 actors placed in this level** use them:
  - Table_02 ×7
  - Cupcake_03 ×3, Pie_Slice ×3
  - CoffeeCupSet ×2, Cupcake_04 ×2, ModernLamp ×2, PlateSet ×2
  - BirthdayCake, CakeDisplay, DishDrying, EspressoMachine, FlowerPot_04, FlowerPot_06, Microwave, Pie (×1 each)
- **New material `M_BrokenProp`**, added to `MapsToCook` in `Config/DefaultGame.ini` so it gets packaged.
- **`BP_MasterInteractables`** (the parent of every `BP_Prop_*`) was modified, which affects all 234 placed props.
- **C++ updates** to `LTTMessDirector`, `LTTInteractableBase`, `LTTInteractionComponent`, `LTTCleanupHUD`, `LTTEndScreen`, `LTTPauseMenu`, and the player controller. New classes: `LTTBriefingScreen`, `LTTUITheme`, `LTTWidgetBaker`.
- **Unrelated content added:** the BroadcastStudio and Hollywood_Vol3 asset packs, with their own maps. They aren't referenced by this level.

## Heads-up

Copies of `WBP_Briefing`, `WBP_CleanupHUD`, and `WBP_EndScreen` exist in both `/Game/_CleanCutAction/UI/` and `/Game/_CleanCutAction/Blueprints/Widgets/`. The level uses the **`Blueprints/Widgets`** copies. Edits to the `UI/` copies won't show up in game.

## Method

Both versions of the map were exported to T3D (full text dump of every actor and non-default property) and diffed. The whole difference is the three added lines above.
