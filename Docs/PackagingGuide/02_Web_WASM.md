# 02 — Web / Mobile Browser (WebAssembly + WebGPU)

**Goal:** CleanCutAction running in desktop Chrome and Chrome on Android, with no install, small download, mobile renderer.

Branch: `packaging/web`

---

## 1. Facts about this target (UE 5.9 MegaJam Preview)

- Web is **experimental**: WebAssembly + WebGPU, mobile renderer, **source build and C++ project required**, Chrome gives the best experience.
- Known issue: premade Mac/Linux engine builds are missing some Wasm support files → **package Web from Windows using the source build**.
- Known issue: AMD browser GPU crashes on the experimental *desktop* rendering tier → **use the mobile renderer tier**.
- Parts of the web renderer are prototypes. If something breaks, check the 5.9 Preview known-issues thread before debugging deeply.

---

## 2. Discover the platform name (do this first)

**VERIFY** and write the answers at the top of this file:

- Platform folder name under `Engine/Platforms/` (e.g. `Web`, `HTML5`, something else): `________`
- Its config folder name, for `Config/<Platform>/<Platform>Engine.ini`: `________`
- The `-platform=` value UAT accepts: `________`
- The device profile name it uses: `________`

Then set `WEB_PLATFORM` in `Scripts/Build_Platform.bat`.

---

## 3. Renderer and features

In `Config/<WebPlatform>/<WebPlatform>Engine.ini`:

```ini
[/Script/Engine.RendererSettings]
r.RayTracing=False
r.DynamicGlobalIlluminationMethod=0
r.ReflectionMethod=2
r.Shadow.Virtual.Enable=0
```

- **VERIFY** the setting that selects the mobile renderer tier for this platform, and make sure it is not the desktop tier.
- No Lumen, no hardware RT, no Nanite rendering (meshes use their Nanite fallback mesh).

**Content check:** play the main map in the editor with the preview platform set to Android/mobile (Settings → Preview Rendering Level). Lighting must still read well with directional light + skylight. Report any meshes whose Nanite fallback looks broken; do not edit the meshes without asking.

---

## 4. Plugins

- DLSS / Streamline must not be included (`PlatformAllowList` = Win64 only, see 01_Windows.md).
- **Ask the user** which other plugins are unused, then disable them for this platform. Fewer plugins means a smaller download.

---

## 5. Size budget (the main fight)

- `MapsToCook`: only the maps the web build needs. If the full game is too big, agree with the user on a reduced "web cut".
- Textures: apply limits in the web device profile, never on the source assets (README → Texture policy).

```ini
; In Config/DefaultDeviceProfiles.ini — replace VERIFY_WebProfile with the real name
[VERIFY_WebProfile DeviceProfile]
DeviceType=VERIFY
BaseProfileName=
+TextureLODGroups=(Group=TEXTUREGROUP_World,MaxLODSize=1024,LODBias=0)
+TextureLODGroups=(Group=TEXTUREGROUP_WorldNormalMap,MaxLODSize=1024,LODBias=0)
+TextureLODGroups=(Group=TEXTUREGROUP_Character,MaxLODSize=1024,LODBias=0)
+TextureLODGroups=(Group=TEXTUREGROUP_CharacterNormalMap,MaxLODSize=1024,LODBias=0)
+TextureLODGroups=(Group=TEXTUREGROUP_UI,MaxLODSize=1024,LODBias=0)
+CVars=sg.ShadowQuality=1
+CVars=sg.PostProcessQuality=1
+CVars=sg.EffectsQuality=1
```

- Audio: check large audio files' compression quality. Report the biggest offenders rather than re-encoding source files.
- After each build, record the total download size in `BUILD_LOG.md`.

---

## 6. Input

- Mouse/keyboard for desktop browsers.
- Touch controls for mobile browsers (virtual joystick or tap controls). Show them only when touch input is detected.
- Request fullscreen/pointer lock from a user click, since browsers block it otherwise.

---

## 7. Build

```
Scripts\Build_Platform.bat Web
```

Output: `Builds\Web\`

---

## 8. Hosting and testing

Server requirements:

- `.wasm` served as `application/wasm`.
- gzip or brotli compression for `.wasm`, `.js` and data files, with matching `Content-Encoding` headers.
- If the build is multithreaded (needs SharedArrayBuffer), serve these headers on the page:
  ```
  Cross-Origin-Opener-Policy: same-origin
  Cross-Origin-Embedder-Policy: require-corp
  ```
- **itch.io:** upload as HTML, tick "SharedArrayBuffer support", set the viewport size.

Local test: use a local server that can send the headers above. Double-clicking the HTML file will not work.

Test order:

1. Desktop Chrome
2. Chrome on Android
3. Other browsers: best-effort, note results only

---

## 9. Checklist

- [ ] Platform name, config folder, UAT name and device profile filled in (section 2)
- [ ] Mobile renderer tier confirmed
- [ ] No DLSS/Streamline files in the package
- [ ] Unused plugins disabled (with user approval)
- [ ] Web device profile caps textures; source assets untouched
- [ ] Download size recorded and agreed acceptable with the user
- [ ] Runs in desktop Chrome
- [ ] Runs in Chrome on Android with touch controls
- [ ] Hosting headers verified on the real host
- [ ] Common verification in README done
