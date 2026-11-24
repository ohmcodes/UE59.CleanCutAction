# Clean! Cut! Action!

![Clean! Cut! Action!](game%20logo.png)

Our entry for the **Meta VR Start Developer Competition 2026** (Gaming track): a first-person VR casual game for Meta Quest, built in Unreal Engine 5.9.

> **This is the `vr` branch: the Meta Quest VR version.** It runs on PC VR through OpenXR (Link, Air Link, Meta XR Simulator) and targets a Quest APK. Everything VR-specific is documented in [Docs/10_VR_Meta_Quest.md](Docs/10_VR_Meta_Quest.md). The web (itch.io), desktop and Android phone builds live on the other branches.

You play the Rookie, a production assistant on the last day of shooting a rom-com. The game has two phases:

1. **Clean the set.** Last night's wrap party trashed the two-floor cafe set. Put everything back where it belongs while director Marlowe Finch heckles you through a megaphone.
2. **Run the shoot.** Actors Dash and Juniper (who are secretly exes) shoot their first-date scene. On Marlowe's cues you run between effect stations on both floors (rain, traffic, thunder, steam, prop tosses, boom mic) and stay out of the camera's frame.

## Requirements

| | |
|---|---|
| Engine | Unreal Engine **5.9.0 Preview**, built from source or installed locally |
| IDE | Visual Studio 2022 (or Rider) with the *Game development with C++* workload |
| Platform | Windows 10/11 for development |
| VR runtime | An OpenXR runtime: **Meta Horizon Link** (headset over Link / Air Link) or **Meta XR Simulator** (no headset) |
| Headset target | Meta Quest 3 / 3S (Quest 2 compatible) |
| Web target | The UE Wasm platform (WebGPU) for the itch.io build |
| Python | 3.x, only needed to test the web build locally |

A project saved in a preview engine can't be opened in an older engine version.

## Getting started

1. Clone the repo:
   ```bash
   git clone https://github.com/ohmcodes/UE59.CleanCutAction.git
   ```
2. Right-click `CleanCutAction.uproject` and choose **Switch Unreal Engine version...**, then pick your UE 5.9 install. Don't commit the `EngineAssociation` change it makes.
3. Open `CleanCutAction.sln` and build the `CleanCutActionEditor` target in `Development Editor | Win64`. You can also open the `.uproject` and let the editor build the module for you.
4. The editor opens on the cafe level in `/Game/StylizedNewYorkCity/Level/`.
5. **VR:** make Meta Horizon Link or the Meta XR Simulator the active OpenXR runtime, then use **Play > VR Preview**. Controls and simulator setup: [Docs/10_VR_Meta_Quest.md](Docs/10_VR_Meta_Quest.md).

## Project layout

```
Source/CleanCutAction/     C++ game module
  Interaction/             interaction, carry, mess director, cleanup HUD, end screen
  UI/                      briefing screen, pause menu, UI theme, widget baker
  Audio/                   music player
  Shoot/                   Phase 2 scene director, effect stations
  VR/                      VR mode: HMD camera, controllers, snap turn, world-space UI panel
Content/_CleanCutAction/   game content: Blueprints, input, materials, UI, break FX, music
Content/StylizedNewYorkCity/  set art and the cafe level (L_NYC_Cafe)
Config/                    engine, game, input, and editor settings
Platforms/Wasm/            Wasm-only config, such as the texture size cap
Scripts/serve_wasm.py      local web server for testing the Wasm build
Scripts/Python/create_vr_input.py  creates the VR input assets (IMC_VR, IA_VR_*)
Docs/                      design and implementation docs
```

**Code conventions.** Systems are written in C++ and tuned or filled with content through Blueprint subclasses. C++ classes use the `LTT` prefix (for example `ULTTCarryComponent`). Asset prefixes (`BP_`, `WBP_`, `M_`, `MI_`, `NS_`, `IA_`, `L_`, ...) are listed in [Docs/00_README.md](Docs/00_README.md).

## Web build (itch.io)

1. Package the project for the **Wasm** platform from the editor (**Platforms > Wasm > Package Project**).
2. Check the size of the largest pak file. itch.io rejects any single file over **200 MB**:
   `Packaged/Wasm/CleanCutAction/Content/Paks/pakchunk0-Wasm.ucas`
   To see what's taking up space, list the container with `UnrealPak.exe` from the engine's `Binaries/Win64` folder:
   ```bash
   UnrealPak.exe -ListContainer=pakchunk0-Wasm.utoc -csv=pak_contents.csv
   ```
3. Test it locally. The server sends the cross-origin isolation headers that SharedArrayBuffer needs, and serves the Brotli-compressed files:
   ```bash
   python Scripts/serve_wasm.py <path-to-packaged-build> 8080
   ```
   Then open http://localhost:8080/ in a browser that supports WebGPU.
4. Zip the build and upload it to itch.io as an HTML game. Under **Embed options**, turn on **SharedArrayBuffer support**.

Only the maps and assets listed under `MapsToCook` in `Config/DefaultGame.ini` get cooked. Add any new level there. Textures are capped at 1024 px for Wasm only, in `Platforms/Wasm/Config/WasmDeviceProfiles.ini`, to keep the download small.

The engine's Wasm docs (`Deploy.md`, `Performance.md`, `Troubleshooting.md`) are in `Engine/Platforms/Wasm/Docs/` in the engine install.

## Documentation

| File | Contents |
|---|---|
| [CleanCutAction_Design.md](Docs/CleanCutAction_Design.md) | Game design, story, and characters |
| [00_README.md](Docs/00_README.md) | Overview, technical decisions, naming conventions |
| [01_Master_Checklist.md](Docs/01_Master_Checklist.md) | Task checklist from setup to submission |
| [02_Architecture_and_Systems.md](Docs/02_Architecture_and_Systems.md) | Classes, save, level flow, input, interaction, carry, tweens |
| [03_Phase1_Cleaning_Implementation.md](Docs/03_Phase1_Cleaning_Implementation.md) | Messy set and cleaning mechanics |
| [04_Phase2_Directing_Implementation.md](Docs/04_Phase2_Directing_Implementation.md) | Cue director, effect stations, frame cone, take meter |
| [05_Script_Director_and_Actors.md](Docs/05_Script_Director_and_Actors.md) | Marlowe's lines and the Phase 2 scene script |
| [06_UI_and_Tweening.md](Docs/06_UI_and_Tweening.md) | Menus, HUDs, end screens, tween specs |
| [07_Audio_List.md](Docs/07_Audio_List.md) | SFX and music |
| [08_VFX_and_Shaders.md](Docs/08_VFX_and_Shaders.md) | Niagara, materials, post-process |

The docs were written as a plan before development started, so some of them no longer match the project. For example, game content lives in `/Game/_CleanCutAction/`, not `Content/CleanCutAction/`. [CLAUDE.md](CLAUDE.md) lists the known differences.

## Credits

- Set art: *Stylized New York City* asset pack
- Built with Unreal Engine 5.9 and AI coding tools for the Meta VR Start Developer Competition 2026

## Copyright and license

Copyright © 2026 Ohmcodes. All Rights Reserved. See [COPYRIGHT.md](COPYRIGHT.md) for the full notice.

This is proprietary software. No license to copy, modify or redistribute it is granted (see [LICENSE](LICENSE)). Third-party Marketplace/Fab assets in `Content/` are covered by their own licenses and can't be redistributed separately.
