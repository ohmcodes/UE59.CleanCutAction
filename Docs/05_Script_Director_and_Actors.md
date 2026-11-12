# Script - Director Lines & Scene 42

Import everything in §1 and the cue/result lines in §2 into `DT_DirectorLines`; import the beat structure in §2.2 into `DA_Scene42`.

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

---

## 1. Phase 1 - Marlowe While You Clean

### 1.1 Intro (`Bark.P1.Intro`, Pri 3, one-shot chain)

| ID | Line |
|---|---|
| P1_Intro_1 | "Rookie! Welcome to the last day of shooting on *Clean! Cut! Action!*. Look around. Breathe it in. That smell? That's my career." |
| P1_Intro_2 | "Last night's wrap party got... enthusiastic. The actors arrive in one hour, and this set needs to be spotless." |
| P1_Intro_3 | "See those yellow tape marks? Every prop has a home. Put everything back exactly where it lives. Exactly. I measure with my soul." |
| P1_Intro_4 | "Start with that chair. Pick it up. Put it on its mark. I'll wait. I'm a patient man. That's a lie, but go." |

### 1.2 First-time tutorials (`Bark.P1.First.*`, Pri 3, one-shot)

| ID | Trigger | Line |
|---|---|---|
| P1_First_Pickup | Pickup | "Look at you, holding a chair. Your parents must be so proud." |
| P1_First_Place | Place | "Hear that? That's the sound of order. Burn it into your memory." |
| P1_First_Stack | Stack 2+ | "Stacking! Efficient. Reckless. I respect both." |
| P1_First_Throw | Throw | "Right-click to throw. Wait, why am I teaching you to throw props on my set?" |
| P1_First_Bin | Trash binned | "Trash goes in the bin. Revolutionary, I know. Write it down." |
| P1_First_Broom | Broom picked up | "That's a broom. Push it at the popcorn. It's basically filmmaking." |
| P1_First_RightIt | Right a table | "Hold to flip the table back up. Gently. That table has a stunt double, and it's busier than you." |
| P1_First_Light | Light aim start | "Aim the light at the X. The X is where the magic happens. Magic is expensive. Don't break it." |
| P1_First_Cable | Cable focus | "Follow the cable to the outlet. Like a detective, but useful." |
| P1_First_Reel | Reel picked up | "Film reels go on the shelf in order. One through five. Like the stages of grief." |
| P1_First_Costume | Costume picked up | "Costumes! Juniper's blues on the left, Dash's browns on the right. Dash only wears brown. Don't ask. It's a whole thing." |
| P1_First_Lens | Lens focus | "Wipe the lens! Someone licked it. I don't want to know who. I know who." |
| P1_First_Notebook | First credit earned | "A crew credit! Open your notebook with Tab. Every credit buys a little competence." |
| P1_First_Ability | First ability unlocked | "A new skill! Use it with the number keys. Next you'll want a chair with your name on it." |

### 1.3 Correct placement (`Bark.P1.Place.Correct`, 25%)

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

### 1.4 Streaks (`Bark.P1.Streak.*`, Pri 2)

| ID | Trigger | Line |
|---|---|---|
| P1_Streak_3 | 3 | "Ooh, a combo! Who ARE you?" |
| P1_Streak_5 | 5 | "Five in a row! Somebody get this Rookie an agent!" |
| P1_Streak_10 | 10 | "TEN! I'm getting emotional. Don't look at me. Keep going. Don't look at me." |

### 1.5 Wrong spot / dropped (`Bark.P1.Place.Wrong`, 30%, cooldown 15 s)

| ID | Line |
|---|---|
| P1_Wrong_01 | "That's not its home. That's its vacation spot." |
| P1_Wrong_02 | "Close isn't a mark. Close is how we get sued." |
| P1_Wrong_03 | "Interesting choice. Wrong, but interesting." |
| P1_Wrong_04 | "That prop looks lost and confused. Much like our lead actor." |
| P1_Wrong_05 | "If I can see it from here, it's wrong. I can see it from here." |
| P1_Wrong_06 | "Floor is not a mark. The floor is the enemy." |

### 1.6 Crashes & throwing

| ID | Trigger | % | Line |
|---|---|---|---|
| P1_Crash_01 | `Crash` | 50 | "Was that crash part of the plan?" |
| P1_Crash_02 | `Crash` | 50 | "I heard that. The insurance company heard that." |
| P1_Crash_03 | `Crash` | 50 | "Gently! Everything on this set is rented!" |
| P1_Crash_04 | `Crash` | 50 | "Ah. The sound of my deposit leaving forever." |
| P1_Throw_01 | `Throw` | 15 | "Are you throwing props? On MY set? ...Do it again, that was cool." |
| P1_Throw_02 | `Throw` | 15 | "Nice arc. Terrible manners." |
| P1_Throw_03 | `Throw` | 15 | "Props are not frisbees. Except the reels. Apparently." |
| P1_TrickShot_01 | `TrickShot` | 100 | "Did that just land on its mark? From THERE? I'm putting that in the blooper reel and the trailer." |
| P1_TrickShot_02 | `TrickShot` | 100 | "Trick shot! You're wasted on tidying. You should be in stunts." |

### 1.7 Trash

| ID | Trigger | % | Line |
|---|---|---|---|
| P1_Bin_01 | `Trash.Bin` | 15 | "Into the bin. Goodbye, pizza. You were a good pizza." |
| P1_Bin_02 | `Trash.Bin` | 15 | "Somebody ordered eleven pizzas and ate four slices. I'm not naming names. Dash." |
| P1_Bin_03 | `Trash.Bin` | 15 | "Is that a shoe? Why is there only one shoe? Bin it. No questions." |
| P1_Bin_04 | `Trash.Bin` | 15 | "Confetti. Who approved a confetti cannon? ...I approved the confetti cannon." |
| P1_Bin_05 | `Trash.Bin` | 15 | "Party hats in the trash. As is tradition." |
| P1_Swish_01 | `Trash.Swish` | 35 | "SWISH! Three points! The crowd goes mild!" |
| P1_Swish_02 | `Trash.Swish` | 35 | "Nothing but bin! Somebody get this on camera!" |
| P1_Swish_03 | `Trash.Swish` | 35 | "From downtown! I'm calling it a sport now." |
| P1_Miss_01 | `Trash.Miss` | 25 | "And it misses. Like my last two films." |
| P1_Miss_02 | `Trash.Miss` | 25 | "Brick! Walk it over like a normal person." |
| P1_BinWrong_01 | `BinWrongItem` | 100 | "That's a set piece, not garbage! The bin spat it out! Even the bin knows!" |

### 1.8 Sweeping

| ID | Trigger | % | Line |
|---|---|---|---|
| P1_Sweep_01 | `Sweep.Stroke` | 5 | "Sweep like nobody's watching. I'm watching. Sweep anyway." |
| P1_Sweep_02 | `Sweep.Stroke` | 5 | "Every kernel of popcorn is a tiny insult to cinema." |
| P1_Sweep_03 | `Sweep.Stroke` | 5 | "Popcorn. On a FILM set. The irony is crushing me." |
| P1_SweepDone_01 | `Sweep.Cleared` | 40 | "That pile is gone! Like it never happened. Like my divorce." |
| P1_SweepDone_02 | `Sweep.Cleared` | 40 | "Clean floor! I can see my reflection. I look tired." |

### 1.9 Cables, lights, reels, costumes, lens

| ID | Trigger | Line |
|---|---|---|
| P1_Cable_01 | `Cable.Done` | "Power restored! I feel a tingle. Either it's art, or I'm being electrocuted." |
| P1_Cable_02 | `Cable.Done` | "That cable had more knots than this script." |
| P1_Light_01 | `Light.Done` | "Look at that glow. That's called motivated lighting. I motivated it. With yelling." |
| P1_Light_02 | `Light.Done` | "Right on the X! A gaffer somewhere just felt a disturbance in the force." |
| P1_ReelWrong_01 | `Reel.Wrong` | "Reel four before reel three? Are you EDITING my film?" |
| P1_ReelWrong_02 | `Reel.Wrong` | "Numbers, Rookie. They go up. That's the whole trick." |
| P1_ReelDone_01 | `Reel.Done` | "All in order. That's the first thing on this production that's been in order." |
| P1_Costume_01 | `Costume.Hung` (30%) | "Careful! That dress costs more than your car. It costs more than MY car." |
| P1_Costume_02 | `Costume.Hung` (30%) | "That jacket has been through a lot. Mostly Dash." |
| P1_CostumeDone_01 | `Costume.Done` | "Wardrobe is happy! Wardrobe is never happy! What did you do?" |
| P1_Lens_01 | `Lens.Done` | "Crystal clear! Now I can see all my mistakes in high definition." |

### 1.10 Group complete (`Bark.P1.Group.Done`, 30%)

| ID | Line |
|---|---|
| P1_Group_01 | "A whole table, done! It looks like a place where people fall in love. Or at least split a muffin." |
| P1_Group_02 | "Another set piece locked in. You hear that ding? That's a crew credit. Spend it wisely. Or recklessly. I'm not your dad." |
| P1_Group_03 | "That corner is ready for its close-up." |

### 1.11 Zone complete (`Bark.P1.Zone.*`, Pri 2)

| ID | Zone | Line |
|---|---|---|
| P1_Zone_Entrance | Entrance | "The entrance is clean! Customers could theoretically walk in and buy fake coffee!" |
| P1_Zone_Seating | Seating | "The tables are set! It looks like a real cafe. Better than real. Real cafes have customers." |
| P1_Zone_Kitchen | Kitchen | "Spotless kitchen. Don't cook in it. It's plywood." |
| P1_Zone_Stairs | Stairs | "The stairs are clear! Nobody trips on camera today. Probably." |
| P1_Zone_Terrace | Terrace | "The terrace! That's where the big scene happens. Don't tell the actors. They'll get nervous." |
| P1_Zone_Crew | Crew | "My crew area! My chair! My little monitor! I could cry. I won't. I'm working." |
| P1_Zone_Craft | Craft | "Craft services restored. Snacks are back. Morale just went up forty percent." |
| P1_Zone_Generic | fallback | "Lights up on another zone! That's the sound of me not getting fired." |

### 1.12 Progress milestones (`Bark.P1.Progress.*`, Pri 2)

| ID | % | Line |
|---|---|---|
| P1_Prog_10 | 10 | "Ten percent. It's a start. A sad start, but a start." |
| P1_Prog_25 | 25 | "A quarter done! I can see the floor. I forgot we had a floor." |
| P1_Prog_50 | 50 | "Halfway! I've stopped hyperventilating. Mostly." |
| P1_Prog_75 | 75 | "Seventy-five percent! I'm starting to believe in things again." |
| P1_Prog_90 | 90 | "Ninety percent! The last bits are hiding. I can feel them hiding." |
| P1_Prog_99 | 99 | "One thing left. ONE. It's mocking us. Find it." |

### 1.13 Abilities

| ID | Trigger | Line |
|---|---|---|
| P1_Ab_Eye_Unlock | `Ability.DirectorsEye.Unlock` | "Director's Eye! Now you see what I see. Mostly mess. And regret." |
| P1_Ab_Eye_Use | `Ability.DirectorsEye.Use` (20%) | "Scanning! Very cinematic. Very Hitchcock." |
| P1_Ab_Grab_Unlock | `Ability.GrabBag.Unlock` | "Grab Bag! Summon the props to your arms, like a wizard. A unionized wizard." |
| P1_Ab_Grab_Use | `Ability.GrabBag.Use` (20%) | "Come to papa, little cups!" |
| P1_Ab_Set_Unlock | `Ability.SetDresser.Unlock` | "Set Dresser's Touch! Everything in your arms flies home. Show-off." |
| P1_Ab_Set_Use | `Ability.SetDresser.Use` (20%) | "Look at them go! It's like a musical number for furniture." |
| P1_Ab_Stack | `Ability.StackEm.Unlock` | "Carrying more? Ambitious. Don't drop the vase. Especially that vase." |
| P1_Ab_Sneakers | `Ability.Sneakers.Unlock` | "Comfy sneakers! Your feet will thank you. Your feet are the only ones thanking you today." |
| P1_Ab_Cooldown | `Ability.Cooldown` (30%, cd 30 s) | "It's recharging! Even magic needs a coffee break." |

### 1.14 Golden reels (Pri 2)

| ID | Trigger | Line |
|---|---|---|
| P1_Gold_1 | `Reel.Gold.1` | "A golden reel! That's from my first film. My good film. You can run now. Sprint with Shift!" |
| P1_Gold_2 | `Reel.Gold.2` | "Another one! I hid those during the party. Don't ask why. I was emotional. Carry one more thing!" |
| P1_Gold_3 | `Reel.Gold.3` | "All three golden reels! You're a better scavenger than my last producer. Faster skills, Rookie!" |

### 1.15 Clapperboard

| ID | Trigger | Line |
|---|---|---|
| P1_Clap_Found | `Clapper.Found` (Pri 3) | "My CLAPPERBOARD! In the fridge? Who puts a clapperboard in the fridge? ...It's cold. Why is it cold? Put it on the stand by my chair." |
| P1_Clap_Hint | Hint when only clapper remains | "Where's my clapperboard? Someone hid it somewhere stupid. Look somewhere stupid." |

### 1.16 Idle & hints

| ID | Trigger | Line |
|---|---|---|
| P1_Idle_01 | `Idle` | "Hello? Rookie? Is this a dramatic pause?" |
| P1_Idle_02 | `Idle` | "You're standing still. Are you thinking? Think faster." |
| P1_Idle_03 | `Idle` | "The mess is still here. It's patient. I'm not." |
| P1_Idle_04 | `Idle` | "Lost? There's a map on the wall by the stairs. It's there for you. And to cover a hole." |
| P1_Idle_05 | `Idle` | "Did you fall asleep standing up? Save that skill for a night shoot." |
| P1_Hint_Entrance | `Hint.Entrance` | "Something's still off by the entrance. I can feel it in my tear ducts." |
| P1_Hint_Seating | `Hint.Seating` | "One of the tables isn't finished. Check the chairs. Chairs are sneaky." |
| P1_Hint_Kitchen | `Hint.Kitchen` | "The kitchen's hiding something. Kitchens always are." |
| P1_Hint_Stairs | `Hint.Stairs` | "Check the stairs, Rookie. Both of them. Yes, there are two. You're welcome." |
| P1_Hint_Terrace | `Hint.Terrace` | "Upstairs! The terrace still needs love. Everyone needs love. Especially terraces." |
| P1_Hint_Crew | `Hint.Crew` | "My crew area isn't done. My OWN area. The disrespect." |
| P1_Hint_Craft | `Hint.Craft` | "Craft services still looks like a crime scene. A delicious crime scene." |

### 1.17 Silly player behavior

| ID | Trigger | Line |
|---|---|---|
| P1_Jump_01 | `JumpSpam` | "Stop bouncing! This is a cafe, not a trampoline set." |
| P1_Jump_02 | `JumpSpam` | "I love the energy. The floorboards don't." |
| P1_Table_01 | `OnTable` | "Get off the table! Tables are for acting!" |
| P1_Stare_01 | `StareAtMarlowe` | "Why are you looking at me? Look at the mess!" |
| P1_Stare_02 | `StareAtMarlowe` | "Yes, it's me. The visionary. Back to work." |
| P1_Lost_01 | `PropLost` (40%) | "Did that just fall through reality? It's back. Don't do that again." |

### 1.18 Walkie-talkie story drips (Pri 3, one-shot chains)

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

### 1.19 Exit door / fired (`Bark.P1.Fired`)

| ID | Line |
|---|---|
| P1_Fired_01 | "Where are you going? Lunch? There's no lunch! You're- you know what? You're FIRED!" |

### 1.20 Phase 1 complete (`Bark.P1.Complete`, Pri 3 chain)

| ID | Line |
|---|---|
| P1_Done_1 | "Is that... is it... it's clean. The set is clean." |
| P1_Done_2 | "Look at it. Look at the LIGHT. I think I'm going to cry. I am crying. It's the megaphone. It's dusty." |
| P1_Done_3 | "PLACES, EVERYONE! Get the actors! Get the rain machine! Get me a tissue!" |

### 1.21 Phase 1 end-screen quotes (by stars)

| Stars | Line |
|---|---|
| 1 | "It's clean. Eventually. That counts. Legally." |
| 2 | "Solid work, Rookie. I only had two minor heart attacks." |
| 3 | "You're the best Rookie I've ever yelled at. And I yell at a LOT of Rookies." |

---

## 2. Phase 2 - Scene 42: The Cafe

### 2.1 Arrival & pre-roll

| ID | Speaker | Line |
|---|---|---|
| P2_Arrive_1 | Marlowe | "Rookie! The set looks incredible. The actors are here. The actors are... not speaking to each other. Great. Perfect. Fine." |
| P2_PreShoot_1 | Marlowe | "Last chance to spend your crew credits. Once we roll, we roll." |
| P2_PreRoll_1 | Marlowe | "Listen up! Scene forty-two. The cafe. Leo and Clara's first date. Rookie, you're on effects." |
| P2_PreRoll_2 | Marlowe | "When I call a cue, you run to that station and make it happen. Rain, traffic, thunder. You're the weather now." |
| P2_PreRoll_3 | Marlowe | "And see that glowing cone? That's what the camera sees. If you walk into it, you're in my movie. You are NOT in my movie." |
| P2_PreRoll_4 | Marlowe | "First, slate the shot. Stand on the mark, clap the board. The one time you're allowed in frame. Enjoy it." |
| P2_Slate_Done | Marlowe | "Scene forty-two, take one! Now get OUT of my shot! ...And... ACTION!" |

### 2.2 Beat-by-beat script

Format: **Pre** = PreDialogue, **Cue** = Marlowe's call, **Post** = result lines (P/G = Perfect/Good, L = Late, M = Miss). *Italics* = off-script. Beats marked (opt) can be cut.

---

#### ACT I - THE ARRIVAL (camera position A, checkpoint)

**A1 · TRAFFIC** - Leo sits alone at table 3, fidgeting.
- Pre: LEO (Dash): "Okay. Be cool. You're a cool guy who drinks coffee. Cool guys drink coffee."
- Cue (Marlowe): "TRAFFIC! Give me a city! Push that car!"
- Post P/G: LEO: "Ah, the city. So busy. So many... cars. That's a normal thing to say."
- Post L: LEO: "Ah, the city. Eventually busy."
- Post M: LEO: "Huh. Quiet street." *Dash (whisper): "Was there supposed to be a car?"* MARLOWE: "THERE WAS SUPPOSED TO BE A CAR."

**A2 · DOOR** (opt) - Clara walks to the cafe door.
- Pre: MARLOWE: "Juniper's walking in. Doorbell on her hand on the handle, Rookie."
- Cue (Marlowe): "DOORBELL! When she opens it!"
- Post P/G: CLARA (Juniper): "Leo? Hi! You look exactly like your profile picture. That never happens."
- Post L: CLARA: "Leo? Hi! The door... rang late. Is that normal here?"
- Post M: *Juniper opens the door. Silence. She closes it and opens it again, looking at the camera.* *JUNIPER: "Should I knock?"*
- Then (all): LEO: "I use a very honest camera." CLARA (sitting): "Clearly."

**A3 · STEAM**
- Pre: LEO: "I got you a cappuccino. I didn't know how you take it, so I got it... the way they make it."
- Cue (Marlowe): "STEAM! I want romance in the air! I want the audience to SMELL the espresso!"
- Post P/G: CLARA: "This place is lovely. It smells like burnt coffee and second chances." *DASH (whisper): "That's not the line." JUNIPER (whisper): "It's my line now."* MARLOWE (whisper): "Keep rolling, keep rolling..."
- Post L: CLARA: "Oh. There's the steam. Very... punctual."
- Post M (over-steam variant if fog): *Both cough.* *JUNIPER: "I can't see him. Honestly? Improvement."*
- Post M (no steam): CLARA: "It smells like... nothing. Like a set." MARLOWE: "IT'S A CAFe, IT SHOULD SMELL LIKE A CAFe."

**A4 · NAPKINS** - Leo knocks over his cup (scripted).
- Pre: LEO: "So, what do you do for- " *(knocks cup over)* "Oh no. Oh no no no."
- Cue (Marlowe): "PROPS! Napkins! Toss them to Dash! Underhand! He can't catch overhand!"
- Post P/G: LEO (dabbing): "Sorry. I'm a little nervous. I ruin things when I'm nervous." *JUNIPER (quietly): "Yeah. You do."* *DASH (stops dabbing): "...Are we doing this now? On camera?"*
- Post L: LEO: "Got them. Eventually. Like everything in my life." *JUNIPER: "Yeah. Eventually."*
- Post M (bonk): *DASH: "OW. Who threw that?"* *JUNIPER (laughing despite herself): "The universe."* *DASH: "...Are we doing this now? On camera?"*

**A5 · THUNDER** - the fight boils over.
- Pre: *JUNIPER: "You want to do this? Fine. Three years, Dash. Three years and you ended it with a fruit basket."* *DASH: "It was a NICE fruit basket!"*
- Cue (Marlowe): "They're off-script! I don't care! THUNDER! Punctuate the drama! Make the gods jealous!"
- Post P/G: *(Thunder crack.) JUNIPER (standing): "I need some air."* *(Storms toward the front stair.)* LEO / *DASH*: "Clara, wait! - Jun, WAIT!"
- Post L: *(Thunder, a beat too late.) JUNIPER: "I need some air." (pauses) "...That was thunder, right? Okay. Storming off now."*
- Post M: *(Feeble bwomp.) JUNIPER: "I need some air." DASH: "Was that thunder or your stomach?" JUNIPER: "STORMING OFF NOW."*
- Then (all): MARLOWE: "They're going UPSTAIRS! Camera, follow them! Crane up! Rookie, GO GO GO, to the second floor! Take the back stairs, the front ones are in the shot!"

---

#### ACT II - THE TERRACE (camera position B, checkpoint)

**B1 · SUNSET**
- Pre: *JUNIPER (at the railing): "You know what the worst part is? You never even said sorry."* *DASH: "I sent a fruit basket!"* *JUNIPER: "You threw it in a LAKE!"* *DASH: "It was a fruit basket ON a lake! It was a boat basket! It was ROMANTIC!"*
- Cue (Marlowe): "SUNSET! Give me orange! Give me longing! Give me magic hour!"
- Post P/G: MARLOWE (whisper): "Oh, that light. That's the light. Nobody move." *JUNIPER (softer): "It sank, Dash. Like, immediately. The ducks ate the grapes."*
- Post L: *JUNIPER: "...It sank, Dash. The ducks ate the grapes."*
- Post M (wrong gel): *DASH (bathed in green): "Why is everything green? Am I sick? Is THIS what heartbreak looks like?"* MARLOWE: "WRONG GEL! WRONG GEL!"
- Post M (no gel): *JUNIPER: "It sank, Dash." DASH: "Can we at least get some nice lighting for this?"*

**B2 · WIND** (opt)
- Pre: *DASH: "I wanted it to be big. Memorable. You deserved memorable."*
- Cue (Marlowe): "WIND! Her hair needs to move like a shampoo commercial! Crank it!"
- Post P/G: *JUNIPER (hair blowing perfectly): "I didn't want big, Dash. I wanted small. A text. A sorry. One small thing."*
- Post L: *JUNIPER (hair moves, finally): "...I wanted small, Dash. One small thing."*
- Post M (over-wind): *(Napkins and her scarf fly away.) JUNIPER: "MY SCARF-" DASH: "I'll get you a new one! A BIG one!" JUNIPER: "THAT'S THE PROBLEM!"*
- Post M (no wind): *JUNIPER (hair perfectly still): "I wanted small, Dash." DASH: "Your hair is very calm about this."*

**B3 · BOOM**
- Pre: MARLOWE (whisper through megaphone, which defeats the purpose): "Quiet moment. Quiet moment. Boom mic, Rookie. Over them. Don't you DARE dip into frame."
- Cue (Marlowe): "BOOM! Get the mic up! Gently!"
- Post P/G: *DASH (quietly): "I'm sorry, Jun. Not Clara. You. I'm sorry."* *(Long pause.)* *JUNIPER: "...Say that again. I don't think the mic got it."* *DASH: "It got it."* MARLOWE (sniffling): "It got it."
- Post L: *DASH: "I'm sorry, Jun." JUNIPER: "What?" DASH: "I'm SORRY." JUNIPER: "...Okay. I heard that one."*
- Post M (boom in shot): *(Mic dips into frame and bumps Dash's head.) DASH: "I'm sorr- OW." JUNIPER (laughing): "Even the microphone thinks you're late."*

**B4 · RAIN** - the big one.
- Pre: *JUNIPER: "You can't just say sorry on a terrace at sunset and expect-"* MARLOWE: "NOW! Rookie, the rain rig! FULL BLAST! Make it POUR!"
- Cue (Marlowe): "RAIN! RAIN! RAIIIIN!"
- Post P/G: *(Downpour.) JUNIPER (soaked, stunned, then laughing): "Of course it's raining. Of COURSE it is."* LEO / *DASH (holding out an umbrella he doesn't have yet)*: "Do you want my... I don't have an umbrella. Hang on."
- Post L: *(Rain arrives a beat late.) JUNIPER: "...And NOW it's raining. Great timing. Very you."*
- Post M (drizzle): *(A sad drizzle.) JUNIPER: "Is it... supposed to be raining?"* MARLOWE: "IT IS SUPPOSED TO BE POURING."

---

#### ACT III - THE RAIN (camera position C, checkpoint; rain level carries over from B4)

**C1 · UMBRELLA**
- Pre: MARLOWE: "He needs an umbrella! The prop table upstairs!"
- Cue (Marlowe): "UMBRELLA! To Dash! Throw it! NOW!"
- Post P/G: *(Dash catches it, opens it with a flourish.) DASH: "Umbrella?"* *JUNIPER (looking at it, then at him): "No."* *(She takes it, and tosses it over the railing.)* *JUNIPER: "Let it rain."*
- Post L: *(Dash fumbles the catch, opens it upside down.) JUNIPER: "No. Let it rain." (tosses it)*
- Post M (bonk): *(It bonks Dash.) DASH: "OW. Why does everything keep hitting me today?"* *JUNIPER: "Karma. Let it rain."*

**C2 · ROSES**
- Pre: *DASH: "Okay. Small gesture. Hang on."* MARLOWE: "Is he- he's improvising. Roses! Give him the roses!"
- Cue (Marlowe): "ROSES! Toss them to Dash! He's about to do something stupid and wonderful!"
- Post P/G: *DASH (holding out the roses): "I stole these from the props table. Small gesture."* *JUNIPER: "...That's a very small gesture."* *DASH: "Is it enough?"* *JUNIPER (smiling): "It's a start."*
- Post L: *DASH: "I... found these. Small gesture." JUNIPER: "They're a little crushed." DASH: "So am I." JUNIPER: "...It's a start."*
- Post M: *(The roses sail past.) DASH (empty-handed, holding out a single petal stuck to his hand): "Small gesture?"* *JUNIPER (laughing): "The smallest. It's a start."*

**C3 · MOONLIGHT** (opt)
- Pre: MARLOWE (barely breathing): "Moonlight. Soft. Romantic. Don't you dare touch anything else, Rookie."
- Cue (Marlowe): "MOONLIGHT! Blue gel! Gently!"
- Post P/G: *(Blue moonlight.) JUNIPER: "Remember the lake?"* *DASH: "I'll never forget the lake."* *JUNIPER: "The ducks were happy, at least."*
- Post L: *JUNIPER: "Remember the lake?" DASH: "The ducks were happy."*
- Post M (green): *DASH (green): "Is the moon... seasick?"* *JUNIPER (giggling): "Don't make me laugh, I'm trying to have a moment."*

**C4 · FINAL THUNDER**
- Pre: *(They step closer under the rain.)* MARLOWE (whisper): "Don't... nobody breathe... Rookie, one last thunder. Make it count. When they lean in."
- Cue (Marlowe): "...and THUNDER!"
- Post P/G: *(Lightning flash: the two lean in, in silhouette. Black frame.)*
- Post L: *(They lean in, wait, wait... flash.) DASH (muffled): "Are we waiting for thunder?" (flash)*
- Post M: *(They lean in. Nothing. They both look at the camera.) JUNIPER: "...We're doing it anyway."* *(A desk lamp flickers.)*
- Then (all) → **Finale**.

### 2.3 Finale & epilogue (`Bark.P2.Wrap`, Pri 3)

| ID | Speaker | Line |
|---|---|---|
| P2_Wrap_1 | Marlowe | "...Cut." |
| P2_Wrap_2 | Marlowe | "CUT! That's a wrap! THAT'S A WRAP!" |
| P2_Wrap_3 | Juniper | "We're going to need a few retakes on that." |
| P2_Wrap_4 | Dash | "At least twelve." |
| P2_Wrap_5 | Marlowe | "No retakes! That WAS the take! That's the Director's Cut!" |
| P2_Wrap_6 | Marlowe | "Rookie. You made it rain. Here. Crew jacket. It's a little damp. Same time tomorrow, Assistant Director." |

### 2.4 Cue result reactions from Marlowe (`Bark.P2.Result.<Cue>.<Grade>`)
Played **before** the beat's Post lines only when the Post list for that grade has no Marlowe line. Keep them short; the actors carry the scene.

| Cue | Perfect | Good | Late | Miss |
|---|---|---|---|---|
| Traffic | "Gorgeous traffic! I believe in this city!" | "Nice. A little jerky, like real traffic." | "Late traffic. Also realistic." | "Where's my CAR?" |
| Door | "Ding! Perfect timing!" | "Good ding." | "Late ding. Still a ding." | "That was a ding for a ghost." |
| Steam | "Look at that steam! I can smell it from here!" | "Nice puff." | "A late puff." | "Too much! It's a cafe, not a sauna!" (over) / "No steam?!" (none) |
| Napkins | "Perfect toss! He even caught it!" | "Caught! Barely! Good!" | "He had to bend down. Undignified, but fine." | "Not his FACE!" |
| Thunder | "THAT'S thunder!" | "Good rumble!" | "A little late, like a real storm." | "That was a burp. That was a weather burp." |
| Sunset | "Magic hour! I'm weeping!" | "Lovely orange." | "The sun took its time." | "GREEN?!" (wrong) / "Where's my sunset?!" (none) |
| Wind | "Shampoo commercial! Perfect!" | "Nice breeze." | "Breeze arrived late." | "That's a hurricane!" (over) / "Her hair is not moving!" (none) |
| Boom | "Clean audio! Clean frame! I love you!" | "Good sound." | "We'll fix it in post." | "BOOM IN SHOT!" |
| Rain | "LOOK AT THAT RAIN! I'm a GENIUS! You're a genius! We're all wet!" | "Rain! Real-looking rain!" | "Late rain! Still rain!" | "That's a drizzle! I asked for drama, not humidity!" |
| Umbrella | "Perfect throw!" | "Good catch, Dash!" | "Close enough!" | "Stop hitting my lead actor!" |
| Roses | "Rose delivery! Romantic!" | "Nice!" | "Slightly crushed. Like love." | "They're in the street!" |
| Moonlight | "Moonlight! Perfect!" | "Nice blue." | "The moon rose slowly." | "Is the moon SICK?" |
| Final Thunder | (none, silence for the moment) | (none) | (none) | (none) |

### 2.5 Other Phase 2 lines

| ID | Trigger | Line |
|---|---|---|
| P2_InFrame_01 | `InFrame` | "Rookie, you're IN THE SHOT!" |
| P2_InFrame_02 | `InFrame` | "Get out of frame! You're not in this movie!" |
| P2_InFrame_03 | `InFrame` | "Why is there a Rookie in my romance?!" |
| P2_InFrame_04 | `InFrame` | "Duck! DUCK! Not a real duck! Get DOWN!" |
| P2_InFrame_05 | `InFrame` | "The audience will think you're the love interest! Move!" |
| P2_Boom_01 | `BoomInShot` | "Boom's in the shot! Up! UP!" |
| P2_Bail_01 | `Bail` | "Where are you going? The cue isn't done!" |
| P2_CueIdle_01 | `CueIdle` | "The cue is NOW, Rookie! Not later! NOW!" |
| P2_CueIdle_02 | `CueIdle` | "The station's glowing! Glowing means GO!" |
| P2_Patience_01 | `Patience` | "EVERYONE FREEZE! ...Okay, carry on. Don't ask me how I did that." |
| P2_Cut_01 | `Cut` | "CUT! CUT CUT CUT!" |
| P2_Cut_02 | `Cut` | "CUT! That was a disaster! A beautiful disaster, but a disaster!" |
| P2_Cut_03 | `Cut` | "CUT! Reset! Somebody get Dash a towel and me a sedative!" |
| P2_Restart_A | `RestartAct.A` | "From the top! Places! Dash, stop eating the prop croissant! ...And ACTION!" |
| P2_Restart_B | `RestartAct.B` | "From the terrace! Places! Juniper, you're furious. Dash, you're sorry. Rookie, you're fast. ACTION!" |
| P2_Restart_C | `RestartAct.C` | "From the rain! Everybody's wet, everybody's in love. ACTION!" |

### 2.6 Phase 2 end-screen quotes (by stars)

| Stars | Line |
|---|---|
| 1 | "We got it. Somehow. Cinema is a miracle." |
| 2 | "That's a scene, Rookie. That's a real scene." |
| 3 | "That's the Director's Cut. Frame it. Frame YOU." |

---

## 3. Menu & Pause Quips (random, shown in small italic on the screens)

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
