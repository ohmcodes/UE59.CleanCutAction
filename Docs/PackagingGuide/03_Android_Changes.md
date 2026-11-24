# 03b — Android: configuration and optimization log

What was actually set up and changed for the Android build (branch `android`, 2026-11-15). The plan and build steps are in `03_Android.md`; this file records what was done and why.

Nothing here has been tested on a device yet.

---

## 1. App icons and launch images

All made from `game cover art.png` (720×370) and written to `Build/Android/res/`, which is where the Android Project Settings fields read from.

| Project Settings field | File | Size | Content |
|---|---|---|---|
| Icon | `drawable/icon.png` | 48×48 | Square crop of the Rookie (mop, headset, clapperboard) |
| LDPI Icon | `drawable-ldpi/icon.png` | 36×36 | Same crop |
| MDPI Icon | `drawable-mdpi/icon.png` | 48×48 | Same crop |
| HDPI Icon | `drawable-hdpi/icon.png` | 72×72 | Same crop |
| XHDPI Icon | `drawable-xhdpi/icon.png` | 96×96 | Same crop |
| Download Background Horizontal | `drawable/downloadimageh.png` | 1280×720 | Full cover art, filling the frame |
| Download Background Vertical | `drawable/downloadimagev.png` | 720×1280 | Full cover art centred on a blurred, darkened copy of itself |
| Launch Landscape | `drawable/splashscreen_landscape.png` | 1280×720 | Same as the horizontal download image |
| Launch Portrait | `drawable/splashscreen_portrait.png` | 720×1280 | Same as the vertical download image |

- The title text is unreadable at icon size, so the icons show the Rookie only (crop x=232, y=118, 252×252 of the cover art).
- The launch images are 1280×720 instead of the default 640×360. The source art is only 720×370, so they are upscaled and slightly soft. Replace them if higher-resolution art becomes available.
- `Build/` appears to be git-ignored. Force-add these files (`git add -f Build/Android/res`) if they should be in the repo.

---

## 2. Android runtime settings

`Config/DefaultEngine.ini`, section `[/Script/AndroidRuntimeSettings.AndroidRuntimeSettings]`:

| Setting | Value | Why |
|---|---|---|
| `PackageName` | `com.ohmcodes.cleancutaction` | The old value `2ms_cleancutaction` fails UBT's check: a package name needs at least two dot-separated parts, each starting with a letter. **This becomes permanent once published on Google Play.** |
| `ApplicationDisplayName` | `Clean! Cut! Action!` | Name under the home-screen icon (was `clean_cut_action`) |
| `StoreVersion` / `VersionDisplayName` | `1` / `1.0` | |
| `MinSDKVersion` / `TargetSDKVersion` | `26` / `36` | Engine defaults (Android 8 minimum) |
| `bPackageDataInsideApk` | `True` | One self-contained APK for sideloading, no OBB |
| `Orientation` | `SensorLandscape` | Landscape, flips when the phone is turned either way |
| `bFullScreen` | `True` | Immersive fullscreen |
| `bShowLaunchImage` | `True` | Was `False`, so the launch images never showed |
| `bBuildForArm64` / `bBuildForX8664` | `True` / `False` | 64-bit ARM phones only |
| `bSupportsVulkan` / `bBuildForES31` | `True` / `True` | Vulkan where the device profile allows it, OpenGL ES fallback |
| `bDetectVulkanByDefault` | `True` | |
| `bStripShaderReflection` | `True` | Smaller shaders |
| `bEnableAdvancedBinaryCompression` / `bCompressDebugSymbols` | `True` / `True` | Smaller native libraries |
| `bMultiTargetFormat_ASTC` | `True` | Only ASTC textures are cooked |
| `bMultiTargetFormat_ETC2` / `bMultiTargetFormat_DXT` | `False` / `False` | Avoids cooking extra texture copies into the APK |

`CleanCutAction.uproject` also has `"TargetPlatforms": ["Android"]` on this branch.

---

## 3. Android device profiles

New file `Config/Android/AndroidDeviceProfiles.ini`. It only affects Android builds. Every `Android_*` tier inherits from `[Android DeviceProfile]`, and phones the engine doesn't recognise fall back to `Android_Default`, which uses Mid.

### All phones (`[Android DeviceProfile]`)

| Setting | Value | Why |
|---|---|---|
| `TextureLODGroups` World, WorldNormalMap, WorldSpecular, Character, CharacterNormalMap, CharacterSpecular | `MaxLODSize=1024` | Mips above 1024 are stripped at cook time. StylizedNewYorkCity textures are 2K (~75% smaller) and the ParisStreet textures are 4K (~94% smaller). Source assets and desktop builds keep full resolution. Same cap as the Wasm build. |
| `t.MaxFPS` | `60` | Stops 90/120 Hz phones from rendering at their full refresh rate, overheating and slowing down |
| `r.SkyLight.RealTimeReflectionCapture` | `0` | Both cafes have a sky light with Real Time Capture on. The sky never changes, so phones capture it once at load. |
| `r.Shadow.DistanceScale` | `0.1` | The sun's dynamic shadow distance is 40,000 cm (400 m, 4 cascades) in both cafes. 0.1 gives 40 m, which covers the cafe and the street outside the windows. |

### Low (`[Android_Low DeviceProfile]`)

| Setting | Value | Why |
|---|---|---|
| `t.MaxFPS` | `30` | Steady frame pacing on weak phones |
| `r.MSAACount` | `1` | No anti-aliasing |
| `r.MobileContentScaleFactor` | `0.7` | Smaller render resolution |
| `r.StaticMeshLODDistanceScale` | `2` | Switches to lower-detail models at half the usual distance |

### Mid (`[Android_Mid DeviceProfile]`, also unrecognised phones)

| Setting | Value | Why |
|---|---|---|
| `r.Shadow.CSM.MaxMobileCascades` | `1` | One sun shadow cascade instead of two |
| `r.MSAACount` | `2` | 2x MSAA instead of 4x |
| `r.StaticMeshLODDistanceScale` | `1.5` | Lower-detail models at two-thirds of the usual distance |

### High / Epic

Engine defaults, plus the "All phones" settings above.

---

## 4. Code change: mirror

`Source/CleanCutAction/World/LTTMirror.cpp`, `ALTTMirror::BeginPlay`, inside `#if PLATFORM_ANDROID`:

| Property | Android limit |
|---|---|
| `ResolutionScale` | at most `0.35` of the viewport |
| `MaxResolution` | at most `512` px |
| `bReflectShadows` | `false` |
| `UpdateDistance` | at most `1200` cm (12 m) |
| `MaxReflectionDistance` | at most `1500` cm (15 m) |

The mirror re-renders the scene every frame it is on screen. Desktop and Wasm builds are unchanged. The Android C++ needs a rebuild to pick this up.

---

## 5. Map audit and changes

All three maps were inspected through the editor.

### `L_NYC_Cafe_Michael` (Phase 1, `GameDefaultMap`)

| Item | Found | Action |
|---|---|---|
| Unused `SceneCapture2D` actors (`SceneCapture2D` to `SceneCapture2D4`) | 4, each rendering the scene to its own 1280×720 render target. No level mesh, C++ or Blueprint reads them. | **Deleted**, level saved |
| `LTTMirror` | 1, a full second scene render while it's visible | Android limits (section 4) |
| Lights | About 22 visible movable lights. Only the sun casts shadows. | None needed |
| Sky light | Real Time Capture on | Turned off on Android through the device profile |
| Sun | 400 m shadow distance, 4 cascades | Cut to 40 m on Android through the device profile |
| Meshes | 668 mesh components, 124 unique meshes, ~4.4M triangles at LOD0. Every prop has 3 LODs that kick in early, so actual on-screen cost is roughly a quarter of that. Heaviest: `SM_Table-02` (41k tris × 18), `SM_Chair_01` (8.9k × 36), `SM_CeilingLamp_01` (32k × 10), `SM_PlateSet` (55k × 2). | LOD distance scale on Low/Mid |
| Post-process | Interior volume: bloom 0.675, lens flare, vignette, histogram auto-exposure | Left as is (minor cost) |

### `L_NYC_Cafe_Phase2_Michael` (Phase 2)

| Item | Found | Action |
|---|---|---|
| Unused `SceneCapture2D` actors | Same 4 as Phase 1 | **Deleted**, level saved |
| Lights | 16 visible spot lights, 6 point lights, 1 rect light. Only the sun casts shadows. | None needed |
| Sky light and sun | Same as Phase 1 | Covered by the device profile |
| Meshes | 678 mesh components | LOD distance scale on Low/Mid |
| Niagara | `NS_Rain`, `NS_Wind`, `NS_Steam` (one CPU emitter each), `NS_SinkBubbles`, `NS_Sparks` | None needed. `ALTTEffectStation::BeginPlay` switches the station effects off and they only play on cue. |

### `/Game/ParisStreet/Levels/SampleMap` (level instance in both cafes)

| Item | Found | Action |
|---|---|---|
| Meshes | 184 static mesh actors, 48 unique meshes, ~84k triangles total, 2–4 LODs each | None needed |
| Lights / scene captures | None (the cafes supply the lighting) | None needed |
| Niagara | `S_Smoke`, one CPU sprite emitter, always running | Left as is (cheap) |
| Textures | 143 textures, many 4096×4096, all in `TEXTUREGROUP_World` / `TEXTUREGROUP_WorldNormalMap` | Cooked at 1024 by the Android texture cap |

The map itself was not modified.

---

## 6. Already fine, left alone

- **Touch controls:** `DefaultTouchInterface=None`, so the engine's virtual joystick won't double up with `UI_TouchSimple`. `ACleanCutActionPlayerController::ShouldUseTouchControls` uses `SVirtualJoystick::ShouldDisplayTouchInterface()`, which is true on Android.
- **Renderer:** the project already targets Mobile hardware: mobile forward shading, 4x MSAA, Nanite and ray tracing off. The engine's `BaseAndroidEngine.ini` and `AndroidScalability.ini` handle occlusion feedback, distance fields and the per-tier shadow and post-process quality.

---

## 7. Possible next steps (not done)

- If draw calls are the bottleneck on a test phone, turn off shadow casting on small Phase 1 props (cups, forks, syrup bottles). About 660 of the ~670 mesh components cast shadows. This would also change the desktop look, so it was left for now.
- Replace the launch images with art at 1280×720 or larger.
- Check the APK size after the first package and look for any texture in an uncapped LOD group.

---

## 8. Before packaging

1. Rebuild the C++ for Android (`LTTMirror.cpp` changed).
2. Disable **All Toolsets** in `CleanCutAction.uproject` (its game feature plugins fail the cook).
3. Package Shipping for Android (ASTC) and install the APK on a test phone.
4. Commit to the `android` branch: both cafe `.umap` files, `LTTMirror.cpp`, `Config/DefaultEngine.ini`, `Config/Android/`, `CleanCutAction.uproject`, this doc, and `Build/Android/res` (force-add).
