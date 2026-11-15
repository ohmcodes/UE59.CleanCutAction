# 02 — Web / Mobile Browser (WebAssembly + WebGPU)

**Goal:** CleanCutAction in desktop Chrome/Edge and Chrome on Android, no install, under itch.io's 200 MB per-file limit.

Branch: `wasm`. Checkout: `F:\UE5 Projects\CCA_WASM`. This is the build that ships on itch.io.

## Status (2026-11-15)

Latest package: **2026-11-15 01:14**, Shipping, from `wasm` at `d1471cd` plus two uncommitted changes (below). Archive: `F:\UE5 Projects\CCA_WASM\Packaged\`.

| File | Size |
|---|---|
| `Wasm/CleanCutAction/Content/Paks/pakchunk0-Wasm.ucas` | 146.5 MB (the largest file; itch.io limit 200 MB) |
| `Wasm/CleanCutAction/Binaries/Wasm/CleanCutAction-Wasm-Shipping.wasm` | 141.2 MB (25.9 MB as `.wasm.br`) |
| `pakchunk0-Wasm.pak` / `pakchunk99-Wasm.pak` | 6.6 MB / 2.2 MB |
| Whole `Wasm/` folder | 330 MB (raw + `.br` copies) |
| `CCA_2ms_WASM_v1.0.0.zip` (upload) | 238.7 MB |

**Uncommitted in `CCA_WASM` (commit them, or the next build from a clean checkout is bigger):**

1. `Platforms/Wasm/Config/WasmDeviceProfiles.ini`: World, WorldNormalMap and WorldSpecular capped at **512** (character groups stay 1024). The committed file says 1024; at 1024 the build was 246 MiB, over itch.io's limit.
2. `CleanCutAction.uproject`: `"TargetPlatforms": [ "Wasm" ]`. Wasm branch only; don't merge it elsewhere.

---

## 1. Facts about this target (UE 5.9 Preview)

| Question | Answer |
|---|---|
| Platform folder | `Engine/Platforms/Wasm` |
| Project platform config folder | `Platforms/Wasm/Config/` (`WasmEngine.ini`, `WasmDeviceProfiles.ini`) |
| UAT `-platform=` | `Wasm` |
| Device profiles | `Wasm` (texture caps), `Wasm_Mobile` (phones/tablets) |
| Shader platforms in the build | `ES3_1`, `ES3_1_DEFERRED` (`wasm-tiers.json`: deferred, no SM5) |
| Target settings section | `[/Script/WasmTargetPlatform.WasmTargetSettings]` in `DefaultEngine.ini` |

- Web is experimental: WebAssembly (64-bit, 1 GB initial / 4 GB max heap) + WebGPU, mobile renderer. Chrome/Edge give the best result; Firefox runs the forward tier.
- The engine here is an installed build and packages Wasm fine; a source build isn't needed.
- The engine HTTP module is a null stub on Wasm, so the leaderboard uses the browser's `fetch()` (`Source/CleanCutAction/UI/LTTLeaderboardSubsystem.*`). Wasm-only code; keep it behind the platform check.

---

## 2. Target settings

`DefaultEngine.ini` (shared, only read by the Wasm platform):

```ini
[/Script/WasmTargetPlatform.WasmTargetSettings]
PageTitle=Clean! Cut! Action!
StartMode=Click            ; wasm branch only: waits for a click, which also starts sound
MobileDevices=Allow
MinDeviceMemoryGB=4
bTargetMobileDeferredTier=True
FaviconFile=(FilePath="game logo icon.ico")
SplashImage=(FilePath="game cover art.png")
CoverBackgroundColor=EBF8FE
```

`StartMode=Click` exists only on the `wasm` branch. Don't copy it to the others.

---

## 3. Renderer — done

`Platforms/Wasm/Config/WasmEngine.ini` loads last for Wasm and puts back the web settings that `DefaultEngine.ini` would override:

```ini
[/Script/Engine.RendererSettings]
r.Mobile.ShadingPath=0
r.Mobile.AntiAliasing=1          ; DefaultEngine.ini has 3 (MSAA), which the web deferred tier can't do
r.MobileHDR=True
r.Mobile.SupportGPUScene=True
r.Shadow.CSM.MaxMobileCascades=2
r.AllowOcclusionQueries=False    ; browser query results arrive 4-6 frames late, props blinked out when turning
```

- Mobile deferred tier (`bTargetMobileDeferredTier=True`), not the desktop tier.
- No Lumen, no hardware RT, no Nanite (project-wide Nanite is off anyway).

---

## 4. Plugins — done

- `AllToolsets` off (`.uproject`). The Wasm docs say its game feature plugins fail the cook.
- `NNEDenoiser` off (54 MB of path-tracer models).
- No DLSS/Streamline on this branch. `Plugins/` must not come in from `windows`.

---

## 5. Size budget — done

- `MapsToCook`: the two cafe maps + `ParisStreet/Levels/SampleMap` + path-loaded assets (README).
- Textures, `Platforms/Wasm/Config/WasmDeviceProfiles.ini`:

```ini
[Wasm DeviceProfile]
+TextureLODGroups=(Group=TEXTUREGROUP_World,MinLODSize=1,MaxLODSize=512,...)            ; 1024 in git, 512 in the shipped build
+TextureLODGroups=(Group=TEXTUREGROUP_WorldNormalMap,MinLODSize=1,MaxLODSize=512,...)
+TextureLODGroups=(Group=TEXTUREGROUP_WorldSpecular,MinLODSize=1,MaxLODSize=512,...)
+TextureLODGroups=(Group=TEXTUREGROUP_Character,MinLODSize=1,MaxLODSize=1024,...)
+TextureLODGroups=(Group=TEXTUREGROUP_CharacterNormalMap,MinLODSize=1,MaxLODSize=1024,...)
+TextureLODGroups=(Group=TEXTUREGROUP_CharacterSpecular,MinLODSize=1,MaxLODSize=1024,...)

[Wasm_Mobile DeviceProfile]
+CVars=r.ScreenPercentage=75
+CVars=r.Shadow.CSM.MaxMobileCascades=1
+CVars=sg.ShadowQuality=1
+CVars=sg.PostProcessQuality=1
+CVars=sg.EffectsQuality=1
```

- Audio: music is the largest (about 14 min in `MusicFX/` plus 4.8 min `EmoteMusic/paroparo`, quality 80). If size gets tight, lower the cook quality for Wasm rather than re-encoding the source.
- After each package, check `pakchunk0-Wasm.ucas` is under 200 MB. List what's inside: `UnrealPak.exe -ListContainer=<file>.utoc -csv=<out>.csv` (engine `Binaries/Win64`).

---

## 6. Input — done

- **Keyboard/mouse** on desktop. Pointer lock comes from the click that starts the game (`StartMode=Click`).
- **Gamepad** through the browser's Gamepad API: same layout as desktop (`04_Linux_SteamDeck.md` section 5). Prompts and menus switch to the gamepad as soon as it's used; menus get gamepad focus.
- **Touch** on phones/tablets: on-screen sticks and buttons, shown only when the touch interface is detected, and hidden while a gamepad is driving a touch device (tablet + controller, Steam Deck browser). A virtual keyboard (`WBP_VirtualKeyboard`) handles name entry.

---

## 7. Build

```
Docs\PackagingGuide\Build_Platform.bat Web
```

Equivalent to the engine docs' command:

```
RunUAT.bat BuildCookRun -project=<P>.uproject -platform=Wasm -clientconfig=Shipping -build -cook -stage -pak -iostore -package -archive -archivedirectory=<checkout>\Packaged -unattended -utf8output
```

Output: `Packaged\Wasm\`. Local test: `Packaged\Play_CleanCutAction.bat` (or `play-cleancutaction.sh`) serves it with `serve.py` on port 8922 with the right headers. Double-clicking `index.html` won't work.

---

## 8. Hosting

- HTTPS (or `http://localhost`), `.wasm` as `application/wasm`, and `Cross-Origin-Opener-Policy: same-origin` + `Cross-Origin-Embedder-Policy: require-corp` (the game runs on wasm threads, which need `SharedArrayBuffer`). The build ships a `_headers` file for Cloudflare Pages-style hosts.
- **itch.io:** HTML project, upload the zipped `Wasm` folder, tick "This file will be played in the browser", and turn on **Edit game > Embed options > SharedArrayBuffer support**. Check it stayed on after saving. `_headers` and the service worker don't help inside itch.io's iframe.
- No single file over 200 MB.

Test order: desktop Chrome → Chrome on Android → others (best effort).

---

## 9. Checklist

- [x] Platform name, config folder, UAT name and device profiles known (section 1)
- [x] Mobile deferred tier
- [x] No DLSS/Streamline files in the package
- [x] `AllToolsets` and `NNEDenoiser` off
- [x] Web device profile caps textures; source assets untouched
- [ ] 512 world-texture cap and `TargetPlatforms` committed on `wasm`
- [x] Size under 200 MB per file (largest 146.5 MB)
- [ ] Runs in desktop Chrome (VERIFY on the itch.io page)
- [ ] Runs in Chrome on Android with touch controls (VERIFY)
- [ ] Gamepad tested in the browser
- [ ] Leaderboard submits from the itch.io page (`fetch()` to `cca.ohmcodes.com`)
- [ ] itch.io SharedArrayBuffer option confirmed on
- [ ] Common verification in README done
