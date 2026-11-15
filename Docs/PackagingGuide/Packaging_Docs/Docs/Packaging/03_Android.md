# 03 — Android (Sideloadable APK)

**Goal:** a single `.apk` that installs by sideloading on a modern arm64 Android phone, with the mobile renderer and touch controls.

Branch: `packaging/android`

---

## 1. Prerequisites (user installs; agent verifies)

- [ ] Android Studio installed.
- [ ] Run `<EngineRoot>\Engine\Extras\Android\SetupAndroid.bat` to install the SDK/NDK versions this engine expects.
- [ ] Restart the editor so it picks up `ANDROID_HOME` / `NDKROOT`.
- [ ] Project Settings → Platforms → Android SDK: paths filled in (or blank to use environment variables).
- [ ] A test phone with Developer Options and USB debugging enabled.

**VERIFY:** do not hardcode SDK/NDK version numbers. Use whatever `SetupAndroid.bat` installs for this engine version.

---

## 2. Android runtime settings

Project Settings → Platforms → Android (writes to `Config/DefaultEngine.ini`; this section is Android-only by nature):

```ini
[/Script/AndroidRuntimeSettings.AndroidRuntimeSettings]
PackageName=com.VERIFY_team.cleancutaction
StoreVersion=1
ApplicationDisplayName=CleanCutAction
bPackageDataInsideApk=True
bBuildForArm64=True
bBuildForX8664=False
bSupportsVulkan=True
bBuildForES31=True
bFullScreen=True
Orientation=SensorLandscape
```

- Click **"Accept SDK License"** in the editor (the agent cannot do this; ask the user).
- **VERIFY** the package name with the user.
- Leave **"For Distribution" unchecked**. The debug key signs the APK, which is fine for sideloading. A release keystore is only needed for the Play Store; ask the user before setting one up.

---

## 3. Renderer

In `Config/Android/AndroidEngine.ini`:

```ini
[/Script/Engine.RendererSettings]
r.RayTracing=False
r.DynamicGlobalIlluminationMethod=0
r.ReflectionMethod=2
r.Shadow.Virtual.Enable=0
r.Mobile.AntiAliasing=1
```

- Mobile renderer. No Lumen, no hardware RT. Nanite meshes use their fallback.
- Use the same content check as 02_Web_WASM.md section 3. If the Web build already passed it, Android will be close.

---

## 4. Device profile and textures

Cook flavor: **ASTC**.

```ini
; Config/DefaultDeviceProfiles.ini
[Android DeviceProfile]
DeviceType=Android
BaseProfileName=
+TextureLODGroups=(Group=TEXTUREGROUP_World,MaxLODSize=2048,LODBias=0)
+TextureLODGroups=(Group=TEXTUREGROUP_WorldNormalMap,MaxLODSize=1024,LODBias=0)
+TextureLODGroups=(Group=TEXTUREGROUP_Character,MaxLODSize=2048,LODBias=0)
+TextureLODGroups=(Group=TEXTUREGROUP_CharacterNormalMap,MaxLODSize=1024,LODBias=0)
+CVars=sg.ShadowQuality=1
+CVars=sg.PostProcessQuality=1
+CVars=sg.EffectsQuality=1
```

- **VERIFY:** if the engine already defines `Android_Low/Mid/High` profiles, add overrides to the relevant ones instead of replacing them.
- Source assets stay untouched (README → Texture policy).

---

## 5. Plugins and code

- DLSS / Streamline excluded (Win64-only `PlatformAllowList`).
- Any platform-specific C++ goes in `#if PLATFORM_ANDROID` blocks.

---

## 6. Input

- Touch controls: virtual joystick + action buttons, sized for thumbs, kept clear of the screen edges and notch.
- Support a Bluetooth gamepad if the input mapping already handles gamepads.
- Handle the Android back button (pause menu instead of quitting immediately).

---

## 7. Build and install

```
Scripts\Build_Platform.bat Android
```

Output: `Builds\Android\` (look for the `.apk`).

Install over USB:

```
adb install -r "Builds\Android\<path>\CleanCutAction-arm64.apk"
```

**VERIFY** the exact APK filename after the first build.

---

## 8. Checklist

- [ ] SDK/NDK installed via SetupAndroid.bat; SDK license accepted by the user
- [ ] Package name confirmed with the user
- [ ] Single APK with data inside; arm64 only
- [ ] Vulkan + ES3.1 enabled; mobile renderer
- [ ] ASTC cook; Android device profile caps textures; source assets untouched
- [ ] No DLSS/Streamline files in the package
- [ ] Installs via `adb install` and launches to gameplay
- [ ] Touch controls usable; back button opens pause
- [ ] Holds acceptable frame rate on the test phone (record device + FPS in BUILD_LOG.md)
- [ ] Common verification in README done
