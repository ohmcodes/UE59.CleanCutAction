# itch.io Page Copy

Text for the itch.io project page. It describes the build that ships now: Phase 1 (clean the trashed cafe set) and Phase 2 (run the effect stations while Dash and Juniper shoot their scene), with an online leaderboard and keyboard/mouse, gamepad and touch controls. Page art and theme settings: `itch/ITCH_THEME_SETTINGS.md`.

## Title

Clean! Cut! Action!

## Short description or tagline

Pick one. All fit the itch.io field (shown under the title on browse pages).

- **Wrap party wrecked the set. Clean it up, then make movie magic on cue.** (recommended)
- Wrap party wrecked the set. Put every prop back on its mark before the cameras roll.
- The cameras roll soon. The set is a disaster. Places, everyone!
- A first-person cleanup game on a trashed movie set. Don't break anything.

## Details

Paste the block below into the **Details** field. The itch.io editor accepts Markdown-style formatting through its rich text box; adjust headings there if they paste as plain text.

---

**Rookie!**

Last night's wrap party got out of hand and the whole cafe set got knocked to pieces. Today Dash and Juniper shoot their big first-date scene right here, and the director wants every cup, chair and croissant back on its mark before the cameras roll.

Break anything and it comes out of your paycheck. Don't make me yell CUT.

*- Marlowe Finch, Director*

### About the game

**Clean! Cut! Action!** is a cozy first-person game set on a two-floor New York cafe movie set. You play the Rookie, a production assistant on the last day of the shoot.

**Phase 1: Clean the set.** Every scattered prop has a glowing silhouette showing exactly where it belongs. Pick it up, line it up, and snap it back into place. Mop up the puddles while you're at it. Drop a prop beside its mark and it slips and shatters. Throw it and it breaks for sure.

**Phase 2: Shoot the scene.** The set is clean and the cameras are rolling. Dash and Juniper (real-life exes, of course) play their date scene, and Marlowe calls the cues. Run between effect stations on both floors to make the rain, the thunder, the espresso steam, the passing car and the boom mic happen right on time, and stay out of the camera's shot.

### Features

- **Put the set back together.** Chairs, tables, cups, plates, pastries and more, each with its own spot on the set.
- **Glowing tape marks.** Blue shows a prop's spot, yellow means you're lined up, orange means something underneath has to go back first (the table before the cup).
- **Things break.** Careless drops and throws smash props into pieces.
- **Two ways to clean.** Race the clock in **Time Trial** for bonus points, or **Just Clean** with no timer and take it easy.
- **Hints when you're lost.** Highlight the nearest stray prop and follow the arrow.
- **Work the effects.** Hold, time, shake and aim your way through the station mini-games. Nail the cue for a Perfect; miss too many and Marlowe yells CUT and the act starts over.
- **Online leaderboard.** Enter your name and see how your cleanup ranks against other players.
- **Plays in your browser** on desktop and touch devices, with keyboard and mouse, a gamepad, or on-screen controls. The on-screen prompts switch to whatever you're using.
- **Dance break.** Because you earned it.

### Scoring

**Phase 1 (on the leaderboard)**

- **+10** for each prop back on its mark
- **-15** for each prop you break
- **Time Trial:** +1 for every second left on the clock

**Phase 2**

- **+100** Perfect cue, **+60** Good, **+25** Late
- **-50** for each CUT
- **-10** for every second you stand in the shot
- Up to **3 stars**. Get all three for the Director's Cut.

### Controls

| Action | Keyboard / Mouse | Gamepad |
|---|---|---|
| Move | W A S D | Left stick |
| Look | Mouse | Right stick |
| Jump | Space | A |
| Crouch | C / Ctrl | R3 (click right stick) |
| Pick up / Place | Left click / E | X |
| Throw (breaks it!) | Right click | RT |
| Hint | F | Y |
| Dance | G | D-pad Up |
| Pause / Unstuck | P / Esc | Menu |
| Effect station: work it | Left click (hold where asked) | RT |
| Effect station: leave | E / Right click | B |

On touch devices, use the on-screen sticks to move and look, and tap the hand button to pick up and place. The name box opens an on-screen keyboard (gamepad: press A on it).

### Tips

- Aim right at the silhouette before you place. The hand icon turns red if the drop will break the prop.
- Put furniture back first. Anything that sits on top of it has to wait.
- In Phase 2, head for the next station as soon as you hear the cue, and walk around the camera, not in front of it.
- The web build needs a browser with **WebGPU** support (recent Chrome or Edge recommended). Give it a moment to load on the first run, then click to start.

### Credits

Made in 3 days for the **Epic Mega AI Jam 2026** with Unreal Engine 5.9.

Environment art: Stylized New York City pack. Third-party assets remain under their own licenses.

Copyright © 2026 QLE OPC. All Rights Reserved.

---

## Page settings reminders

- **Kind of project:** HTML. Upload the zipped Wasm build (`Packaged/Wasm`, see `Docs/PackagingGuide/02_Web_WASM.md`) and tick **This file will be played in the browser**.
- **Embed options:** turn on **SharedArrayBuffer support**. The game won't start without it. Check it's still on after saving.
- **Viewport:** 16:9 (for example 1280×720), with the fullscreen button on. VERIFY the size on the live page.
- Check that no uploaded file is over 200 MB. The current build's largest file is `pakchunk0-Wasm.ucas` at 146.5 MB.
- **Windows download (optional):** if the Windows build is packaged (`01_Windows.md`), upload its zip as a separate downloadable file marked Windows. It's the only build that can enter the NVIDIA RTX Award.
- **Credits:** before publishing, add the music (six `kf0xxxxx` tracks in `Content/_CleanCutAction/MusicFX`, plus `EmoteMusic/paroparo`) and any CC-BY sounds to the Credits block, with their licenses. VERIFY the sources; there is no `CREDITS.md` yet.
- **Genre:** Simulation or Puzzle. **Tags:** cozy, cleaning, first-person, 3d, casual, funny, leaderboard, controller, unreal-engine, game-jam.
- **Inputs** (Metadata): Keyboard, Mouse, Gamepad, Touchscreen.
