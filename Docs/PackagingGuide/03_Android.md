# 03 — Android (Sideloadable APK)

**Goal:** one `.apk` that sideloads on a modern arm64 Android phone, with the mobile renderer and touch controls.

Branch: `android`. Checkout: `F:\UE5 Projects\CCA_Android`.

## Status (2026-11-15)

- Branch has the shared fixes and gamepad support, and **no Android-specific config**: no `[/Script/AndroidRuntimeSettings.AndroidRuntimeSettings]` section, no `Config/Android/`, no Android device profile. Its `Config/` and `.uproject` match `windows` (without `Config/Windows/`).
- Not packaged yet. SDK/NDK install not confirmed (`ANDROID_HOME` / `NDKROOT` aren't set in the agent's shell; VERIFY on the build machine).
- Already in the project and useful here: `TargetedHardwareClass=Mobile`, `DefaultGraphicsPerformance=Scalable`, touch controls, a virtual keyboard for name entry, gamepad support.
- Phones already run the game in the browser (`02_Web_WASM.md`, `Wasm_Mobile` profile). The APK is only needed for a native build.

---

## 1. Prerequisites (user installs; agent verifies)

- [ ] Android Studio installed.
- [ ] Run `F:\UE5 Engines\UE_5.9\Engine\Extras\Android\SetupAndroid.bat` to install the SDK/NDK this engine expects. UE 5.9 asks for **NDK r27c** (`Engine/Config/Android/Android_SDK.json`).
- [ ] Restart the editor so it picks up `ANDROID_HOME` / `NDKROOT`.
- [ ] Project Settings → Platforms → Android SDK: paths filled in (or blank to use environment variables).
- [ ] A test phone with Developer Options and USB debugging on.

---

## 2. Android runtime settings — not done

Project Settings → Platforms → Android (writes to `Config/DefaultEngine.ini`; the section is Android-only). Do it on the `android` branch:

```ini
[/Script/AndroidRuntimeSettings.AndroidRuntimeSettings]
PackageName=com.VERIFY_team.cleancutaction
StoreVersion=1
ApplicationDisplayName=Clean! Cut! Action!
bPackageDataInsideApk=True
bBuildForArm64=True
bBuildForX8664=False
bSupportsVulkan=True
bBuildForES31=True
bFullScreen=True
Orientation=SensorLandscape
```

- The user clicks **"Accept SDK License"** in the editor.
- **VERIFY** the package name with the user (company: QLE OPC).
- Leave **"For Distribution"** off. The debug key is fine for sideloading; a release keystore is only for the Play Store (ask first).
- `DefaultEngine.ini` already has an `AndroidFileServerRuntimeSettings` section (editor file server, not shipped). Leave it.

---

## 3. Renderer — not done

`Config/Android/AndroidEngine.ini`:

```ini
[/Script/Engine.RendererSettings]
r.Mobile.AntiAliasing=1   ; DefaultEngine.ini has 3 (MSAA x4); FXAA/TAA is cheaper on phones. VERIFY on device.
```

- Mobile renderer: no Lumen, no hardware RT, no Nanite (Nanite is off project-wide). Desktop-only settings in `DefaultEngine.ini` (Lumen, VSM) don't apply to the mobile renderer.
- The web build already runs the mobile deferred tier (`r.Mobile.ShadingPath=0` is the project default), so lighting should read similarly.

---

## 4. Device profile and textures — not done

Cook flavor: **ASTC** (phones lack the BC formats a default cook uses). ETC2 is the fallback for old GPUs.

`Config/Android/AndroidDeviceProfiles.ini`:

```ini
[Android DeviceProfile]
+TextureLODGroups=(Group=TEXTUREGROUP_World,MinLODSize=1,MaxLODSize=1024,LODBias=0,MinMagFilter=aniso,MipFilter=point,MipGenSettings=TMGS_SimpleAverage)
+TextureLODGroups=(Group=TEXTUREGROUP_WorldNormalMap,MinLODSize=1,MaxLODSize=1024,LODBias=0,MinMagFilter=aniso,MipFilter=point,MipGenSettings=TMGS_SimpleAverage)
+TextureLODGroups=(Group=TEXTUREGROUP_Character,MinLODSize=1,MaxLODSize=1024,LODBias=0,MinMagFilter=aniso,MipFilter=point,MipGenSettings=TMGS_SimpleAverage)
+TextureLODGroups=(Group=TEXTUREGROUP_CharacterNormalMap,MinLODSize=1,MaxLODSize=1024,LODBias=0,MinMagFilter=aniso,MipFilter=point,MipGenSettings=TMGS_SimpleAverage)
+CVars=r.ScreenPercentage=75
+CVars=sg.ShadowQuality=1
+CVars=sg.PostProcessQuality=1
+CVars=sg.EffectsQuality=1
```

- **VERIFY:** the engine defines `Android_Low/Mid/High/...` profiles that inherit from `Android`. Put overrides in the base `Android` profile or the relevant tier, don't replace them.
- Source assets stay untouched (README → Texture policy).
- Music is the largest audio. Lower the Android cook quality if the APK is too big, rather than re-encoding.

---

## 5. Plugins and code

- No DLSS/Streamline. `AllToolsets` and `NNEDenoiser` off.
- The leaderboard uses `FHttpModule` on Android; the APK needs the INTERNET permission (VERIFY it's in the generated manifest).
- Android-only C++ goes in `#if PLATFORM_ANDROID`.

---

## 6. Input — mostly done (shared code)

- **Touch:** the template's on-screen controls (`MobileControlsWidget`) spawn when the touch interface is detected, with a music toggle next to the touch pause button. VERIFY they're thumb-sized and clear of the notch on the test phone.
- **Gamepad:** a Bluetooth controller works with the same layout as desktop (`04_Linux_SteamDeck.md` section 5); the touch overlay hides while the gamepad is in use.
- **Back button:** VERIFY it opens the pause menu instead of quitting. The pause menu closes on B (`Gamepad_FaceButton_Right`); Android's back key isn't handled explicitly in code.

---

## 7. Build and install

```
Docs\PackagingGuide\Build_Platform.bat Android
```

Output: `Packaged\Android_ASTC\` (VERIFY the folder and APK name after the first build).

```
adb install -r "Packaged\Android_ASTC\CleanCutAction-arm64.apk"
```

---

## 8. Checklist

- [ ] SDK/NDK (r27c) installed via SetupAndroid.bat; SDK license accepted by the user
- [ ] Package name confirmed with the user
- [ ] Android runtime settings: single APK, data inside, arm64 only
- [ ] Vulkan + ES3.1; mobile renderer
- [ ] ASTC cook; Android device profile caps textures; source assets untouched
- [ ] No DLSS/Streamline files in the package
- [ ] Installs via `adb install` and launches to gameplay
- [ ] Touch controls usable; back button opens pause
- [ ] Leaderboard works (INTERNET permission)
- [ ] Acceptable frame rate on the test phone (record device + FPS)
- [ ] Common verification in README done
