# Clean! Cut! Action! (Epic Mega AI Jam 2026)

First-person 3D casual game for a 3-day AI game jam. Engine: **UE 5.9.0 Preview** (`F:/UE5 Engines/UE_5.9`).
C++ for systems, Blueprint subclasses for content and tuning. Target: **Wasm (WebGPU) build uploaded to itch.io**.

The design docs live in `Docs/`. The overview below is imported in full; read the other files when a task touches their area.

@Docs/00_README.md

## Read these when working on...

| Area | File |
|---|---|
| Game design, story, characters | `Docs/CleanCutAction_Design.md` |
| What's done / what's next | `Docs/01_Master_Checklist.md` |
| Classes, save, level flow, input, interaction, carry, tweens, gameplay tags, director barks, pause | `Docs/02_Architecture_and_Systems.md` |
| Phase 1: messy set, Messifier tool, cleaning mechanics | `Docs/03_Phase1_Cleaning_Implementation.md` |
| Phase 2: cafe shoot, cue/beat director, effect stations, frame cone, take meter | `Docs/04_Phase2_Directing_Implementation.md` |
| Marlowe's lines, Phase 2 scene script | `Docs/05_Script_Director_and_Actors.md` |
| Menus, HUDs, end screens, tween specs | `Docs/06_UI_and_Tweening.md` |
| SFX and music | `Docs/07_Audio_List.md` |
| Niagara, materials, post-process | `Docs/08_VFX_and_Shaders.md` |

## Current project state (differs from the docs)

- Game content folder is `/Game/_CleanCutAction/` (with `Blueprints/`, `Core/`), not `Content/CleanCutAction/` as the docs say.
- `Source/` still holds the First Person template classes (`CleanCutAction*`, `Variant_Horror`, `Variant_Shooter`); the `LTT`-prefixed classes from the docs aren't written yet.
- The only map cooked for packaging is `/Game/StylizedNewYorkCity/Level/L_NYC_Cafe` (Config/DefaultGame.ini `MapsToCook`).
- Set art comes from the StylizedNewYorkCity pack (`/Game/StylizedNewYorkCity`).

## Web build / itch.io

- itch.io rejects any file over **200 MB**. Check `Packaged/Wasm/CleanCutAction/Content/Paks/pakchunk0-Wasm.ucas` after each package.
- List what's in a container: `UnrealPak.exe -ListContainer=<file>.utoc -csv=<out>.csv` (engine `Binaries/Win64`).
- Textures are capped at 1024 for Wasm only in `Platforms/Wasm/Config/WasmDeviceProfiles.ini`; StylizedNewYorkCity textures are 2K and were the bulk of the size.
- itch.io page needs **Embed options > SharedArrayBuffer support** turned on.
- Wasm platform docs: `F:/UE5 Engines/UE_5.9/Engine/Platforms/Wasm/Docs/` (Deploy.md, Performance.md, Troubleshooting.md).
