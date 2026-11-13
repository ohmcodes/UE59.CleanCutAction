# itch.io page setup: Clean! Cut! Action!

The colours match the in-game UI theme (`Source/CleanCutAction/UI/LTTUITheme.h`): cream paper panels, gold trim, wood brown, and brown ink.
Regenerate the images with `python Saved/Scripts/make_itch_art.py`. The cafe source captures are in `Saved/ItchShots/`.

## Files in this folder

| File | Where it goes on itch.io |
|---|---|
| `cover_630x500.png` | **Edit game > Cover image** (the thumbnail in browse/search lists) |
| `banner_960x300.png` | **Edit theme > Banner** (transparent PNG, sits on top of the page background) |
| `banner_960x300_solid.png` | Same banner with the cafe behind it. Use it for devlogs and jam submissions, or as the banner if you choose a flat page background |
| `background_2560x1440.jpg` | **Edit theme > Background image** |
| `embed_bg_1280x720.png` | Logo splash with a clear centre (itch puts its "Run game" button there). Upload it as the **first screenshot**, use it as a jam thumbnail, or place it at the top of the description |
| `embed_bg_plain_1280x720.jpg` | Text-free blurred cafe. Use it behind the game frame or loader (see the Embed section) |
| `screenshot_1..3_*.png` | **Edit game > Screenshots** (1920x1080) |

## Edit theme (open the game page and click "Edit theme" at the top)

### Layout
- **Banner:** `banner_960x300.png`, **Align: Center**
- **Hide game title:** OFF (the banner reads "Last Day on Set" and leaves the game title to itch)
- **Layout:** the column layout with screenshots in the side column works best for a 16:9 embed

### Background
- **Background image:** `background_2560x1440.jpg`
- **Repeat:** No repeat
- **Position:** Center (top). Turn on **Fixed/attach** if offered, so the film strip stays still while scrolling
- **Background colour:** `#2B1A0C` (deep coffee; shows past the image edges)

### Colours

| Setting | Hex | Theme name | Notes |
|---|---|---|---|
| BG (page) | `#2B1A0C` | Backdrop | Matches the image edges |
| BG2 (content column) | `#FBF7EC` | Paper | Cream panel, like the in-game menus |
| Text | `#4A3018` | Ink | Brown body text |
| Link | `#9A6633` | Wood | Use `#7A4E22` (HeaderBrown) if you want more contrast |
| Button | `#F4C45A` | ButtonGold | Primary "Run game" / "Download" button |
| Button text | `#4A3018` | Ink | If the editor offers it |
| Embed BG | `#2B1A0C` | Backdrop | Area around the game frame |
| Border/accents (if offered) | `#C9A15A` | GoldTrim | |

Alternative dark column: BG2 `#3A2412`, Text `#FFF8E6`, Link `#F4C45A`, Button `#F4C45A`. Cream is closer to the in-game menus.

### Fonts
- **Header font:** *Lilita One* (closest to the chunky cream-on-brown title). Fallbacks: *Fredoka One* or *Bowlby One*
- **Body font:** *Nunito* or *Rubik*
- **Font size:** Medium (or Large)

If those fonts aren't in itch's list, pick the roundest bold font for headers and a clean sans for body text.

## Embed / HTML settings (Edit game > Uploads / Embed options)
- **Kind of project:** HTML. Tick **This file will be played in the browser** on the Wasm zip
- **Viewport:** 1280 x 720 (16:9; matches the embed art). Tick **Fullscreen button**
- **Mobile friendly:** ON if you support touch (the game has touch controls), orientation **Landscape**
- **Automatically start on page load:** OFF, so players see the "Run game" button over the cover/embed BG
- **SharedArrayBuffer support:** **ON** (required by the UE Wasm build)
- Keep `pakchunk0-Wasm.ucas` under **200 MB** (itch's per-file limit)

### Optional: art on the game's own loading screen
The packaged loader (`Packaged/Wasm/index.html`) draws a gradient in `#art` (`linear-gradient(var(--cover-top), var(--cover-bottom))`).
To show the cafe while it loads, copy `embed_bg_plain_1280x720.jpg` next to `index.html` and change that rule to
`background:url(embed_bg_plain_1280x720.jpg) center/cover`. This file is regenerated on every package, so redo the change (or patch the engine's Wasm template) after each build.

## Description styling tips
- Section headers: use the header font through itch's Heading style, e.g. **HOW TO PLAY**, **CONTROLS**, **CREDITS**
- Put `embed_bg_1280x720.png` or a GIF at the top of the description as a hero image
- Credit the StylizedNewYorkCity asset pack and the Epic Mega AI Jam 2026
