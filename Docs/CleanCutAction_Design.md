# Clean! Cut! Action! - Game Design Document

**Jam:** 3-day AI game jam
**Theme:** Director's Cut
**Genre:** 3D casual tidying + rhythm/timing action
**Reference game:** *Librarian: Tidy Up the Arcane Library!* (ArtRising, 2026)

---

## 1. Concept

A messy movie set has to be cleaned before the director can start filming. Once the set is ready, two actors play a date scene, and the player helps the scene work by operating practical effects on the director's cues: a sprinkler for rain, a cardboard car for traffic, thunder sheets, prop hand-offs, and more.

**Core loop:** Clean the set > unlock abilities > ACTION! > hit the director's cues > earn Stars > spend Stars in the skill tree.

---

## 2. Title Options

| Title | Why it works |
|---|---|
| **Clean! Cut! Action!** (working title) | Shared with the film-within-the-game; hints at romance and retakes |
| Quiet on Set! | Short, instantly reads as "movie set," ironic since the set is chaos |
| Set Dressed | Pun on the real film job of set dressing |
| That's a Wrap! | Upbeat, doubles as the ending line |
| Places, Everyone! | Fits "put everything in its place" |
| Rain on Cue | Points at the signature mechanic |

If the jam requires the theme in the title: **Clean! Cut! Action!: A Director's Cut**

---

## 3. Narrative

**Premise:** You're the new Production Assistant ("the Rookie") on the last day of shooting for *Clean! Cut! Action!*, a rom-com by Marlowe Finch, a famously picky director whose career depends on this film. Last night's wrap party left the set a disaster, and the actors arrive in one hour.

### Characters
- **Marlowe Finch (Director):** dramatic and impatient, but warms up to you. Shouts cues through a megaphone and grades every take. He is also the tutorial voice.
- **Dash & Juniper (Actors):** co-stars who are secretly exes and can barely look at each other. The scene is their characters' first date in the rain.
- **The Rookie (Player):** silent protagonist. Marlowe carries the personality.

### Story Beats
1. **Intro:** An angry note from Marlowe is pinned to the call sheet (the objective). "One hour, Rookie. If I see a single popcorn kernel in frame, you're fired." It promises a reward: craft-services donuts.
2. **Mid-clean:** Dash and Juniper arrive early and argue, leaving a few extra items to tidy.
3. **Action:** Marlowe calls "ACTION!" and directs you through the date scene.
4. **Twist:** Your rain effect is so convincing that Dash and Juniper forget their lines and reconnect for real. Marlowe whispers, "Keep rolling..."
5. **Ending:** "That's a wrap!" The final shot is a clapperboard reading **DIRECTOR'S CUT**. Marlowe hands you a crew jacket: "Same time tomorrow, *Assistant Director*."

**Joke ending (optional):** Leave the set without finishing any zone > Marlowe screams "YOU'RE FIRED!" (a nod to Librarian's joke achievement).

---

## 4. Phase 1: Cleaning the Set

Every item has a glowing tape-mark outline where it belongs. Bringing the item close snaps it into place.

| Interaction | How it works | Feel |
|---|---|---|
| Return props | Carry chairs, lamps, vase to their tape-mark silhouettes | Snap + chime |
| Trash pickup | Cups, pizza boxes, confetti into bins; throwing counts | Basketball-style arc |
| Sweep | Broom over popcorn/confetti piles (radius-based decal erase) | Visible progress |
| Untangle cables | Follow a cable from light to outlet, click to straighten | Mini spline puzzle |
| Aim the lights | Rotate stage lights until beams hit the marked "X" | Light turns warm when correct |
| Costume rack | Hang outfits by color, match hanger tags | Sorting |
| Stack film reels | Stack reels 1-5 in order on the shelf | Order puzzle (like Librarian's volume order) |
| Wipe camera lens | Hold to wipe a smudged lens; the camera view clears | Satisfying reveal |
| Find the clapperboard | Hidden item, required to finish | Mini scavenger hunt |

**Implementation:** One generic `Placeable` component (item ID + target transform + snap radius) covers most of this table.

---

## 5. Phase 2: The Action Scene

Marlowe calls a cue and an icon with a timer ring appears. Run to the matching station and operate it before the ring empties. The scene is a scripted 60-90 second sequence.

### Signature Mechanic: Stay Out of Frame
The camera's view is drawn on the floor as a translucent cone. Standing in it > "You're in the shot!" and a point penalty.

| Cue | Station | Player action |
|---|---|---|
| "RAIN!" | Sprinkler valve + hose | Turn valve, aim spray over actors, stay out of frame |
| "TRAFFIC!" | Cardboard car on a rail | Push it back and forth to a rhythm prompt |
| "WIND!" | Giant fan | Crank to keep the wind meter in the sweet spot |
| "THUNDER!" | Thunder sheet + flash lamp | Shake sheet, then flash; matched timing = bonus |
| "UMBRELLA!" | Prop table | Toss umbrella into the actor's catch zone |
| "ROSES!" | Prop table | Same, different prop |
| "SUNSET!" | Light rig | Swap the gel to orange |
| "BOOM!" | Boom mic | Hold above the frame line without dipping in |

**Scoring:** Each cue is Perfect / Good / Late; frame violations cost points. Result: 1-3 Stars + a Marlowe quote. A failed take > "CUT! Take two!" and an instant retry, never a game over.

**Implementation:** Store the scene as a list of `{time, cueType, duration}` in a ScriptableObject or JSON. New scenes are data, not code.

---

## 6. Skill Tree: "Crew Credits"

Earn **Stars** from completing zones and from take ratings. Spend them in the Rookie's **notebook** (diegetic UI). Abilities are a mix of passive stats and **active abilities with short duration + cooldown** (Librarian's approach: the player times them, which keeps them engaging).

### Hustle (movement)
1. **Comfy Sneakers** (passive): +20% move speed
2. **Dodge the Shot** (passive): camera cone flashes before you step in
3. **Sprint Burst** (active): short dash, cooldown

### Prop Master (cleaning)
1. **Stack 'Em** (passive): carry 2 items (3 at max rank)
2. **Wide Broom** (passive): bigger sweep radius
3. **Director's Eye** (active): highlights all misplaced props of the type you're holding
4. **Grab Bag** (active): pulls nearby props of the same type into your hands

### Special Effects (action scene)
1. **Cue Sense** (passive): see the next cue icon 2 seconds early
2. **Steady Hands** (passive): wider "Perfect" timing window
3. **Long Throw** (passive): longer toss range
4. **Director's Patience** (active, once per take): freeze the cue timer for 3 seconds

### Hidden Collectibles: Golden Film Reels
2-3 hidden reels around the set unlock free perks (Sprint, Carry +1). Place the **first reel in the player's first view** so exploration pays off immediately.

---

## 7. Lessons from *Librarian: Tidy Up the Arcane Library!*

### Why it worked
- **Very Positive on Steam (~93% of ~17k English reviews), priced at $5.99.**
- **Progression carries the game.** Review analytics show players under 2 hours were only ~60% positive, versus 95%+ after that. The chore becomes fun once upgrades kick in, so *the early minutes are the weakest point.*

### Object interaction
- **Two-tier feedback:** each correctly placed book glows golden; a completed row's indicator turns blue and plays a sound. A dark indicator means something in the row is wrong.
- **Gentle errors:** a red glow flags mistakes, with no punishment. Failure is even a joke achievement.
- **Readable objects:** categories are recognizable from cover design (e.g., curse books are black with red text), so players build personal mastery.
- **Humor in the objects:** players love the funny book titles.
- **Sound as texture:** footsteps and the thud of dropped books.
- **The world is the progress bar:** the floor is revealed as books are shelved, and ambient sound and music grow richer as shelves fill.

### UI
- **Diegetic:** the objective is a letter from the principal, category maps hang on the wall by the stairs, and abilities are bought from a personal handbook.
- **Unobtrusive:** reviewers praise how little the UI gets in the way.
- **Live judgment from a character:** the Principal's evaluation updates in real time based on correct rows.

### Onboarding
- A clear reason (the letter) plus a carrot (pizza).
- **The first key is directly in view at spawn** and unlocks Jump, which is needed for the other keys.
- Easy early rows with obvious visual cues give quick wins.

### Retention
- Rows earn points for 5 active abilities (Assemble, Insight, Sorting, Auto-Shelving, Shelf Guide), each with duration and cooldown that improve with upgrades.
- A second track: 4 hidden keys > chests with Sprint, Jump, and carry capacity (10 > 15).
- Replay hooks: speedrun, no-magic run, joke ending, post-game Special Stage.

### What players complained about
- Broken book physics and stacking
- Overpowered magic abilities
- Short length / low replayability
- Controversy over generative AI use

---

## 8. Applying the Lessons

1. **Two-tier feedback:** gold sparkle per prop; a set light turns on + chime when a whole zone is done.
2. **The set is the progress bar:** stage lights come on zone by zone; music layers build until "ACTION!"
3. **Diegetic UI:** Marlowe's note on the call sheet, a set map taped to a wall, the skill tree in the Rookie's notebook.
4. **Fix the first two minutes:** give the first ability within 1-2 minutes and put the first golden reel and the clapperboard in the opening view.
5. **Active abilities with cooldowns:** Director's Eye and Grab Bag.
6. **One hidden collectible track:** golden film reels.
7. **Jokes in the props:** silly script titles, crate labels, and costume tags.
8. **Avoid their mistakes:** snap-to-slot instead of free physics stacking; don't let abilities trivialize the game too early.

---

## 9. 3-Day Jam Plan

| Day | Focus | Deliverables |
|---|---|---|
| **Day 1** | Cleaning | Player controller, pickup/carry/place with snap targets, trash bins, two-tier feedback, Set Ready meter, graybox of one set (cafe street corner) |
| **Day 2** | Action | Cue sequencer, 4 effects (Rain, Traffic, Thunder, Umbrella toss), camera frame cone, scoring + Star rating, simple actor animations (idle, talk, reaction) |
| **Day 3** | Wrap-up | Skill tree notebook UI (~6 nodes), golden reels, dialogue popups for the 5 story beats, audio (megaphone barks), ending clapperboard, bug fixes |

**Cut first if short on time:** cable untangling, wind fan, costume rack.
**Never cut:** snap feedback, Marlowe's voice lines, the rain effect (your trailer shot).

---

## Sources

- Steam store page: https://store.steampowered.com/app/4197610/Librarian_Tidy_Up_the_Arcane_Library/
- KeenGamer review: https://www.keengamer.com/articles/reviews/pc-reviews/librarian-tidy-up-the-arcane-library-review-simply-spellbinding/
- Gaming.net review: https://www.gaming.net/reviews/librarian-tidy-up-the-arcane-library-review-pc/
- Whisper of the House guide hub: https://www.whisperofthehouse.com/librarian
- Vaporlens review analytics (third-party estimates): https://vaporlens.app/app/4197610/librarian_tidy_up_the_arcane_library
