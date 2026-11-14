// Clean! Cut! Action!

#include "Shoot/LTTSceneData.h"

namespace LTTScene42
{
	FLTTDialogueLine Line(const TCHAR* Speaker, const TCHAR* Text, bool bOffScript = false, const TCHAR* Montage = nullptr)
	{
		FLTTDialogueLine L;
		L.Speaker = FName(Speaker);
		L.Text = FText::FromString(Text);
		L.bOffScript = bOffScript;
		if (Montage)
		{
			L.Montage = FName(Montage);
		}
		return L;
	}

	/** The line also sends an actor (the speaker unless Actor is given) to a mark. */
	FLTTDialogueLine Walk(FLTTDialogueLine L, const TCHAR* Mark, const TCHAR* Actor = nullptr)
	{
		L.MoveToMark = FName(Mark);
		if (Actor)
		{
			L.MoveActor = FName(Actor);
		}
		return L;
	}

	FLTTBeat& AddBeat(TArray<FLTTBeat>& Beats, const TCHAR* ID, const TCHAR* Act, ELTTCue Cue, float Window, const TCHAR* Call,
		bool bGradeBySpeed = false, bool bOptional = false)
	{
		FLTTBeat& B = Beats.AddDefaulted_GetRef();
		B.BeatID = FName(ID);
		B.Act = FName(Act);
		B.bOptional = bOptional;
		B.CueSpec.Cue = Cue;
		B.CueSpec.BaseWindow = Window;
		B.CueSpec.bGradeBySpeed = bGradeBySpeed;
		B.CueSpec.CallLine = FText::FromString(Call);
		return B;
	}

	const TCHAR* Marlowe = TEXT("Marlowe");
	const TCHAR* Dash = TEXT("Dash");
	const TCHAR* Juniper = TEXT("Juniper");
	const TCHAR* Leo = TEXT("Leo");
	const TCHAR* Clara = TEXT("Clara");
}

const TArray<FLTTDialogueLine>& FLTTBeat::GetPost(ELTTGrade Grade) const
{
	// Each list falls back to the next better one.
	if (Grade == ELTTGrade::Miss && PostMiss.Num() > 0)
	{
		return PostMiss;
	}
	if ((Grade == ELTTGrade::Miss || Grade == ELTTGrade::Late) && PostLate.Num() > 0)
	{
		return PostLate;
	}
	if (Grade != ELTTGrade::Perfect && PostGood.Num() > 0)
	{
		return PostGood;
	}
	return PostPerfect;
}

FText ULTTSceneData::GetCueName(ELTTCue Cue)
{
	switch (Cue)
	{
	case ELTTCue::Slate: return NSLOCTEXT("LTTShoot", "CueSlate", "SLATE");
	case ELTTCue::Traffic: return NSLOCTEXT("LTTShoot", "CueTraffic", "TRAFFIC");
	case ELTTCue::Door: return NSLOCTEXT("LTTShoot", "CueDoor", "DOORBELL");
	case ELTTCue::Steam: return NSLOCTEXT("LTTShoot", "CueSteam", "STEAM");
	case ELTTCue::Napkins: return NSLOCTEXT("LTTShoot", "CueNapkins", "NAPKINS");
	case ELTTCue::Thunder: return NSLOCTEXT("LTTShoot", "CueThunder", "THUNDER");
	case ELTTCue::Sunset: return NSLOCTEXT("LTTShoot", "CueSunset", "SUNSET");
	case ELTTCue::Wind: return NSLOCTEXT("LTTShoot", "CueWind", "WIND");
	case ELTTCue::Boom: return NSLOCTEXT("LTTShoot", "CueBoom", "BOOM MIC");
	case ELTTCue::Rain: return NSLOCTEXT("LTTShoot", "CueRain", "RAIN");
	case ELTTCue::Umbrella: return NSLOCTEXT("LTTShoot", "CueUmbrella", "UMBRELLA");
	case ELTTCue::Roses: return NSLOCTEXT("LTTShoot", "CueRoses", "ROSES");
	case ELTTCue::Moonlight: return NSLOCTEXT("LTTShoot", "CueMoonlight", "MOONLIGHT");
	default: return FText::GetEmpty();
	}
}

FText ULTTSceneData::GetGradeName(ELTTGrade Grade)
{
	switch (Grade)
	{
	case ELTTGrade::Perfect: return NSLOCTEXT("LTTShoot", "GradePerfect", "PERFECT!");
	case ELTTGrade::Good: return NSLOCTEXT("LTTShoot", "GradeGood", "GOOD");
	case ELTTGrade::Late: return NSLOCTEXT("LTTShoot", "GradeLate", "LATE");
	case ELTTGrade::Miss: return NSLOCTEXT("LTTShoot", "GradeMiss", "MISS!");
	default: return FText::GetEmpty();
	}
}

ULTTSceneData* ULTTSceneData::MakeScene42(UObject* Outer)
{
	ULTTSceneData* Data = NewObject<ULTTSceneData>(Outer ? Outer : GetTransientPackage(), NAME_None, RF_Transient);
	Data->FillWithScene42();
	return Data;
}

void ULTTSceneData::FillWithScene42()
{
	using namespace LTTScene42;

	PreRoll.Reset();
	SlateDone.Reset();
	Beats.Reset();
	Wrap.Reset();
	ActTitles.Reset();
	RestartLines.Reset();

	ActTitles.Add(TEXT("A"), FText::FromString(TEXT("ACT I - THE ARRIVAL")));
	ActTitles.Add(TEXT("B"), FText::FromString(TEXT("ACT II - THE TERRACE")));
	ActTitles.Add(TEXT("C"), FText::FromString(TEXT("ACT III - THE RAIN")));

	RestartLines.Add(TEXT("A"), FText::FromString(TEXT("From the top! Places! Dash, stop eating the prop croissant! ...And ACTION!")));
	RestartLines.Add(TEXT("B"), FText::FromString(TEXT("From the terrace! Places! Juniper, you're furious. Dash, you're sorry. Rookie, you're fast. ACTION!")));
	RestartLines.Add(TEXT("C"), FText::FromString(TEXT("From the rain! Everybody's wet, everybody's in love. ACTION!")));

	// ---- 2.1 Arrival & pre-roll ----
	PreRoll.Add(Line(Marlowe, TEXT("Rookie! The set looks incredible. The actors are here. The actors are... not speaking to each other. Great. Perfect. Fine.")));
	PreRoll.Add(Line(Marlowe, TEXT("Listen up! Scene forty-two. The cafe. Leo and Clara's first date. Rookie, you're on effects.")));
	PreRoll.Add(Line(Marlowe, TEXT("When I call a cue, you run to that station and make it happen. Rain, traffic, thunder. You're the weather now.")));
	PreRoll.Add(Line(Marlowe, TEXT("And see that glowing cone? That's what the camera sees. If you walk into it, you're in my movie. You are NOT in my movie.")));
	PreRoll.Add(Line(Marlowe, TEXT("First, slate the shot. Stand on the mark, clap the board. The one time you're allowed in frame. Enjoy it.")));
	SlateDone.Add(Line(Marlowe, TEXT("Scene forty-two, take one! Now get OUT of my shot! ...And... ACTION!")));

	// ---- ACT I - THE ARRIVAL ----
	{
		FLTTBeat& B = AddBeat(Beats, TEXT("A1_Traffic"), TEXT("A"), ELTTCue::Traffic, 9.f, TEXT("TRAFFIC! Give me a city! Push that car!"));
		B.bActCheckpoint = true;
		B.DashStartMark = TEXT("A_LeoTable");
		B.JuniperStartMark = TEXT("A_ClaraDoor");
		B.PreDialogue.Add(Line(Leo, TEXT("Okay. Be cool. You're a cool guy who drinks coffee. Cool guys drink coffee.")));
		B.PostPerfect.Add(Line(Leo, TEXT("Ah, the city. So busy. So many... cars. That's a normal thing to say.")));
		B.PostLate.Add(Line(Leo, TEXT("Ah, the city. Eventually busy.")));
		B.PostMiss.Add(Line(Leo, TEXT("Huh. Quiet street.")));
		B.PostMiss.Add(Line(Dash, TEXT("(whisper) Was there supposed to be a car?"), true));
		B.PostMiss.Add(Line(Marlowe, TEXT("THERE WAS SUPPOSED TO BE A CAR.")));
	}
	{
		FLTTBeat& B = AddBeat(Beats, TEXT("A2_Door"), TEXT("A"), ELTTCue::Door, 7.f, TEXT("DOORBELL! When she opens it!"), false, true);
		B.PreDialogue.Add(Line(Marlowe, TEXT("Juniper's walking in. Doorbell on her hand on the handle, Rookie.")));
		B.PostPerfect.Add(Line(Clara, TEXT("Leo? Hi! You look exactly like your profile picture. That never happens.")));
		B.PostLate.Add(Line(Clara, TEXT("Leo? Hi! The door... rang late. Is that normal here?")));
		B.PostMiss.Add(Line(Juniper, TEXT("(opens the door twice, looks at the camera) Should I knock?"), true));
		B.PostAlways.Add(Line(Leo, TEXT("I use a very honest camera.")));
		B.PostAlways.Add(Walk(Line(Clara, TEXT("Clearly.")), TEXT("A_ClaraTable")));
	}
	{
		FLTTBeat& B = AddBeat(Beats, TEXT("A3_Steam"), TEXT("A"), ELTTCue::Steam, 8.f, TEXT("STEAM! I want romance in the air! I want the audience to SMELL the espresso!"));
		B.PreDialogue.Add(Walk(Line(Leo, TEXT("I got you a cappuccino. I didn't know how you take it, so I got it... the way they make it.")), TEXT("A_ClaraTable"), Juniper));
		B.PostPerfect.Add(Line(Clara, TEXT("This place is lovely. It smells like burnt coffee and second chances.")));
		B.PostPerfect.Add(Line(Dash, TEXT("(whisper) That's not the line."), true));
		B.PostPerfect.Add(Line(Juniper, TEXT("(whisper) It's my line now."), true));
		B.PostPerfect.Add(Line(Marlowe, TEXT("(whisper) Keep rolling, keep rolling...")));
		B.PostLate.Add(Line(Clara, TEXT("Oh. There's the steam. Very... punctual.")));
		B.PostMiss.Add(Line(Clara, TEXT("It smells like... nothing. Like a set.")));
		B.PostMiss.Add(Line(Marlowe, TEXT("IT'S A CAFE, IT SHOULD SMELL LIKE A CAFE.")));
	}
	{
		FLTTBeat& B = AddBeat(Beats, TEXT("A4_Napkins"), TEXT("A"), ELTTCue::Napkins, 8.f, TEXT("PROPS! Napkins! Toss them to Dash! Underhand! He can't catch overhand!"), true);
		B.PreDialogue.Add(Line(Leo, TEXT("So, what do you do for- (knocks his cup over) Oh no. Oh no no no."), false, TEXT("Startled")));
		B.PostPerfect.Add(Line(Leo, TEXT("Sorry. I'm a little nervous. I ruin things when I'm nervous."), false, TEXT("Catch")));
		B.PostPerfect.Add(Line(Juniper, TEXT("(quietly) Yeah. You do."), true));
		B.PostPerfect.Add(Line(Dash, TEXT("...Are we doing this now? On camera?"), true));
		B.PostLate.Add(Line(Leo, TEXT("Got them. Eventually. Like everything in my life.")));
		B.PostLate.Add(Line(Juniper, TEXT("Yeah. Eventually."), true));
		B.PostMiss.Add(Line(Dash, TEXT("OW. Who threw that?"), true, TEXT("Startled")));
		B.PostMiss.Add(Line(Juniper, TEXT("(laughing despite herself) The universe."), true, TEXT("Laugh")));
		B.PostMiss.Add(Line(Dash, TEXT("...Are we doing this now? On camera?"), true));
	}
	{
		FLTTBeat& B = AddBeat(Beats, TEXT("A5_Thunder"), TEXT("A"), ELTTCue::Thunder, 10.f, TEXT("They're off-script! I don't care! THUNDER! Punctuate the drama! Make the gods jealous!"));
		B.PreDialogue.Add(Line(Juniper, TEXT("You want to do this? Fine. Three years, Dash. Three years and you ended it with a fruit basket."), true, TEXT("Angry")));
		B.PreDialogue.Add(Line(Dash, TEXT("It was a NICE fruit basket!"), true));
		B.PostPerfect.Add(Walk(Line(Juniper, TEXT("I need some air."), true, TEXT("StormOff")), TEXT("B_ClaraRail")));
		B.PostPerfect.Add(Line(Dash, TEXT("Clara, wait! - Jun, WAIT!"), true));
		B.PostLate.Add(Walk(Line(Juniper, TEXT("I need some air. ...That was thunder, right? Okay. Storming off now."), true, TEXT("StormOff")), TEXT("B_ClaraRail")));
		B.PostMiss.Add(Line(Juniper, TEXT("I need some air."), true));
		B.PostMiss.Add(Line(Dash, TEXT("Was that thunder or your stomach?"), true));
		B.PostMiss.Add(Walk(Line(Juniper, TEXT("STORMING OFF NOW."), true, TEXT("StormOff")), TEXT("B_ClaraRail")));
		B.PostAlways.Add(Walk(Line(Marlowe, TEXT("They're going UPSTAIRS! Camera, follow them! Crane up! Rookie, GO GO GO, to the second floor! Take the back stairs, the front ones are in the shot!")), TEXT("B_LeoRail"), Dash));
	}

	// ---- ACT II - THE TERRACE ----
	{
		FLTTBeat& B = AddBeat(Beats, TEXT("B1_Sunset"), TEXT("B"), ELTTCue::Sunset, 9.f, TEXT("SUNSET! Give me orange! Give me longing! Give me magic hour!"), true);
		B.bActCheckpoint = true;
		B.DashStartMark = TEXT("B_LeoRail");
		B.JuniperStartMark = TEXT("B_ClaraRail");
		B.PreDialogue.Add(Line(Juniper, TEXT("You know what the worst part is? You never even said sorry."), true));
		B.PreDialogue.Add(Line(Dash, TEXT("I sent a fruit basket!"), true));
		B.PreDialogue.Add(Line(Juniper, TEXT("You threw it in a LAKE!"), true, TEXT("Angry")));
		B.PreDialogue.Add(Line(Dash, TEXT("It was a fruit basket ON a lake! It was a boat basket! It was ROMANTIC!"), true));
		B.PostPerfect.Add(Line(Marlowe, TEXT("(whisper) Oh, that light. That's the light. Nobody move.")));
		B.PostPerfect.Add(Line(Juniper, TEXT("(softer) It sank, Dash. Like, immediately. The ducks ate the grapes."), true));
		B.PostLate.Add(Line(Juniper, TEXT("...It sank, Dash. The ducks ate the grapes."), true));
		B.PostMiss.Add(Line(Juniper, TEXT("It sank, Dash."), true));
		B.PostMiss.Add(Line(Dash, TEXT("Can we at least get some nice lighting for this?"), true));
	}
	{
		FLTTBeat& B = AddBeat(Beats, TEXT("B2_Wind"), TEXT("B"), ELTTCue::Wind, 9.f, TEXT("WIND! Her hair needs to move like a shampoo commercial! Crank it!"), false, true);
		B.PreDialogue.Add(Line(Dash, TEXT("I wanted it to be big. Memorable. You deserved memorable."), true));
		B.PostPerfect.Add(Line(Juniper, TEXT("I didn't want big, Dash. I wanted small. A text. A sorry. One small thing."), true));
		B.PostLate.Add(Line(Juniper, TEXT("...I wanted small, Dash. One small thing."), true));
		B.PostMiss.Add(Line(Juniper, TEXT("(hair perfectly still) I wanted small, Dash."), true));
		B.PostMiss.Add(Line(Dash, TEXT("Your hair is very calm about this."), true));
	}
	{
		FLTTBeat& B = AddBeat(Beats, TEXT("B3_Boom"), TEXT("B"), ELTTCue::Boom, 9.f, TEXT("BOOM! Get the mic up! Gently!"));
		B.PreDialogue.Add(Line(Marlowe, TEXT("(whispering through the megaphone) Quiet moment. Quiet moment. Boom mic, Rookie. Over them. Don't you DARE dip into frame.")));
		B.PostPerfect.Add(Line(Dash, TEXT("(quietly) I'm sorry, Jun. Not Clara. You. I'm sorry."), true));
		B.PostPerfect.Add(Line(Juniper, TEXT("...Say that again. I don't think the mic got it."), true));
		B.PostPerfect.Add(Line(Dash, TEXT("It got it."), true));
		B.PostPerfect.Add(Line(Marlowe, TEXT("(sniffling) It got it.")));
		B.PostLate.Add(Line(Dash, TEXT("I'm sorry, Jun."), true));
		B.PostLate.Add(Line(Juniper, TEXT("What?"), true));
		B.PostLate.Add(Line(Dash, TEXT("I'm SORRY."), true));
		B.PostLate.Add(Line(Juniper, TEXT("...Okay. I heard that one."), true));
		B.PostMiss.Add(Line(Dash, TEXT("I'm sorr- OW."), true, TEXT("Startled")));
		B.PostMiss.Add(Line(Juniper, TEXT("(laughing) Even the microphone thinks you're late."), true, TEXT("Laugh")));
	}
	{
		FLTTBeat& B = AddBeat(Beats, TEXT("B4_Rain"), TEXT("B"), ELTTCue::Rain, 12.f, TEXT("RAIN! RAIN! RAIIIIN!"));
		B.PreDialogue.Add(Line(Juniper, TEXT("You can't just say sorry on a terrace at sunset and expect-"), true));
		B.PreDialogue.Add(Line(Marlowe, TEXT("NOW! Rookie, the rain rig! FULL BLAST! Make it POUR!")));
		B.PostPerfect.Add(Line(Juniper, TEXT("(soaked, then laughing) Of course it's raining. Of COURSE it is."), true, TEXT("Laugh")));
		B.PostPerfect.Add(Line(Dash, TEXT("Do you want my... I don't have an umbrella. Hang on."), true));
		B.PostLate.Add(Line(Juniper, TEXT("...And NOW it's raining. Great timing. Very you."), true));
		B.PostMiss.Add(Line(Juniper, TEXT("Is it... supposed to be raining?"), true));
		B.PostMiss.Add(Line(Marlowe, TEXT("IT IS SUPPOSED TO BE POURING.")));
	}

	// ---- ACT III - THE RAIN ----
	{
		FLTTBeat& B = AddBeat(Beats, TEXT("C1_Umbrella"), TEXT("C"), ELTTCue::Umbrella, 8.f, TEXT("UMBRELLA! To Dash! Throw it! NOW!"), true);
		B.bActCheckpoint = true;
		B.DashStartMark = TEXT("C_Center_L");
		B.JuniperStartMark = TEXT("C_Center_R");
		B.RestoreEffects = { ELTTCue::Sunset, ELTTCue::Rain };
		B.PreDialogue.Add(Walk(Line(Marlowe, TEXT("He needs an umbrella! The prop table upstairs!")), TEXT("C_Center_L"), Dash));
		B.PostPerfect.Add(Walk(Line(Dash, TEXT("Umbrella?"), true, TEXT("Catch")), TEXT("C_Center_R"), Juniper));
		B.PostPerfect.Add(Line(Juniper, TEXT("No. (tosses it over the railing) Let it rain."), true));
		B.PostLate.Add(Walk(Line(Dash, TEXT("(opens it upside down) ...Umbrella?"), true), TEXT("C_Center_R"), Juniper));
		B.PostLate.Add(Line(Juniper, TEXT("No. Let it rain."), true));
		B.PostMiss.Add(Walk(Line(Dash, TEXT("OW. Why does everything keep hitting me today?"), true, TEXT("Startled")), TEXT("C_Center_R"), Juniper));
		B.PostMiss.Add(Line(Juniper, TEXT("Karma. Let it rain."), true));
	}
	{
		FLTTBeat& B = AddBeat(Beats, TEXT("C2_Roses"), TEXT("C"), ELTTCue::Roses, 8.f, TEXT("ROSES! Toss them to Dash! He's about to do something stupid and wonderful!"), true);
		B.PreDialogue.Add(Line(Dash, TEXT("Okay. Small gesture. Hang on."), true));
		B.PreDialogue.Add(Line(Marlowe, TEXT("Is he- he's improvising. Roses! Give him the roses!")));
		B.PostPerfect.Add(Line(Dash, TEXT("I stole these from the props table. Small gesture."), true, TEXT("Catch")));
		B.PostPerfect.Add(Line(Juniper, TEXT("...That's a very small gesture."), true));
		B.PostPerfect.Add(Line(Dash, TEXT("Is it enough?"), true));
		B.PostPerfect.Add(Line(Juniper, TEXT("(smiling) It's a start."), true));
		B.PostLate.Add(Line(Dash, TEXT("I... found these. Small gesture."), true));
		B.PostLate.Add(Line(Juniper, TEXT("They're a little crushed."), true));
		B.PostLate.Add(Line(Dash, TEXT("So am I."), true));
		B.PostLate.Add(Line(Juniper, TEXT("...It's a start."), true));
		B.PostMiss.Add(Line(Dash, TEXT("(holding out a single petal) Small gesture?"), true));
		B.PostMiss.Add(Line(Juniper, TEXT("(laughing) The smallest. It's a start."), true, TEXT("Laugh")));
	}
	{
		FLTTBeat& B = AddBeat(Beats, TEXT("C3_Moonlight"), TEXT("C"), ELTTCue::Moonlight, 8.f, TEXT("MOONLIGHT! Blue gel! Gently!"), true, true);
		B.PreDialogue.Add(Line(Marlowe, TEXT("(barely breathing) Moonlight. Soft. Romantic. Don't you dare touch anything else, Rookie.")));
		B.PostPerfect.Add(Line(Juniper, TEXT("Remember the lake?"), true));
		B.PostPerfect.Add(Line(Dash, TEXT("I'll never forget the lake."), true));
		B.PostPerfect.Add(Line(Juniper, TEXT("The ducks were happy, at least."), true));
		B.PostLate.Add(Line(Juniper, TEXT("Remember the lake?"), true));
		B.PostLate.Add(Line(Dash, TEXT("The ducks were happy."), true));
		B.PostMiss.Add(Line(Dash, TEXT("Is the moon... seasick?"), true));
		B.PostMiss.Add(Line(Juniper, TEXT("(giggling) Don't make me laugh, I'm trying to have a moment."), true, TEXT("Laugh")));
	}
	{
		FLTTBeat& B = AddBeat(Beats, TEXT("C4_FinalThunder"), TEXT("C"), ELTTCue::Thunder, 9.f, TEXT("...and THUNDER!"));
		B.PreDialogue.Add(Line(Marlowe, TEXT("(whisper) Don't... nobody breathe... Rookie, one last thunder. Make it count. When they lean in.")));
		B.PostPerfect.Add(Line(Juniper, TEXT("(they lean in, in silhouette against the flash)"), true, TEXT("LeanIn")));
		B.PostLate.Add(Line(Dash, TEXT("(leaning in, muffled) Are we waiting for thunder?"), true, TEXT("LeanIn")));
		B.PostMiss.Add(Line(Juniper, TEXT("(they both look at the camera) ...We're doing it anyway."), true, TEXT("LeanIn")));
	}

	// ---- 2.3 Finale & epilogue ----
	Wrap.Add(Line(Marlowe, TEXT("...Cut.")));
	Wrap.Add(Line(Marlowe, TEXT("CUT! That's a wrap! THAT'S A WRAP!")));
	Wrap.Add(Line(Juniper, TEXT("We're going to need a few retakes on that."), true));
	Wrap.Add(Line(Dash, TEXT("At least twelve."), true));
	Wrap.Add(Line(Marlowe, TEXT("No retakes! That WAS the take! That's the Director's Cut!")));
	Wrap.Add(Line(Marlowe, TEXT("Rookie. You made it rain. Here. Crew jacket. It's a little damp. Same time tomorrow, Assistant Director.")));

#if WITH_EDITOR
	if (!HasAnyFlags(RF_Transient))
	{
		MarkPackageDirty();
	}
#endif
}
