# AI usage (jam submission record)

Written 2026-11-15 from what the repo shows. Trim anything that doesn't match what actually happened.

---

## "Which OpenAI models or products did you use, and how?"

**No OpenAI model or product appears anywhere in the project** (source, scripts, docs, config or git history, checked 2026-11-15).

- The AI coding agent is **Claude Code by Anthropic**, not OpenAI. Don't describe it in the OpenAI field.
- The video-to-animation tool uses **Google MediaPipe** (pose landmarker), not an OpenAI model.

If an OpenAI product was used outside the repo (ChatGPT, Sora, gpt-image, Codex, for cover art, the dance video, music or writing), name it and what it made. Otherwise leave the field blank or write "None". That only rules the entry out of the OpenAI award.

---

## How we used AI (paste-ready)

We built *Clean! Cut! Action!* in UE 5.9 with **Claude Code (Anthropic)** as our main coding agent. It worked directly in the project and connected to the Unreal Editor over MCP. We kept one repo with a branch per target: main, Web (Wasm/WebGPU), Windows, Linux/Steam Deck and Android.

- **Gameplay systems in C++:** prop pickup, carry and placement with home silhouettes, the "earthquake" that wrecks the set, cleaning (trash, dishes, mopping), and the Phase 2 shoot. The shoot has a cue/beat director, effect stations, the camera-frame cone and a take meter with retakes.
- **UI and input:** every menu and HUD is built in C++ with one shared theme. Keyboard and mouse, gamepad and touch all work, and button prompts switch to whichever device you're using.
- **Online leaderboard:** uses the browser's `fetch()` on the web build and the engine's HTTP module elsewhere.
- **Writing:** director Marlowe's heckles and the Phase 2 scene script.
- **Packaging for every branch:** fixed cook failures, kept the web build under itch.io's 200 MB file limit by capping texture sizes for the web only, and turned the Windows build into an RTX showcase. For that build we ported NVIDIA's DLSS 4.5 plugin to UE 5.9.
- **Video to animation:** a Python pipeline uses Google's MediaPipe pose model to track a person dancing in a video. It solves bone rotations for the Rookie's skeleton and writes an Unreal animation through the editor's Python API. That's how we made the Rookie's dance emote (G key) without motion capture or retargeting.

### Where things are

| Item | Location |
|---|---|
| Video to animation tool | `Scripts/Python/VideoToAnim/` (`track.py`, `solve.py`, `ue_emote.py`, README), commit `2621808` (2026-11-14) |
| Emote it produced | `/Game/_CleanCutAction/Character/Rookie/Anims/A_Emote_Rookie` |
| Branch folders | main `F:\UE5 Projects\CleanCutAction`, wasm `CCA_WASM`, windows `CCA_Windows`, linux `CCA_Linux`, android `CCA_Android` |
| DLSS 5.9 port (reusable kit, outside the repo) | `F:\UE5 Projects\_SharedKits\NVIDIA_DLSS_UE59_Kit\` |

### Before you paste

- Git shows Claude as co-author on 16 of 106 commits; the video-to-animation commit isn't one of them. The repo can't show who wrote the scripts, the lines or the art, so trim any bullet that doesn't match.

---

## NVIDIA: "How did you use the NVIDIA features you checked above?" (paste-ready)

Tick only **NVIDIA DLSS**. Full notes: `NVIDIA_RTX_Award.md`.

> **NVIDIA DLSS 4.5 (Windows build)**
>
> *Clean! Cut! Action!* uses DLSS Super Resolution to upscale the two-floor New York cafe set. That lets the Windows build run hardware ray-traced Lumen lighting and reflections at Cinematic settings and still play smoothly during the Phase 2 shoot, while rain, steam and thunder effects run and the player sprints between stations. DLSS Ray Reconstruction cleans up the ray-traced reflections on the cafe's windows and glossy surfaces. DLSS is on by default (Quality), and players and judges can change it live from the pause menu: **NVIDIA DLSS** (Off/TSR, DLAA, Quality, Balanced, Performance) and **Ray Reconstruction** (On/Off). On RTX 40/50 GPUs, a **Frame Generation** toggle also appears, with Reflex enabled for low latency. GPUs without DLSS fall back to TSR.
>
> **We ported the DLSS 4.5 plugin to UE 5.9.** NVIDIA's plugin only supports UE 5.6–5.8, and it doesn't compile on the 5.9 MegaJam Preview. We fixed it for 5.9 without changing the engine: 5.9's stricter format-string checks and the plugins' engine version. We packaged the result as a reusable kit with a one-command installer, so other 5.9 projects can add DLSS in minutes. We'd be glad to share it with NVIDIA or other jam teams.

### Before you paste

- **Frame Generation:** only tested on an RTX 3070, which can't run it. Without an RTX 40/50 test, delete that sentence.
- **Pause-menu toggles:** check the DLSS and Ray Reconstruction buttons in Play before submitting.
- **Kit offer:** the kit contains NVIDIA's plugin files, so sharing it with other teams may fall under NVIDIA's license (`NVIDIA RTX SDK EULA.pdf` in the kit). Offering it to NVIDIA directly is fine.
