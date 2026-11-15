# Script - Director Lines & Scene 42

Phase 1 lines (§1) are hard-coded in C++ (`ALTTCleanupBarker::BuildPools`, `Source/CleanCutAction/Interaction/LTTCleanupBarker.cpp`); there is no DataTable for them. Import the cue/result lines in §2 into `DT_DirectorLines`; import the beat structure in §2.2 into `DA_Scene42`.

---

## 0. Cast & Data Notes

| Speaker | Who | Voice direction | Blip pitch |
|---|---|---|---|
| **Marlowe** | Marlowe Finch, the director | Theatrical, exhausted, secretly sentimental. Always through the megaphone (megaphone submix) | Low, 0.8 |
| **Dash** | Dash, lead actor (plays **Leo**) | Charming, nervous, over-explains | Mid, 1.0 |
| **Juniper** | Juniper, lead actress (plays **Clara**) | Dry, quick, guarded, laughs despite herself | High, 1.2 |
| **Walkie_Dash / Walkie_Juniper** | Same actors over the walkie-talkie in Phase 1 | Walkie submix + squelch | Same |

**Subtitle speaker labels:** in-character lines show the character name with the actor in brackets, e.g. "LEO (Dash)". Off-script lines show the actor's name in italics with an "(off-script)" tag.

`DT_DirectorLines` columns: see `02 §9`. In the tables below, **ID** = RowName, **Pri** = priority, **%** = chance. Defaults if blank: Pri 1, 100%, no one-shot.

**Phase 1 as built:** no voice-over, blips or submixes. Lines are text in `ULTTBarkWidget` (gold pill, bottom centre, megaphone icon) with the speaker label "MARLOWE", "DASH (walkie)" or "JUNIPER (walkie)" (`ALTTCleanupBarker::GetCurrentLine`). `ALTTCleanupBarker` has optional `LineSound` (Marlowe) and `WalkieSound` (squelch) slots. The **ID** in §1 is the `LOCTEXT` key in `LTTCleanupBarker.cpp`, not a DataTable row.

---

## 1. Phase 1 - Marlowe While You Clean

**As built.** Every line below is the shipped text from `ALTTCleanupBarker::BuildPools` (`Source/CleanCutAction/Interaction/LTTCleanupBarker.cpp`). The **ID** column is the `LOCTEXT` key; the **Pool** is the name passed to `Trigger()`. To change a line, edit the C++. Lines show as text in `ULTTBarkWidget` (no VO). `{0}` / `{1}` are filled at runtime (key labels follow keyboard or gamepad).

**Rules** (`LTTCleanupBarker.h`): Pri 3 = tutorial / story / finale, Pri 2 = milestones, Pri 1 = reactions. Non-chain pools pick lines from a shuffle bag (no repeats until the pool runs dry). Reactions roll their chance × `ChattinessScale` and are at least 4 s apart. A new line can cut in after the current one has been up 1 s; a cut Pri 2–3 line is said again afterwards, a cut reaction is dropped. Once the player has placed 10 props, the next interrupted tutorial line ends all tutorial lines (intro + `First.*`) for the session. Console `LTT.DisableBarks` (or `1`) silences everything and clears the queue; `LTT.DisableBarks 0` turns it back on.

Defaults if blank: Pri 1, 100%, no cooldown.

### 1.1 Intro (`Intro`, Pri 3, one-shot chain)
Starts 1.2 s after the briefing closes and the screen fades in. `{0}` = hint key.

| ID | Line |
|---|---|
| P1_Intro_1 | "Rookie! Welcome to the last day of shooting on Clean! Cut! Action! Look around. Breathe it in. That smell? That's my career." |
| P1_Intro_2 | "Last night's wrap party got... enthusiastic. The actors arrive in one hour, and this set needs to be spotless." |
| P1_Intro_3 | "See those see-through outlines? Every prop has a home. Put everything back exactly where it lives. Exactly. I measure with my soul." |
| P1_Intro_4 | "Start with something close. Pick it up. Put it on its mark. I'll wait. I'm a patient man. That's a lie, but go." |
| P1_Intro_5 | "Lost? Press {0} and I'll point. Grudgingly." |

### 1.2 Power out (the set starts pitch black, see `03 §4`)

| ID | Pool | Pri | Trigger | Line |
|---|---|---|---|---|
| P1_PowerOut_01 | `Power.Out` (one-shot) | 3 | After the intro, when nobody is talking and the power is off | "And somebody tripped the breaker at the party. Find the electric box - look for the little red light - and turn the power back on." |
| P1_PowerRemind_01 | `Power.Remind` | 2 | Every 45 s (`PowerNagSeconds`) while the power is still off and nobody is talking | "Still dark, Rookie! The electric box. Red light. Flip it. Let there be light." |
| P1_PowerRemind_02 | `Power.Remind` | 2 | | "I can't direct what I can't see. Power. Box. On. Now." |
| P1_PowerRemind_03 | `Power.Remind` | 2 | | "The breaker, Rookie! This is a film set, not a seance." |
| P1_PowerRemind_04 | `Power.Remind` | 2 | | "Follow the arrow to the electric box and turn the power on. I'll be here. In the dark. Brooding." |
| P1_Cable_01 | `Cable.Power` | 2 | Power switched back on after being off | "Power restored! I feel a tingle. Either it's art, or I'm being electrocuted." |

### 1.3 First-time tutorials (`First.*`, Pri 3, one-shot)
Prop kinds come from `ELTTBarkPropKind` (`BarkKind` on the prop; `Auto` picks from the class name: Table/Chair → Furniture, Lamp → Light, Socket → Electric, Camera → Camera, Book → Shelf, Pillow/Curtain → Fabric, Broom/Mop/Spray/Soap → Broom).

| ID | Pool | Trigger | Line |
|---|---|---|---|
| P1_First_Pickup | `First.Pickup` | First pickup (not a dish) | "Look at you, holding a prop. Your parents must be so proud." |
| P1_First_Dish | `First.Dish` | First pickup of a dishwashable prop | "Dirty dishes go in the sink. Somebody drank four lattes out of that. It was me." |
| P1_First_Place | `First.Place` | First prop put back | "Hear that? That's the sound of order. Burn it into your memory." |
| P1_First_Stack | `First.Stack` | First prop put back on top of another prop (cup on its table) | "Stacking! Efficient. Reckless. I respect both." |
| P1_First_Throw | `First.Throw` | First throw. `{0}` = "Right-click" / "Right trigger" | "{0} to throw. Wait, why am I teaching you to throw props on my set? It BREAKS them!" |
| P1_First_Break | `First.Break` | First prop broken | "Gently! Everything on this set is rented! Broken props cost points. And my deposit." |
| P1_First_Bin | `First.Bin` | First trash in a bin | "Trash goes in the bin. Revolutionary, I know. Write it down." |
| P1_First_Broom | `First.Broom` | First pickup of cleaning gear (the mop) | "That's cleaning gear. Push it at the mess. It's basically filmmaking." |
| P1_First_RightIt | `First.RightIt` | First pickup of furniture lying on its side. `{0}` = prop name | "Flip that {0} back up. Gently. It has a stunt double, and it's busier than you." |
| P1_First_Light | `First.Light` | First lamp picked up | "A light! Put it on its mark. The mark is where the magic happens. Magic is expensive. Don't break it." |
| P1_First_Cable | `First.Cable` | Looking at the electric box, or a socket (focus or pickup) | "Follow the cable to the outlet. Like a detective, but useful." |
| P1_First_Reel | `First.Reel` | First book picked up | "Those go on the shelf in order. Like film reels. One through five. Like the stages of grief." |
| P1_First_Costume | `First.Costume` | First pillow / curtain picked up | "Soft goods! Juniper's blues on the left, Dash's browns on the right. Dash only wears brown. Don't ask. It's a whole thing." |
| P1_First_Lens | `First.Lens` | Looking at / picking up the film camera | "Wipe the lens! Someone licked it. I don't want to know who. I know who." |

### 1.4 Correct placement (`Place.Correct`, 25%)
Said on a placement when no more specific line (tutorial, trick shot, kind line, streak) fired. Also for set dressing put back on its mark.

| ID | Line |
|---|---|
| P1_Place_01 | "Yes. Yes! That's where it lives." |
| P1_Place_02 | "Beautiful. I'd frame that. I won't. But I'd think about it." |
| P1_Place_03 | "Perfect. Now do that eighty more times." |
| P1_Place_04 | "The lamp thanks you. I thank you. My therapist thanks you." |
| P1_Place_05 | "See? The set is healing." |
| P1_Place_06 | "Gorgeous placement. Very motivated. Very method." |
| P1_Place_07 | "Correct! Somewhere, a production designer stopped crying." |
| P1_Place_08 | "That's a keeper. Print it." |
| P1_Place_09 | "Like a puzzle piece coming home for the holidays." |
| P1_Place_10 | "Ooh, the angle on that. You've done this before. Have you done this before?" |
| P1_Place_11 | "Exactly on the mark. I may have misjudged you, Rookie." |
| P1_Place_12 | "Nailed it. Not literally. Please don't nail anything." |

### 1.5 Streaks (`Streak.3/5/10`, Pri 2)
A placement within 6 s of the previous one (`StreakWindow`) adds to the streak. Trash and dishes don't count.

| ID | Streak | Line |
|---|---|---|
| P1_Streak_3 | 3 | "Ooh, a combo! Who ARE you?" |
| P1_Streak_5 | 5 | "Five in a row! Somebody get this Rookie an agent!" |
| P1_Streak_10 | 10 | "TEN! I'm getting emotional. Don't look at me. Keep going. Don't look at me." |

### 1.6 Wrong spot (`Place.Wrong`, 30%, cooldown 15 s)
1.2 s after a plain drop, if the cleanup prop isn't home or held (not for trash, dishes, or a prop already told off by the bin / wrong-order lines).

| ID | Line |
|---|---|
| P1_Wrong_01 | "That's not its home. That's its vacation spot." |
| P1_Wrong_02 | "Close isn't a mark. Close is how we get sued." |
| P1_Wrong_03 | "Interesting choice. Wrong, but interesting." |
| P1_Wrong_04 | "That prop looks lost and confused. Much like our lead actor." |
| P1_Wrong_05 | "If I can see it from here, it's wrong. I can see it from here." |
| P1_Wrong_06 | "Floor is not a mark. The floor is the enemy." |

### 1.7 Wrong order (`Reel.Wrong`, cooldown 20 s)
Interact while aiming at a home whose support isn't back yet (the prop is dropped). `{0}` = held prop, `{1}` = the prop that has to go back first.

| ID | Line |
|---|---|
| P1_ReelWrong_01 | "The {0} before the {1}? Are you EDITING my film?" |
| P1_ReelWrong_02 | "Order, Rookie. The {1} goes back first. That's the whole trick." |

### 1.8 Crashes, throwing, trick shots

| ID | Pool | Pri | % | Cooldown | Trigger | Line |
|---|---|---|---|---|---|---|
| P1_Crash_01 | `Crash` | 1 | 50 | 20 s | A prop the player let go of hits something at ≥ 650 cm/s (`CrashSpeed`), or a prop breaks (after `First.Break`) | "Was that crash part of the plan?" |
| P1_Crash_02 | `Crash` | | | | | "I heard that. The insurance company heard that." |
| P1_Crash_03 | `Crash` | | | | | "Gently! Everything on this set is rented!" |
| P1_Crash_04 | `Crash` | | | | | "Ah. The sound of my deposit leaving forever." |
| P1_Throw_01 | `Throw` | 1 | 15 | | Any throw after the first | "Are you throwing props? On MY set? ...Do it again, that was cool." |
| P1_Throw_02 | `Throw` | | | | | "Nice arc. Terrible manners." |
| P1_Throw_03 | `Throw` | | | | | "Props are not frisbees. Except the reels. Apparently." |
| P1_TrickShot_01 | `TrickShot` | 2 | 100 | | Thrown prop lands in its own silhouette | "Did that just land on its mark? From THERE? I'm putting that in the blooper reel and the trailer." |
| P1_TrickShot_02 | `TrickShot` | | | | | "Trick shot! You're wasted on tidying. You should be in stunts." |

### 1.9 Trash, bin and sink

| ID | Pool | Pri | % | Trigger | Line |
|---|---|---|---|---|---|
| P1_Bin_01 | `Trash.Bin` | 1 | 15 | Trash put / dropped in a bin | "Into the bin. Goodbye, pizza. You were a good pizza." |
| P1_Bin_02 | `Trash.Bin` | | | | "Somebody ordered eleven pizzas and ate four slices. I'm not naming names. Dash." |
| P1_Bin_03 | `Trash.Bin` | | | | "Is that a shoe? Why is there only one shoe? Bin it. No questions." |
| P1_Bin_04 | `Trash.Bin` | | | | "Confetti. Who approved a confetti cannon? ...I approved the confetti cannon." |
| P1_Bin_05 | `Trash.Bin` | | | | "Party hats in the trash. As is tradition." |
| P1_Swish_01 | `Trash.Swish` | 1 | 35 | Trash thrown into a bin | "SWISH! Three points! The crowd goes mild!" |
| P1_Swish_02 | `Trash.Swish` | | | | "Nothing but bin! Somebody get this on camera!" |
| P1_Swish_03 | `Trash.Swish` | | | | "From downtown! I'm calling it a sport now." |
| P1_Miss_01 | `Trash.Miss` | 1 | 25 | Thrown trash hits something that isn't a bin | "And it misses. Like my last two films." |
| P1_Miss_02 | `Trash.Miss` | | | | "Brick! Walk it over like a normal person." |
| P1_BinWrong_01 | `BinWrongItem` | 2 | 100 (cd 8 s) | A set piece lands in a bin and is spat out | "That's a set piece, not garbage! The bin spat it out! Even the bin knows!" |
| P1_Dish_01 | `Dish.Washed` | 1 | 30 | A dish washed in the sink | "Washing dishes. On a film set. The most realistic thing we've shot all year." |
| P1_Dish_02 | `Dish.Washed` | | | | "Scrub it like it owes you money." |
| P1_Dish_03 | `Dish.Washed` | | | | "Clean cup! Fake coffee tastes better in a clean cup." |
| P1_Sweep_01 | `Sweep.Stroke` | 1 | 5 | Fallback after a bin / wash when nothing else was said | "Sweep like nobody's watching. I'm watching. Sweep anyway." |
| P1_Sweep_02 | `Sweep.Stroke` | | | | "Every kernel of popcorn is a tiny insult to cinema." |
| P1_Sweep_03 | `Sweep.Stroke` | | | | "Popcorn. On a FILM set. The irony is crushing me." |
| P1_SweepDone_01 | `Sweep.Cleared` | 2 | 100 | The last trash item is binned | "That pile is gone! Like it never happened. Like my divorce." |
| P1_SweepDone_02 | `Sweep.Cleared` | | | | "Clean floor! I can see my reflection. I look tired." |

There is no sweeping mechanic; the `Sweep.*` lines still mention popcorn.

### 1.10 Prop-kind lines (lights, sockets, books, pillows, camera)
The original cable / light-aim / reel / costume / lens jobs were not built; these pools fire for the matching prop kinds instead.

| ID | Pool | Pri | % | Cooldown | Trigger | Line |
|---|---|---|---|---|---|---|
| P1_Light_01 | `Light.Done` | 1 | 100 | 15 s | A lamp put back (it lights up), or any light switched on while the power is on | "Look at that glow. That's called motivated lighting. I motivated it. With yelling." |
| P1_Light_02 | `Light.Done` | | | | | "Right on the X! A gaffer somewhere just felt a disturbance in the force." |
| P1_Cable_02 | `Cable.Placed` | 1 | 100 | 20 s | A socket put back | "That cable had more knots than this script." |
| P1_ReelDone_01 | `Reel.Done` | 2 | 100 | | Every book is back | "All in order. That's the first thing on this production that's been in order." |
| P1_Costume_01 | `Costume.Hung` | 1 | 30 | | A pillow / curtain put back | "Careful! That costs more than your car. It costs more than MY car." |
| P1_Costume_02 | `Costume.Hung` | | | | | "That's been through a lot. Mostly Dash." |
| P1_CostumeDone_01 | `Costume.Done` | 2 | 100 | | Every pillow / curtain is back | "Wardrobe is happy! Wardrobe is never happy! What did you do?" |
| P1_Lens_01 | `Lens.Done` | 2 | 100 | | The film camera is back on its mark | "Crystal clear! Now I can see all my mistakes in high definition." |

### 1.11 Progress milestones (`Progress.*`, Pri 2)
% = jobs done / total jobs (100, incl. puddles). Only the highest milestone reached is said; `Progress.99` fires when exactly one job is left.

| ID | % | Line |
|---|---|---|
| P1_Prog_10 | 10 | "Ten percent. It's a start. A sad start, but a start." |
| P1_Prog_25 | 25 | "A quarter done! I can see the floor. I forgot we had a floor." |
| P1_Prog_50 | 50 | "Halfway! I've stopped hyperventilating. Mostly." |
| P1_Prog_75 | 75 | "Seventy-five percent! I'm starting to believe in things again." |
| P1_Prog_90 | 90 | "Ninety percent! The last bits are hiding. I can feel them hiding." |
| P1_Prog_99 | 1 left | "One thing left. ONE. It's mocking us. Find it." |

### 1.12 Idle and hints

| ID | Pool | Pri | % | Trigger | Line |
|---|---|---|---|---|---|
| P1_Idle_01 | `Idle` | 1 | 100 | 40 s with nothing put back and nothing in hand, then every 60 s | "Hello? Rookie? Is this a dramatic pause?" |
| P1_Idle_02 | `Idle` | | | | "You're standing still. Are you thinking? Think faster." |
| P1_Idle_03 | `Idle` | | | | "The mess is still here. It's patient. I'm not." |
| P1_Idle_04 | `Idle` | | | `{0}` = hint key | "Lost? Press {0}. That's what it's there for. And to cover a hole." |
| P1_Idle_05 | `Idle` | | | | "Did you fall asleep standing up? Save that skill for a night shoot." |
| P1_Hint_01 | `Hint` | 2 | 70 | Player presses Hint. `{0}` = hinted prop's name | "The {0}! Right there! I can feel it in my tear ducts." |
| P1_Hint_02 | `Hint` | | | | "Follow the arrow, Rookie. The {0} misses its home." |
| P1_Hint_03 | `Hint` | | | | "That {0} is hiding. Props always are." |
| P1_Hint_04 | `Hint` | | | | "Go get the {0}. Everyone needs love. Especially the {0}." |

### 1.13 Countdown (time trial only)

| ID | Pool | Pri | Trigger | Line |
|---|---|---|---|---|
| P1_Time_5 | `Time.5Min` | 2 | 5:00 left (only if the limit is over 7 min) | "Five minutes, Rookie! The actors are in the parking lot. I can hear Dash's car. It sounds nervous." |
| P1_Time_1 | `Time.1Min` | 2 | 1:00 left (only if the limit is over 2 min) | "ONE MINUTE! Move like a producer is chasing you!" |
| P1_Time_Up | `Time.Up` | 3 | Time ran out (clears the queue) | "TIME! ...The actors are here. Act natural. Pretend the mess is a choice." |

### 1.14 Walkie-talkie story drips (`Story.20/40/60/80`, Pri 3, one-shot chains)
Played when progress passes 20 / 40 / 60 / 80%, one chain per placement, in order. Walkie lines show the speaker as "JUNIPER (walkie)" / "DASH (walkie)".

| ID | % gate | Speaker | Line |
|---|---|---|---|
| P1_Story20_1 | 20 | Walkie_Juniper | "Marlowe, it's Juniper. Is HE there yet?" |
| P1_Story20_2 | | Marlowe | "Who, Dash? Not yet." |
| P1_Story20_3 | | Walkie_Juniper | "Good. Tell me when he gets there, so I can be later." |
| P1_Story40_1 | 40 | Walkie_Dash | "Marlowe, buddy! Is Juniper on set? Asking for a friend. The friend is me." |
| P1_Story40_2 | | Marlowe | "She's not here, Dash." |
| P1_Story40_3 | | Walkie_Dash | "Cool. Cool cool cool. Totally cool. I'm cool." |
| P1_Story60_1 | 60 | Marlowe | "Between us, Rookie... Dash and Juniper used to date. In real life. It ended badly." |
| P1_Story60_2 | | Marlowe | "There was a fruit basket. And a lake. Nobody talks about the lake." |
| P1_Story80_1 | 80 | Marlowe | "In an hour they have to fall in love on camera. I'm going to need the rain machine. And a miracle. Mostly the rain machine." |
| P1_Story80_2 | | Walkie_Juniper | "Marlowe, I'm parking. Is the set ready?" |
| P1_Story80_3 | | Marlowe | "...Define 'ready'." |

### 1.15 Phase 1 complete (`Complete`, Pri 3 chain)
When the set is clean. Clears the queue first.

| ID | Line |
|---|---|
| P1_Done_1 | "Is that... is it... it's clean. The set is clean." |
| P1_Done_2 | "Look at it. Look at the LIGHT. I think I'm going to cry. I am crying. It's the megaphone. It's dusty." |
| P1_Done_3 | "PLACES, EVERYONE! Get the actors! Get the rain machine! Get me a tissue!" |

### 1.16 Planned / cut
Written in the original plan but not in the build (the triggers don't exist): Notebook / ability tutorials, group-complete and zone-complete lines, ability lines, golden reels, clapperboard, zone-specific hints, jump spam / standing on a table / staring at Marlowe / prop lost, the "You're FIRED!" exit line, and the star-based end-screen quotes (the end screen has no stars or quote).

---

## 2. Phase 2 - Scene 42: The Cafe

**As built.** Everything here is hard-coded in C++. The script is in `ULTTSceneData::FillWithScene42` (`Source/CleanCutAction/Shoot/LTTSceneData.cpp`). The reactions, in-frame, CUT and end-screen lines are in `LTTSceneDirector.cpp`, and the station barks are in `LTTEffectStation.cpp`. No `DA_Scene42` asset or `DT_DirectorLines` rows exist. The director builds Scene 42 at runtime when its `SceneData` is empty. Lines have no voice-over and show as subtitles, while Marlowe's cue calls and reactions show as megaphone barks.

Speaker labels: `LEO (Dash)` / `CLARA (Juniper)` in character, `Dash (off-script)` / `Juniper (off-script)` when they drop the script, and `MARLOWE`. Stage directions are written into the subtitle text in parentheses. Nothing acts them out yet, because the montage maps are empty.

### 2.1 Pre-roll (`PreRoll`, `SlateDone`)

| # | Speaker | Line |
|---|---|---|
| PreRoll 1 | Marlowe | "Rookie! The set looks incredible. The actors are here. The actors are... not speaking to each other. Great. Perfect. Fine." |
| PreRoll 2 | Marlowe | "Listen up! Scene forty-two. The cafe. Leo and Clara's first date. Rookie, you're on effects." |
| PreRoll 3 | Marlowe | "When I call a cue, you run to that station and make it happen. Rain, traffic, thunder. You're the weather now." |
| PreRoll 4 | Marlowe | "And see that glowing cone? That's what the camera sees. If you walk into it, you're in my movie. You are NOT in my movie." |
| PreRoll 5 | Marlowe | "First, slate the shot. Stand on the mark, clap the board. The one time you're allowed in frame. Enjoy it." |
| SlateDone | Marlowe | "Scene forty-two, take one! Now get OUT of my shot! ...And... ACTION!" |

Cut: the pre-shoot credits line (`P2_PreShoot_1`). There is no notebook or credits system.

### 2.2 Beat-by-beat script

Format: **Pre** = PreDialogue, **Cue** = Marlowe's call (window in seconds before travel time), **Post** = result lines (P = Perfect, used for Good too since no beat has a `PostGood` list; L = Late; M = Miss), **Then** = `PostAlways`. *Italics* = off-script. **(opt)** beats are skipped by `bSkipOptionalBeats`. **(speed)** = also graded by how fast it was done. `→ mark` = someone walks to that mark when the line starts.

Act titles: A "ACT I - THE ARRIVAL", B "ACT II - THE TERRACE", C "ACT III - THE RAIN". Each act's first beat is a checkpoint (camera anchor, start marks, CUT restart point).

---

#### ACT I - THE ARRIVAL (start marks: Dash `A_LeoTable`, Juniper `A_ClaraDoor`)

**A1_Traffic · TRAFFIC** (9 s)
- Pre: LEO: "Okay. Be cool. You're a cool guy who drinks coffee. Cool guys drink coffee."
- Cue: "TRAFFIC! Give me a city! Push that car!"
- Post P: LEO: "Ah, the city. So busy. So many... cars. That's a normal thing to say."
- Post L: LEO: "Ah, the city. Eventually busy."
- Post M: LEO: "Huh. Quiet street." *DASH: "(whisper) Was there supposed to be a car?"* MARLOWE: "THERE WAS SUPPOSED TO BE A CAR."

**A2_Door · DOORBELL** (7 s, opt)
- Pre: MARLOWE: "Juniper's walking in. Doorbell on her hand on the handle, Rookie."
- Cue: "DOORBELL! When she opens it!"
- Post P: CLARA: "Leo? Hi! You look exactly like your profile picture. That never happens."
- Post L: CLARA: "Leo? Hi! The door... rang late. Is that normal here?"
- Post M: *JUNIPER: "(opens the door twice, looks at the camera) Should I knock?"*
- Then: LEO: "I use a very honest camera." CLARA: "Clearly." (→ `A_ClaraTable`)

**A3_Steam · STEAM** (8 s)
- Pre: LEO: "I got you a cappuccino. I didn't know how you take it, so I got it... the way they make it." (Juniper → `A_ClaraTable`)
- Cue: "STEAM! I want romance in the air! I want the audience to SMELL the espresso!"
- Post P: CLARA: "This place is lovely. It smells like burnt coffee and second chances." *DASH: "(whisper) That's not the line."* *JUNIPER: "(whisper) It's my line now."* MARLOWE: "(whisper) Keep rolling, keep rolling..."
- Post L: CLARA: "Oh. There's the steam. Very... punctual."
- Post M: CLARA: "It smells like... nothing. Like a set." MARLOWE: "IT'S A CAFE, IT SHOULD SMELL LIKE A CAFE."

**A4_Napkins · NAPKINS** (8 s, speed)
- Pre: LEO: "So, what do you do for- (knocks his cup over) Oh no. Oh no no no." [Startled]
- Cue: "PROPS! Napkins! Toss them to Dash! Underhand! He can't catch overhand!"
- Post P: LEO: "Sorry. I'm a little nervous. I ruin things when I'm nervous." [Catch] *JUNIPER: "(quietly) Yeah. You do."* *DASH: "...Are we doing this now? On camera?"*
- Post L: LEO: "Got them. Eventually. Like everything in my life." *JUNIPER: "Yeah. Eventually."*
- Post M: *DASH: "OW. Who threw that?"* [Startled] *JUNIPER: "(laughing despite herself) The universe."* [Laugh] *DASH: "...Are we doing this now? On camera?"*

**A5_Thunder · THUNDER** (10 s)
- Pre: *JUNIPER: "You want to do this? Fine. Three years, Dash. Three years and you ended it with a fruit basket."* [Angry] *DASH: "It was a NICE fruit basket!"*
- Cue: "They're off-script! I don't care! THUNDER! Punctuate the drama! Make the gods jealous!"
- Post P: *JUNIPER: "I need some air."* [StormOff → `B_ClaraRail`] *DASH: "Clara, wait! - Jun, WAIT!"*
- Post L: *JUNIPER: "I need some air. ...That was thunder, right? Okay. Storming off now."* [StormOff → `B_ClaraRail`]
- Post M: *JUNIPER: "I need some air."* *DASH: "Was that thunder or your stomach?"* *JUNIPER: "STORMING OFF NOW."* [StormOff → `B_ClaraRail`]
- Then: MARLOWE: "They're going UPSTAIRS! Camera, follow them! Crane up! Rookie, GO GO GO, to the second floor! Take the back stairs, the front ones are in the shot!" (Dash → `B_LeoRail`)

---

#### ACT II - THE TERRACE (start marks: Dash `B_LeoRail`, Juniper `B_ClaraRail`)

**B1_Sunset · SUNSET** (9 s, speed)
- Pre: *JUNIPER: "You know what the worst part is? You never even said sorry."* *DASH: "I sent a fruit basket!"* *JUNIPER: "You threw it in a LAKE!"* [Angry] *DASH: "It was a fruit basket ON a lake! It was a boat basket! It was ROMANTIC!"*
- Cue: "SUNSET! Give me orange! Give me longing! Give me magic hour!"
- Post P: MARLOWE: "(whisper) Oh, that light. That's the light. Nobody move." *JUNIPER: "(softer) It sank, Dash. Like, immediately. The ducks ate the grapes."*
- Post L: *JUNIPER: "...It sank, Dash. The ducks ate the grapes."*
- Post M: *JUNIPER: "It sank, Dash."* *DASH: "Can we at least get some nice lighting for this?"*

**B2_Wind · WIND** (9 s, opt)
- Pre: *DASH: "I wanted it to be big. Memorable. You deserved memorable."*
- Cue: "WIND! Her hair needs to move like a shampoo commercial! Crank it!"
- Post P: *JUNIPER: "I didn't want big, Dash. I wanted small. A text. A sorry. One small thing."*
- Post L: *JUNIPER: "...I wanted small, Dash. One small thing."*
- Post M: *JUNIPER: "(hair perfectly still) I wanted small, Dash."* *DASH: "Your hair is very calm about this."*

**B3_Boom · BOOM MIC** (9 s)
- Pre: MARLOWE: "(whispering through the megaphone) Quiet moment. Quiet moment. Boom mic, Rookie. Over them. Don't you DARE dip into frame."
- Cue: "BOOM! Get the mic up! Gently!"
- Post P: *DASH: "(quietly) I'm sorry, Jun. Not Clara. You. I'm sorry."* *JUNIPER: "...Say that again. I don't think the mic got it."* *DASH: "It got it."* MARLOWE: "(sniffling) It got it."
- Post L: *DASH: "I'm sorry, Jun."* *JUNIPER: "What?"* *DASH: "I'm SORRY."* *JUNIPER: "...Okay. I heard that one."*
- Post M: *DASH: "I'm sorr- OW."* [Startled] *JUNIPER: "(laughing) Even the microphone thinks you're late."* [Laugh]

**B4_Rain · RAIN** (12 s)
- Pre: *JUNIPER: "You can't just say sorry on a terrace at sunset and expect-"* MARLOWE: "NOW! Rookie, the rain rig! FULL BLAST! Make it POUR!"
- Cue: "RAIN! RAIN! RAIIIIN!"
- Post P: *JUNIPER: "(soaked, then laughing) Of course it's raining. Of COURSE it is."* [Laugh] *DASH: "Do you want my... I don't have an umbrella. Hang on."*
- Post L: *JUNIPER: "...And NOW it's raining. Great timing. Very you."*
- Post M: *JUNIPER: "Is it... supposed to be raining?"* MARLOWE: "IT IS SUPPOSED TO BE POURING."

---

#### ACT III - THE RAIN (start marks: Dash `C_Center_L`, Juniper `C_Center_R`; after a CUT, Sunset and Rain are switched back on)

**C1_Umbrella · UMBRELLA** (8 s, speed)
- Pre: MARLOWE: "He needs an umbrella! The prop table upstairs!" (Dash → `C_Center_L`)
- Cue: "UMBRELLA! To Dash! Throw it! NOW!"
- Post P: *DASH: "Umbrella?"* [Catch] (Juniper → `C_Center_R`) *JUNIPER: "No. (tosses it over the railing) Let it rain."*
- Post L: *DASH: "(opens it upside down) ...Umbrella?"* (Juniper → `C_Center_R`) *JUNIPER: "No. Let it rain."*
- Post M: *DASH: "OW. Why does everything keep hitting me today?"* [Startled] (Juniper → `C_Center_R`) *JUNIPER: "Karma. Let it rain."*

**C2_Roses · ROSES** (8 s, speed)
- Pre: *DASH: "Okay. Small gesture. Hang on."* MARLOWE: "Is he- he's improvising. Roses! Give him the roses!"
- Cue: "ROSES! Toss them to Dash! He's about to do something stupid and wonderful!"
- Post P: *DASH: "I stole these from the props table. Small gesture."* [Catch] *JUNIPER: "...That's a very small gesture."* *DASH: "Is it enough?"* *JUNIPER: "(smiling) It's a start."*
- Post L: *DASH: "I... found these. Small gesture."* *JUNIPER: "They're a little crushed."* *DASH: "So am I."* *JUNIPER: "...It's a start."*
- Post M: *DASH: "(holding out a single petal) Small gesture?"* *JUNIPER: "(laughing) The smallest. It's a start."* [Laugh]

**C3_Moonlight · MOONLIGHT** (8 s, speed, opt)
- Pre: MARLOWE: "(barely breathing) Moonlight. Soft. Romantic. Don't you dare touch anything else, Rookie."
- Cue: "MOONLIGHT! Blue gel! Gently!"
- Post P: *JUNIPER: "Remember the lake?"* *DASH: "I'll never forget the lake."* *JUNIPER: "The ducks were happy, at least."*
- Post L: *JUNIPER: "Remember the lake?"* *DASH: "The ducks were happy."*
- Post M: *DASH: "Is the moon... seasick?"* *JUNIPER: "(giggling) Don't make me laugh, I'm trying to have a moment."* [Laugh]

**C4_FinalThunder · THUNDER** (9 s)
- Pre: MARLOWE: "(whisper) Don't... nobody breathe... Rookie, one last thunder. Make it count. When they lean in."
- Cue: "...and THUNDER!"
- Post P: *JUNIPER: "(they lean in, in silhouette against the flash)"* [LeanIn]
- Post L: *DASH: "(leaning in, muffled) Are we waiting for thunder?"* [LeanIn]
- Post M: *JUNIPER: "(they both look at the camera) ...We're doing it anyway."* [LeanIn]
- Then → finale.

`[Name]` = the montage the line asks for (`Laugh`, `Angry`, `StormOff`, `Catch`, `Startled`, `LeanIn`). `StormOff` also makes the walker run.

Cut from the plan: the grade variants (over-steam fog, wrong-gel green Dash, over-wind scarf, boom-in-shot bump), since each beat has one Miss list. Also cut: Leo/Clara sitting, the umbrella being taken and thrown on screen, and the silhouette / black-frame finale shot.

### 2.3 Finale & epilogue (`Wrap`)

Plays after the last beat, with `SW_applause` and frame checks off. The player can still move.

| # | Speaker | Line |
|---|---|---|
| 1 | Marlowe | "...Cut." |
| 2 | Marlowe | "CUT! That's a wrap! THAT'S A WRAP!" |
| 3 | *Juniper* | "We're going to need a few retakes on that." |
| 4 | *Dash* | "At least twelve." |
| 5 | Marlowe | "No retakes! That WAS the take! That's the Director's Cut!" |
| 6 | Marlowe | "Rookie. You made it rain. Here. Crew jacket. It's a little damp. Same time tomorrow, Assistant Director." |

### 2.4 Marlowe's cue reactions (`ALTTSceneDirector::GetResultBark`)

Shown as a megaphone bark while the Post lines play, **only if those lines have no Marlowe line**, and never on the last beat (Final Thunder).

| Cue | Perfect | Good | Late | Miss |
|---|---|---|---|---|
| Traffic | "Gorgeous traffic! I believe in this city!" | "Nice. A little jerky, like real traffic." | "Late traffic. Also realistic." | "Where's my CAR?" |
| Door | "Ding! Perfect timing!" | "Good ding." | "Late ding. Still a ding." | "That was a ding for a ghost." |
| Steam | "Look at that steam! I can smell it from here!" | "Nice puff." | "A late puff." | "No steam?!" |
| Napkins | "Perfect toss! He even caught it!" | "Caught! Barely! Good!" | "He had to bend down. Undignified, but fine." | "Not his FACE!" |
| Thunder | "THAT'S thunder!" | "Good rumble!" | "A little late, like a real storm." | "That was a burp. That was a weather burp." |
| Sunset | "Magic hour! I'm weeping!" | "Lovely orange." | "The sun took its time." | "Where's my sunset?!" |
| Wind | "Shampoo commercial! Perfect!" | "Nice breeze." | "Breeze arrived late." | "Her hair is not moving!" |
| Boom | "Clean audio! Clean frame! I love you!" | "Good sound." | "We'll fix it in post." | "BOOM IN SHOT!" |
| Rain | "LOOK AT THAT RAIN! I'm a GENIUS! You're a genius! We're all wet!" | "Rain! Real-looking rain!" | "Late rain! Still rain!" | "That's a drizzle! I asked for drama, not humidity!" |
| Umbrella | "Perfect throw!" | "Good catch, Dash!" | "Close enough!" | "Stop hitting my lead actor!" |
| Roses | "Rose delivery! Romantic!" | "Nice!" | "Slightly crushed. Like love." | "They're in the street!" |
| Moonlight | "Moonlight! Perfect!" | "Nice blue." | "The moon rose slowly." | "Is the moon SICK?" |

### 2.5 Other Phase 2 lines

| Trigger | Source | Line |
|---|---|---|
| In frame (random, 8 s cooldown) | `TickFrameCheck` | "Rookie, you're IN THE SHOT!" / "Get out of frame! You're not in this movie!" / "Why is there a Rookie in my romance?!" / "Duck! DUCK! Not a real duck! Get DOWN!" / "The audience will think you're the love interest! Move!" |
| Left a station early | `LeaveStation` | "Where are you going? The cue isn't done!" |
| Steam over-pressure | Station HoldRelease | "Too much! It's a cafe, not a sauna!" |
| Wrong gel | Station Choice | "<COLOR>?! WRONG GEL! WRONG GEL!" (ORANGE / BLUE / GREEN) |
| CUT (random) | `CallCut` | "CUT! CUT CUT CUT!" / "CUT! That was a disaster! A beautiful disaster, but a disaster!" / "CUT! Reset! Somebody get Dash a towel and me a sedative!" |
| Restart Act A | `RestartLines` | "From the top! Places! Dash, stop eating the prop croissant! ...And ACTION!" |
| Restart Act B | `RestartLines` | "From the terrace! Places! Juniper, you're furious. Dash, you're sorry. Rookie, you're fast. ACTION!" |
| Restart Act C | `RestartLines` | "From the rain! Everybody's wet, everybody's in love. ACTION!" |

Cut: the boom-in-shot bark, cue-idle nags and the Director's Patience line. Those mechanics don't exist.

### 2.6 Phase 2 end-screen quotes (`GetWrapQuote`, by stars)

| Stars | Line |
|---|---|
| 1 | "We got it. Somehow. Cinema is a miracle." |
| 2 | "That's a scene, Rookie. That's a real scene." |
| 3 | "That's the Director's Cut. Frame it. Frame YOU." |

---

## 3. Menu & Pause Quips (random, shown in small italic on the screens)

**Status: not in the build.** There is no main menu map (the game starts on the Phase 1 briefing), the pause menu (`ULTTPauseMenu`: title "CUT!", subtitle "Game paused") shows no quips, and there are no credits or locked Phase 2 button. Kept as unused copy.

**Main menu (Marlowe):**
- "Quiet on set! ...It's a menu. Nobody's on set. Carry on."
- "Every great film starts with somebody cleaning up after a party."
- "Press Start. Or don't. I'll be here. Yelling."
- "If you see a lake, don't ask."

**Pause (Marlowe):**
- "Taking five? Union rules. I respect it. I hate it, but I respect it."
- "Paused. The popcorn isn't going anywhere. That's the problem."
- "Hydrate, Rookie. Then get back out there."
- "The actors are waiting. Well, Dash is. Juniper is pretending not to."

**Locked Phase 2 button tooltip:** "Clean the set first, Rookie."

**Credits roll (jokey roles, after the real credits):**
- Rain ............ You
- Traffic ........... Also you
- Thunder ........... You again
- Fruit Basket ........ Deceased (Lake)
- The Ducks ......... Thanks for the grapes
- Craft Services ........ The real hero
