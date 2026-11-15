// Clean! Cut! Action!

#include "Interaction/LTTCleanupBarker.h"
#include "Interaction/LTTMessDirector.h"
#include "Interaction/LTTInteractableBase.h"
#include "Interaction/LTTCarryComponent.h"
#include "Interaction/LTTInteractionComponent.h"
#include "Interaction/LTTPowerBox.h"
#include "Interaction/LTTPowerSubsystem.h"
#include "Interaction/LTTLightSwitchComponent.h"
#include "UObject/UObjectIterator.h"
#include "UI/LTTBarkWidget.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "UI/LTTInputDevice.h"

#define LOCTEXT_NAMESPACE "LTTCleanupBarker"

ALTTCleanupBarker::ALTTCleanupBarker()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.f;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	WidgetClass = ULTTBarkWidget::StaticClass();

	static ConstructorHelpers::FObjectFinder<UTexture2D> Megaphone(TEXT("/Game/_CleanCutAction/UI/Widget/megaphone.megaphone"));
	BarkIcon = Megaphone.Object;
}

ALTTCleanupBarker* ALTTCleanupBarker::Get(const UObject* WorldContext)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<ALTTCleanupBarker> It(World); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

// ---- Lines (Docs/05 §1, adapted to the mechanics this build has) ----

ALTTCleanupBarker::FPool& ALTTCleanupBarker::AddPool(FName Id, int32 Priority, float Chance, float Cooldown,
	std::initializer_list<FText> Lines, ELTTBarkSpeaker Speaker)
{
	FPool& Pool = Pools.Add(Id);
	Pool.Priority = Priority;
	Pool.Chance = Chance;
	Pool.Cooldown = Cooldown;
	for (const FText& Text : Lines)
	{
		FLine Line;
		Line.Text = Text;
		Line.Speaker = Speaker;
		Line.Priority = Priority;
		Pool.Lines.Add(Line);
	}
	return Pool;
}

void ALTTCleanupBarker::BuildPools()
{
	using S = ELTTBarkSpeaker;

	// 1.1 Intro (chain).
	AddPool("Intro", 3, 1.f, 0.f, {
		LOCTEXT("P1_Intro_1", "Rookie! Welcome to the last day of shooting on Clean! Cut! Action! Look around. Breathe it in. That smell? That's my career."),
		LOCTEXT("P1_Intro_2", "Last night's wrap party got... enthusiastic. The actors arrive in one hour, and this set needs to be spotless."),
		LOCTEXT("P1_Intro_3", "See those see-through outlines? Every prop has a home. Put everything back exactly where it lives. Exactly. I measure with my soul."),
		LOCTEXT("P1_Intro_4", "Start with something close. Pick it up. Put it on its mark. I'll wait. I'm a patient man. That's a lie, but go."),
		LOCTEXT("P1_Intro_5", "Lost? Press {0} and I'll point. Grudgingly.") }).bChain = true;
	Pools["Intro"].bOneShot = true;

	// 1.2 First-time tutorials (one-shot).
	AddPool("First.Pickup", 3, 1.f, 0.f, { LOCTEXT("P1_First_Pickup", "Look at you, holding a prop. Your parents must be so proud.") }).bOneShot = true;
	AddPool("First.Place", 3, 1.f, 0.f, { LOCTEXT("P1_First_Place", "Hear that? That's the sound of order. Burn it into your memory.") }).bOneShot = true;
	AddPool("First.Throw", 3, 1.f, 0.f, { LOCTEXT("P1_First_Throw", "{0} to throw. Wait, why am I teaching you to throw props on my set? It BREAKS them!") }).bOneShot = true;
	AddPool("First.Stack", 3, 1.f, 0.f, { LOCTEXT("P1_First_Stack", "Stacking! Efficient. Reckless. I respect both.") }).bOneShot = true;
	AddPool("First.Bin", 3, 1.f, 0.f, { LOCTEXT("P1_First_Bin", "Trash goes in the bin. Revolutionary, I know. Write it down.") }).bOneShot = true;
	AddPool("First.Broom", 3, 1.f, 0.f, { LOCTEXT("P1_First_Broom", "That's cleaning gear. Push it at the mess. It's basically filmmaking.") }).bOneShot = true;
	AddPool("First.RightIt", 3, 1.f, 0.f, { LOCTEXT("P1_First_RightIt", "Flip that {0} back up. Gently. It has a stunt double, and it's busier than you.") }).bOneShot = true;
	AddPool("First.Light", 3, 1.f, 0.f, { LOCTEXT("P1_First_Light", "A light! Put it on its mark. The mark is where the magic happens. Magic is expensive. Don't break it.") }).bOneShot = true;
	AddPool("First.Cable", 3, 1.f, 0.f, { LOCTEXT("P1_First_Cable", "Follow the cable to the outlet. Like a detective, but useful.") }).bOneShot = true;
	AddPool("First.Reel", 3, 1.f, 0.f, { LOCTEXT("P1_First_Reel", "Those go on the shelf in order. Like film reels. One through five. Like the stages of grief.") }).bOneShot = true;
	AddPool("First.Costume", 3, 1.f, 0.f, { LOCTEXT("P1_First_Costume", "Soft goods! Juniper's blues on the left, Dash's browns on the right. Dash only wears brown. Don't ask. It's a whole thing.") }).bOneShot = true;
	AddPool("First.Lens", 3, 1.f, 0.f, { LOCTEXT("P1_First_Lens", "Wipe the lens! Someone licked it. I don't want to know who. I know who.") }).bOneShot = true;
	AddPool("First.Dish", 3, 1.f, 0.f, { LOCTEXT("P1_First_Dish", "Dirty dishes go in the sink. Somebody drank four lattes out of that. It was me.") }).bOneShot = true;
	AddPool("First.Break", 3, 1.f, 0.f, { LOCTEXT("P1_First_Break", "Gently! Everything on this set is rented! Broken props cost points. And my deposit.") }).bOneShot = true;

	// 1.3 Correct placement (25%).
	AddPool("Place.Correct", 1, 0.25f, 0.f, {
		LOCTEXT("P1_Place_01", "Yes. Yes! That's where it lives."),
		LOCTEXT("P1_Place_02", "Beautiful. I'd frame that. I won't. But I'd think about it."),
		LOCTEXT("P1_Place_03", "Perfect. Now do that eighty more times."),
		LOCTEXT("P1_Place_04", "The lamp thanks you. I thank you. My therapist thanks you."),
		LOCTEXT("P1_Place_05", "See? The set is healing."),
		LOCTEXT("P1_Place_06", "Gorgeous placement. Very motivated. Very method."),
		LOCTEXT("P1_Place_07", "Correct! Somewhere, a production designer stopped crying."),
		LOCTEXT("P1_Place_08", "That's a keeper. Print it."),
		LOCTEXT("P1_Place_09", "Like a puzzle piece coming home for the holidays."),
		LOCTEXT("P1_Place_10", "Ooh, the angle on that. You've done this before. Have you done this before?"),
		LOCTEXT("P1_Place_11", "Exactly on the mark. I may have misjudged you, Rookie."),
		LOCTEXT("P1_Place_12", "Nailed it. Not literally. Please don't nail anything.") });

	// 1.4 Streaks.
	AddPool("Streak.3", 2, 1.f, 0.f, { LOCTEXT("P1_Streak_3", "Ooh, a combo! Who ARE you?") });
	AddPool("Streak.5", 2, 1.f, 0.f, { LOCTEXT("P1_Streak_5", "Five in a row! Somebody get this Rookie an agent!") });
	AddPool("Streak.10", 2, 1.f, 0.f, { LOCTEXT("P1_Streak_10", "TEN! I'm getting emotional. Don't look at me. Keep going. Don't look at me.") });

	// 1.5 Wrong spot / dropped (30%, 15 s).
	AddPool("Place.Wrong", 1, 0.3f, 15.f, {
		LOCTEXT("P1_Wrong_01", "That's not its home. That's its vacation spot."),
		LOCTEXT("P1_Wrong_02", "Close isn't a mark. Close is how we get sued."),
		LOCTEXT("P1_Wrong_03", "Interesting choice. Wrong, but interesting."),
		LOCTEXT("P1_Wrong_04", "That prop looks lost and confused. Much like our lead actor."),
		LOCTEXT("P1_Wrong_05", "If I can see it from here, it's wrong. I can see it from here."),
		LOCTEXT("P1_Wrong_06", "Floor is not a mark. The floor is the enemy.") });

	// 1.6 Crashes and throwing.
	AddPool("Crash", 1, 0.5f, 20.f, {
		LOCTEXT("P1_Crash_01", "Was that crash part of the plan?"),
		LOCTEXT("P1_Crash_02", "I heard that. The insurance company heard that."),
		LOCTEXT("P1_Crash_03", "Gently! Everything on this set is rented!"),
		LOCTEXT("P1_Crash_04", "Ah. The sound of my deposit leaving forever.") });
	AddPool("Throw", 1, 0.15f, 0.f, {
		LOCTEXT("P1_Throw_01", "Are you throwing props? On MY set? ...Do it again, that was cool."),
		LOCTEXT("P1_Throw_02", "Nice arc. Terrible manners."),
		LOCTEXT("P1_Throw_03", "Props are not frisbees. Except the reels. Apparently.") });
	AddPool("TrickShot", 2, 1.f, 0.f, {
		LOCTEXT("P1_TrickShot_01", "Did that just land on its mark? From THERE? I'm putting that in the blooper reel and the trailer."),
		LOCTEXT("P1_TrickShot_02", "Trick shot! You're wasted on tidying. You should be in stunts.") });

	// 1.7 Trash (and the sink).
	AddPool("Trash.Miss", 1, 0.25f, 0.f, {
		LOCTEXT("P1_Miss_01", "And it misses. Like my last two films."),
		LOCTEXT("P1_Miss_02", "Brick! Walk it over like a normal person.") });
	AddPool("BinWrongItem", 2, 1.f, 8.f, {
		LOCTEXT("P1_BinWrong_01", "That's a set piece, not garbage! The bin spat it out! Even the bin knows!") });

	// 1.8 Sweeping (the cafe has no popcorn piles: trash and dishes are the sweeping).
	AddPool("Sweep.Stroke", 1, 0.05f, 0.f, {
		LOCTEXT("P1_Sweep_01", "Sweep like nobody's watching. I'm watching. Sweep anyway."),
		LOCTEXT("P1_Sweep_02", "Every kernel of popcorn is a tiny insult to cinema."),
		LOCTEXT("P1_Sweep_03", "Popcorn. On a FILM set. The irony is crushing me.") });
	AddPool("Sweep.Cleared", 2, 1.f, 0.f, {
		LOCTEXT("P1_SweepDone_01", "That pile is gone! Like it never happened. Like my divorce."),
		LOCTEXT("P1_SweepDone_02", "Clean floor! I can see my reflection. I look tired.") });

	// 1.9 Cables, lights, reels, costumes, lens.
	AddPool("Cable.Power", 2, 1.f, 0.f, { LOCTEXT("P1_Cable_01", "Power restored! I feel a tingle. Either it's art, or I'm being electrocuted.") });
	// The breaker starts off: the first line after the intro, then reminders until the power is back.
	AddPool("Power.Out", 3, 1.f, 0.f, {
		LOCTEXT("P1_PowerOut_01", "And somebody tripped the breaker at the party. Find the electric box - look for the little red light - and turn the power back on.") }).bOneShot = true;
	AddPool("Power.Remind", 2, 1.f, 0.f, {
		LOCTEXT("P1_PowerRemind_01", "Still dark, Rookie! The electric box. Red light. Flip it. Let there be light."),
		LOCTEXT("P1_PowerRemind_02", "I can't direct what I can't see. Power. Box. On. Now."),
		LOCTEXT("P1_PowerRemind_03", "The breaker, Rookie! This is a film set, not a seance."),
		LOCTEXT("P1_PowerRemind_04", "Follow the arrow to the electric box and turn the power on. I'll be here. In the dark. Brooding.") });
	AddPool("Cable.Placed", 1, 1.f, 20.f, { LOCTEXT("P1_Cable_02", "That cable had more knots than this script.") });
	AddPool("Light.Done", 1, 1.f, 15.f, {
		LOCTEXT("P1_Light_01", "Look at that glow. That's called motivated lighting. I motivated it. With yelling."),
		LOCTEXT("P1_Light_02", "Right on the X! A gaffer somewhere just felt a disturbance in the force.") });
	AddPool("Reel.Wrong", 1, 1.f, 20.f, {
		LOCTEXT("P1_ReelWrong_01", "The {0} before the {1}? Are you EDITING my film?"),
		LOCTEXT("P1_ReelWrong_02", "Order, Rookie. The {1} goes back first. That's the whole trick.") });
	AddPool("Reel.Done", 2, 1.f, 0.f, { LOCTEXT("P1_ReelDone_01", "All in order. That's the first thing on this production that's been in order.") });
	AddPool("Costume.Hung", 1, 0.3f, 0.f, {
		LOCTEXT("P1_Costume_01", "Careful! That costs more than your car. It costs more than MY car."),
		LOCTEXT("P1_Costume_02", "That's been through a lot. Mostly Dash.") });
	AddPool("Costume.Done", 2, 1.f, 0.f, { LOCTEXT("P1_CostumeDone_01", "Wardrobe is happy! Wardrobe is never happy! What did you do?") });
	AddPool("Lens.Done", 2, 1.f, 0.f, { LOCTEXT("P1_Lens_01", "Crystal clear! Now I can see all my mistakes in high definition.") });

	AddPool("Trash.Bin", 1, 0.15f, 0.f, {
		LOCTEXT("P1_Bin_01", "Into the bin. Goodbye, pizza. You were a good pizza."),
		LOCTEXT("P1_Bin_02", "Somebody ordered eleven pizzas and ate four slices. I'm not naming names. Dash."),
		LOCTEXT("P1_Bin_03", "Is that a shoe? Why is there only one shoe? Bin it. No questions."),
		LOCTEXT("P1_Bin_04", "Confetti. Who approved a confetti cannon? ...I approved the confetti cannon."),
		LOCTEXT("P1_Bin_05", "Party hats in the trash. As is tradition.") });
	AddPool("Trash.Swish", 1, 0.35f, 0.f, {
		LOCTEXT("P1_Swish_01", "SWISH! Three points! The crowd goes mild!"),
		LOCTEXT("P1_Swish_02", "Nothing but bin! Somebody get this on camera!"),
		LOCTEXT("P1_Swish_03", "From downtown! I'm calling it a sport now.") });
	AddPool("Dish.Washed", 1, 0.3f, 0.f, {
		LOCTEXT("P1_Dish_01", "Washing dishes. On a film set. The most realistic thing we've shot all year."),
		LOCTEXT("P1_Dish_02", "Scrub it like it owes you money."),
		LOCTEXT("P1_Dish_03", "Clean cup! Fake coffee tastes better in a clean cup.") });

	// 1.12 Progress milestones.
	AddPool("Progress.10", 2, 1.f, 0.f, { LOCTEXT("P1_Prog_10", "Ten percent. It's a start. A sad start, but a start.") });
	AddPool("Progress.25", 2, 1.f, 0.f, { LOCTEXT("P1_Prog_25", "A quarter done! I can see the floor. I forgot we had a floor.") });
	AddPool("Progress.50", 2, 1.f, 0.f, { LOCTEXT("P1_Prog_50", "Halfway! I've stopped hyperventilating. Mostly.") });
	AddPool("Progress.75", 2, 1.f, 0.f, { LOCTEXT("P1_Prog_75", "Seventy-five percent! I'm starting to believe in things again.") });
	AddPool("Progress.90", 2, 1.f, 0.f, { LOCTEXT("P1_Prog_90", "Ninety percent! The last bits are hiding. I can feel them hiding.") });
	AddPool("Progress.99", 2, 1.f, 0.f, { LOCTEXT("P1_Prog_99", "One thing left. ONE. It's mocking us. Find it.") });

	// 1.16 Idle and hints.
	AddPool("Idle", 1, 1.f, 0.f, {
		LOCTEXT("P1_Idle_01", "Hello? Rookie? Is this a dramatic pause?"),
		LOCTEXT("P1_Idle_02", "You're standing still. Are you thinking? Think faster."),
		LOCTEXT("P1_Idle_03", "The mess is still here. It's patient. I'm not."),
		LOCTEXT("P1_Idle_04", "Lost? Press {0}. That's what it's there for. And to cover a hole."),
		LOCTEXT("P1_Idle_05", "Did you fall asleep standing up? Save that skill for a night shoot.") });
	AddPool("Hint", 2, 0.7f, 0.f, {
		LOCTEXT("P1_Hint_01", "The {0}! Right there! I can feel it in my tear ducts."),
		LOCTEXT("P1_Hint_02", "Follow the arrow, Rookie. The {0} misses its home."),
		LOCTEXT("P1_Hint_03", "That {0} is hiding. Props always are."),
		LOCTEXT("P1_Hint_04", "Go get the {0}. Everyone needs love. Especially the {0}.") });

	// Countdown (not in the doc's table; the jam build has a clock).
	AddPool("Time.5Min", 2, 1.f, 0.f, { LOCTEXT("P1_Time_5", "Five minutes, Rookie! The actors are in the parking lot. I can hear Dash's car. It sounds nervous.") });
	AddPool("Time.1Min", 2, 1.f, 0.f, { LOCTEXT("P1_Time_1", "ONE MINUTE! Move like a producer is chasing you!") });
	AddPool("Time.Up", 3, 1.f, 0.f, { LOCTEXT("P1_Time_Up", "TIME! ...The actors are here. Act natural. Pretend the mess is a choice.") });

	// 1.18 Walkie-talkie story drips (chains at 20/40/60/80%).
	{
		FPool& P = AddPool("Story.20", 3, 1.f, 0.f, {});
		P.bChain = true;
		P.Lines = {
			{ LOCTEXT("P1_Story20_1", "Marlowe, it's Juniper. Is HE there yet?"), S::WalkieJuniper, 3 },
			{ LOCTEXT("P1_Story20_2", "Who, Dash? Not yet."), S::Marlowe, 3 },
			{ LOCTEXT("P1_Story20_3", "Good. Tell me when he gets there, so I can be later."), S::WalkieJuniper, 3 } };
	}
	{
		FPool& P = AddPool("Story.40", 3, 1.f, 0.f, {});
		P.bChain = true;
		P.Lines = {
			{ LOCTEXT("P1_Story40_1", "Marlowe, buddy! Is Juniper on set? Asking for a friend. The friend is me."), S::WalkieDash, 3 },
			{ LOCTEXT("P1_Story40_2", "She's not here, Dash."), S::Marlowe, 3 },
			{ LOCTEXT("P1_Story40_3", "Cool. Cool cool cool. Totally cool. I'm cool."), S::WalkieDash, 3 } };
	}
	{
		FPool& P = AddPool("Story.60", 3, 1.f, 0.f, {
			LOCTEXT("P1_Story60_1", "Between us, Rookie... Dash and Juniper used to date. In real life. It ended badly."),
			LOCTEXT("P1_Story60_2", "There was a fruit basket. And a lake. Nobody talks about the lake.") });
		P.bChain = true;
	}
	{
		FPool& P = AddPool("Story.80", 3, 1.f, 0.f, {});
		P.bChain = true;
		P.Lines = {
			{ LOCTEXT("P1_Story80_1", "In an hour they have to fall in love on camera. I'm going to need the rain machine. And a miracle. Mostly the rain machine."), S::Marlowe, 3 },
			{ LOCTEXT("P1_Story80_2", "Marlowe, I'm parking. Is the set ready?"), S::WalkieJuniper, 3 },
			{ LOCTEXT("P1_Story80_3", "...Define 'ready'."), S::Marlowe, 3 } };
	}

	// 1.20 Phase 1 complete (chain).
	AddPool("Complete", 3, 1.f, 0.f, {
		LOCTEXT("P1_Done_1", "Is that... is it... it's clean. The set is clean."),
		LOCTEXT("P1_Done_2", "Look at it. Look at the LIGHT. I think I'm going to cry. I am crying. It's the megaphone. It's dusty."),
		LOCTEXT("P1_Done_3", "PLACES, EVERYONE! Get the actors! Get the rain machine! Get me a tissue!") }).bChain = true;

	for (TPair<FName, FPool>& Pair : Pools)
	{
		if (Pair.Value.bChain)
		{
			Pair.Value.bOneShot = true;
		}
		// The intro and the "first time you..." lines are the tutorial.
		if (Pair.Key == "Intro" || Pair.Key.ToString().StartsWith(TEXT("First.")))
		{
			Pair.Value.bTutorial = true;
			for (FLine& Line : Pair.Value.Lines)
			{
				Line.bTutorial = true;
			}
		}
	}
}

// ---- Setup ----

void ALTTCleanupBarker::BeginPlay()
{
	Super::BeginPlay();
	BuildPools();
	TryBind();
}

void ALTTCleanupBarker::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Widget)
	{
		Widget->RemoveFromParent();
		Widget = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

float ALTTCleanupBarker::Now() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.f;
}

void ALTTCleanupBarker::TryBind()
{
	if (!bDirectorBound)
	{
		if (ALTTMessDirector* D = ALTTMessDirector::Get(this))
		{
			Director = D;
			D->OnProgressChanged.AddDynamic(this, &ALTTCleanupBarker::HandleProgress);
			D->OnAllPlaced.AddDynamic(this, &ALTTCleanupBarker::HandleAllPlaced);
			D->OnTimeUp.AddDynamic(this, &ALTTCleanupBarker::HandleTimeUp);
			D->OnHintShown.AddDynamic(this, &ALTTCleanupBarker::HandleHint);
			bDirectorBound = true;
		}
	}
	const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Carry.IsValid())
	{
		if (ULTTCarryComponent* C = Pawn ? Pawn->FindComponentByClass<ULTTCarryComponent>() : nullptr)
		{
			Carry = C;
			C->OnPickedUp.AddDynamic(this, &ALTTCleanupBarker::HandlePickedUp);
			C->OnReleased.AddDynamic(this, &ALTTCleanupBarker::HandleReleased);
		}
	}
	if (!bFocusBound)
	{
		if (ULTTInteractionComponent* I = Pawn ? Pawn->FindComponentByClass<ULTTInteractionComponent>() : nullptr)
		{
			I->OnFocusChanged.AddDynamic(this, &ALTTCleanupBarker::HandleFocusChanged);
			bFocusBound = true;
		}
	}
	if (!bLightsBound && bDirectorBound)
	{
		// Every switchable light: a lamp put back on its mark, or a wall / ceiling lamp flipped on.
		for (TObjectIterator<ULTTLightSwitchComponent> It; It; ++It)
		{
			if (It->GetWorld() == GetWorld())
			{
				It->OnLightToggled.AddUniqueDynamic(this, &ALTTCleanupBarker::HandleLightToggled);
			}
		}
		bLightsBound = true;
	}
	if (!bPowerBound)
	{
		if (ULTTPowerSubsystem* Power = GetWorld() ? GetWorld()->GetSubsystem<ULTTPowerSubsystem>() : nullptr)
		{
			Power->OnPowerChanged.AddDynamic(this, &ALTTCleanupBarker::HandlePowerChanged);
			bPowerWasOff = !Power->IsPowerOn();
			bPowerBound = true;
		}
	}
	CreateWidgetIfNeeded();
}

void ALTTCleanupBarker::CreateWidgetIfNeeded()
{
	if (Widget || !WidgetClass)
	{
		return;
	}
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC || !PC->IsLocalController())
	{
		return;
	}
	Widget = CreateWidget<ULTTBarkWidget>(PC, WidgetClass);
	if (Widget)
	{
		Widget->Setup(this, BarkIcon);
		Widget->AddToViewport(5); // over the cleanup HUD (1) and the ending shots, under the end screen (10)
	}
}

// ---- Saying lines ----

float ALTTCleanupBarker::GetLineSeconds(const FLine& Line) const
{
	TArray<FString> Words;
	Line.Text.ToString().ParseIntoArrayWS(Words);
	return FMath::Max(MinLineSeconds, Words.Num() * SecondsPerWord) + LinePadding;
}

void ALTTCleanupBarker::StartLine(const FLine& Line)
{
	Current = Line;
	bTalking = true;
	LineSecondsLeft = GetLineSeconds(Line);
	CurrentDuration = LineSecondsLeft;
	CurrentStartTime = Now();
	bCurrentRequeued = false;
	BatchInsertIndex = 0;
	++LineSerial;
	UE_LOG(LogTemp, Log, TEXT("Bark [%s] %s"), *StaticEnum<ELTTBarkSpeaker>()->GetNameStringByValue(static_cast<int64>(Line.Speaker)), *Line.Text.ToString());
	USoundBase* Sound = Line.Speaker == ELTTBarkSpeaker::Marlowe ? LineSound.Get() : WalkieSound.Get();
	if (Sound)
	{
		UGameplayStatics::PlaySound2D(this, Sound);
	}
}

void ALTTCleanupBarker::Say(TArray<FLine> Lines)
{
	if (!bEnabled || Lines.Num() == 0)
	{
		return;
	}
	if (Lines[0].Priority == 2)
	{
		// One milestone waits; the newest replaces an older one.
		Queue.RemoveAll([](const FLine& L) { return L.Priority == 2; });
	}

	// Nobody talking (or between two lines of a chain): say it now; the rest of the chain carries on after it.
	if (!bTalking)
	{
		const FLine First = Lines[0];
		Lines.RemoveAt(0);
		Queue.Insert(Lines, 0);
		ChainWait = 0.f;
		StartLine(First);
		return;
	}

	const float T = Now();
	const float Shown = T - CurrentStartTime;

	// Said in the same moment as the line on screen (a pickup line and its tutorial): play right after it, in order.
	if (Shown <= 0.05f)
	{
		Queue.Insert(Lines, FMath::Min(BatchInsertIndex, Queue.Num()));
		BatchInsertIndex += Lines.Num();
		return;
	}

	// Interrupting the tutorial once the player has placed enough props on their own: they know the game, so the
	// tutorial stops for the session (the cut line, the rest of the intro, and every "first time" line to come).
	if (Current.bTutorial && !bTutorialsOff && TutorialSkipAfterPlaced > 0 && PlacedByPlayer >= TutorialSkipAfterPlaced)
	{
		bTutorialsOff = true;
		bCurrentRequeued = true; // don't say the cut line again
		Queue.RemoveAll([](const FLine& L) { return L.bTutorial; });
		Lines.RemoveAll([](const FLine& L) { return L.bTutorial; });
		UE_LOG(LogTemp, Log, TEXT("Bark: tutorial stopped (player placed %d props)."), PlacedByPlayer);
		if (Lines.Num() == 0)
		{
			return; // a tutorial cut in on a tutorial: nothing left to say, let the line on screen finish
		}
	}

	// Interrupt. A tutorial / story / milestone line that was cut short is said again after the new line, and the
	// rest of its chain (still in the queue) follows. A cut-off reaction is just dropped.
	TArray<FLine> Front = MoveTemp(Lines);
	if (Current.Priority >= 2 && !bCurrentRequeued && Shown < CurrentDuration * 0.75f)
	{
		Front.Add(Current);
		bCurrentRequeued = true;
	}

	// The line on screen has only just appeared: let it be read for a moment before cutting it.
	if (Shown < MinShowBeforeInterrupt)
	{
		LineSecondsLeft = FMath::Min(LineSecondsLeft, MinShowBeforeInterrupt - Shown);
		Queue.Insert(Front, 0);
		return;
	}

	const FLine First = Front[0];
	Front.RemoveAt(0);
	Queue.Insert(Front, 0);
	ChainWait = 0.f;
	StartLine(First);
}

bool ALTTCleanupBarker::Trigger(FName Id, const FFormatOrderedArguments& Args, bool bIgnoreChance)
{
	FPool* Pool = Pools.Find(Id);
	if (!bEnabled || !Pool || Pool->Lines.Num() == 0)
	{
		return false;
	}
	if ((Pool->bOneShot && Pool->bUsed) || (Pool->bTutorial && bTutorialsOff))
	{
		return false;
	}
	const float T = Now();
	if (T < Pool->ReadyTime)
	{
		return false;
	}
	const float Chance = Pool->Priority <= 1 ? Pool->Chance * ChattinessScale : Pool->Chance;
	if (!bIgnoreChance && Chance < 1.f && FMath::FRand() >= Chance)
	{
		return false;
	}
	// Reactions cut in on whatever is being said, but not more often than every ReactionGap seconds
	// (a skipped one doesn't use up its line or cooldown).
	if (Pool->Priority <= 1)
	{
		if (T < LastReactionTime + ReactionGap)
		{
			return false;
		}
		LastReactionTime = T;
	}

	auto Fill = [&Args](FLine Line)
	{
		if (Args.Num() > 0)
		{
			Line.Text = FText::Format(Line.Text, Args);
		}
		return Line;
	};

	TArray<FLine> Lines;
	if (Pool->bChain)
	{
		for (const FLine& L : Pool->Lines)
		{
			Lines.Add(Fill(L));
		}
	}
	else
	{
		if (Pool->Bag.Num() == 0)
		{
			for (int32 i = 0; i < Pool->Lines.Num(); ++i)
			{
				Pool->Bag.Insert(i, FMath::RandRange(0, Pool->Bag.Num()));
			}
		}
		Lines.Add(Fill(Pool->Lines[Pool->Bag.Pop()]));
	}
	Pool->bUsed = true;
	Pool->ReadyTime = T + Pool->Cooldown;
	Say(MoveTemp(Lines));
	return true;
}

void ALTTCleanupBarker::Bark(const FText& Text, ELTTBarkSpeaker Speaker, int32 Priority)
{
	FLine Line;
	Line.Text = Text;
	Line.Speaker = Speaker;
	Line.Priority = Priority;
	Say({ Line });
}

void ALTTCleanupBarker::SetBarksEnabled(bool bInEnabled)
{
	bEnabled = bInEnabled;
	if (!bEnabled)
	{
		ClearQueuedBarks();
	}
}

void ALTTCleanupBarker::ClearQueuedBarks()
{
	Queue.Reset();
	BatchInsertIndex = 0;
	ChainWait = 0.f;
	bCurrentRequeued = false;
	if (bTalking)
	{
		bTalking = false;
		LineSecondsLeft = 0.f;
		QuietSince = Now();
	}
}

bool ALTTCleanupBarker::GetCurrentLine(FText& OutText, FText& OutSpeaker, bool& bOutWalkie) const
{
	if (!bTalking)
	{
		return false;
	}
	OutText = Current.Text;
	bOutWalkie = Current.Speaker != ELTTBarkSpeaker::Marlowe;
	switch (Current.Speaker)
	{
	case ELTTBarkSpeaker::WalkieDash:    OutSpeaker = LOCTEXT("SpeakerDash", "DASH (walkie)"); break;
	case ELTTBarkSpeaker::WalkieJuniper: OutSpeaker = LOCTEXT("SpeakerJuniper", "JUNIPER (walkie)"); break;
	default:                             OutSpeaker = LOCTEXT("SpeakerMarlowe", "MARLOWE"); break;
	}
	return true;
}

// ---- Tick ----

void ALTTCleanupBarker::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bDirectorBound || !Carry.IsValid() || !bFocusBound || !bPowerBound || !Widget)
	{
		TryBind();
	}

	if (bTalking)
	{
		LineSecondsLeft -= DeltaSeconds;
		if (LineSecondsLeft <= 0.f)
		{
			bTalking = false;
			QuietSince = Now();
			ChainWait = Queue.Num() > 0 ? FMath::Max(ChainGap, 0.01f) : 0.f;
		}
	}
	else if (ChainWait > 0.f)
	{
		ChainWait -= DeltaSeconds;
		if (ChainWait <= 0.f && Queue.Num() > 0)
		{
			ChainWait = 0.f;
			const FLine Next = Queue[0];
			Queue.RemoveAt(0);
			StartLine(Next);
		}
	}
	else if (Queue.Num() > 0)
	{
		const FLine Next = Queue[0];
		Queue.RemoveAt(0);
		StartLine(Next);
	}

	TickTriggers();
}

void ALTTCleanupBarker::TickTriggers()
{
	const ALTTMessDirector* D = Director.Get();
	if (!D || bFinished)
	{
		return;
	}
	const float T = Now();

	if (!bStarted)
	{
		if (!D->HasMessStarted() || D->IsBriefingOpen())
		{
			return;
		}
		bStarted = true;
		StartTime = T;
		LastActivityTime = T;
		NextIdleTime = T + IdleSeconds;
		LastPlaced = D->GetPlacedCount(); // props that start at home don't count as the player's progress
		StartPlaced = LastPlaced;
		LastBroken = D->GetBrokenCount();
	}

	if (!bIntroDone && T >= StartTime + IntroDelay)
	{
		bIntroDone = true;
		FFormatOrderedArguments Args;
		Args.Add(ULTTInputDeviceSubsystem::KeyLabelFor(D->HintKey, D->HintGamepadKey));
		Trigger("Intro", Args);
	}

	// Breaker off: tell the player once the intro is done, then remind them until the power's back.
	if (bIntroDone && !bTalking && Queue.Num() == 0 && T >= NextPowerNagTime && !ULTTPowerSubsystem::IsPowerOnIn(this))
	{
		if (Trigger(bPowerOutSaid ? FName("Power.Remind") : FName("Power.Out")))
		{
			bPowerOutSaid = true;
			NextPowerNagTime = T + PowerNagSeconds;
		}
	}

	// Countdown warnings (only when the clock is long enough for them to mean something).
	if (D->bUseTimer && D->IsTimerRunning())
	{
		const float Left = D->GetTimeRemaining();
		if (!bWarned5Min && Left <= 300.f && D->TimeLimitSeconds > 420.f)
		{
			bWarned5Min = true;
			Trigger("Time.5Min");
		}
		if (!bWarned1Min && Left <= 60.f && D->TimeLimitSeconds > 120.f)
		{
			bWarned1Min = true;
			Trigger("Time.1Min");
		}
	}

	// Idle: nothing put back and nothing in hand for a while.
	const ULTTCarryComponent* C = Carry.Get();
	if (C && C->GetHeldItem())
	{
		LastActivityTime = T;
	}
	if (T - LastActivityTime >= IdleSeconds && T >= NextIdleTime && !bTalking && Queue.Num() == 0)
	{
		FFormatOrderedArguments Args;
		Args.Add(ULTTInputDeviceSubsystem::KeyLabelFor(D->HintKey, D->HintGamepadKey));
		if (Trigger("Idle", Args))
		{
			NextIdleTime = T + IdleRepeatSeconds;
		}
	}
}

// ---- Events ----

void ALTTCleanupBarker::HandleProgress(int32 Placed, int32 Total)
{
	const ALTTMessDirector* D = Director.Get();
	if (!D || !bStarted || bFinished)
	{
		LastPlaced = Placed;
		LastBroken = D ? D->GetBrokenCount() : 0;
		return;
	}

	const int32 Broken = D->GetBrokenCount();
	if (Broken > LastBroken)
	{
		LastActivityTime = Now();
		if (!Trigger("First.Break"))
		{
			Trigger("Crash");
		}
	}
	LastBroken = Broken;

	if (Placed > LastPlaced && !D->IsSetClean())
	{
		OnCleanedOne(); // a burst (the Place All cheat) counts once
	}
	LastPlaced = Placed;
	const bool bWasFamiliar = TutorialSkipAfterPlaced > 0 && PlacedByPlayer >= TutorialSkipAfterPlaced;
	PlacedByPlayer = FMath::Max(PlacedByPlayer, Placed - StartPlaced);
	if (!bWasFamiliar && TutorialSkipAfterPlaced > 0 && PlacedByPlayer >= TutorialSkipAfterPlaced)
	{
		// They know the game now: tutorial lines still waiting (the rest of the intro, a "first time" line, a cut
		// line waiting to be said again) are dropped. The line on screen finishes.
		const int32 Dropped = Queue.RemoveAll([](const FLine& L) { return L.bTutorial; });
		BatchInsertIndex = FMath::Min(BatchInsertIndex, Queue.Num());
		if (Current.bTutorial)
		{
			bCurrentRequeued = true; // if it gets cut now, don't say it again
		}
		UE_LOG(LogTemp, Log, TEXT("Bark: player placed %d props, dropped %d queued tutorial lines."), PlacedByPlayer, Dropped);
	}
	CheckMilestones(Placed, Total);
}

void ALTTCleanupBarker::OnCleanedOne()
{
	const float T = Now();
	LastActivityTime = T;

	ALTTInteractableBase* Item = LastReleased.Get();
	if (Item && (Item->IsWashed() || Item->IsTrashed()))
	{
		bool bSaid = false;
		if (Item->IsWashed())
		{
			bSaid = Trigger("Dish.Washed");
		}
		else
		{
			bSaid = Trigger("First.Bin");
			// Thrown in from range: a swish. Dropped in: just binned.
			bSaid = bSaid || Trigger(Item->bLastReleaseWasThrow ? FName("Trash.Swish") : FName("Trash.Bin"));
		}
		bSaid = bSaid || Trigger("Sweep.Stroke");
		if (!bTrashDone && IsTrashDone())
		{
			bTrashDone = true;
			Trigger("Sweep.Cleared");
		}
		return;
	}

	Streak = (T - LastPlaceTime <= StreakWindow) ? Streak + 1 : 1;
	LastPlaceTime = T;

	// Only trust the last released prop if it's the one that just went home (not a stale one before a cheat).
	if (Item && !Item->IsPlaced())
	{
		Item = nullptr;
	}

	bool bSaid = Item && Item->WasTrickShot() && Trigger("TrickShot");
	bSaid = Trigger("First.Place") || bSaid;
	if (!bSaid && Item && IsValid(Item->HomeSupport))
	{
		bSaid = Trigger("First.Stack"); // put on top of another prop (a cup on its table)
	}
	if (Item && OnKindCleaned(Item))
	{
		bSaid = true;
	}
	if (bSaid)
	{
		return;
	}
	if (Streak == 3 || Streak == 5 || Streak == 10)
	{
		Trigger(*FString::Printf(TEXT("Streak.%d"), Streak));
		return;
	}
	Trigger("Place.Correct");
}

bool ALTTCleanupBarker::OnKindCleaned(ALTTInteractableBase* Item)
{
	const ELTTBarkPropKind Kind = Item->GetBarkKind();
	switch (Kind)
	{
	case ELTTBarkPropKind::Light:
		return Trigger("Light.Done"); // back on its mark, the lamp lights up
	case ELTTBarkPropKind::Electric:
		return Trigger("Cable.Placed");
	case ELTTBarkPropKind::Camera:
	case ELTTBarkPropKind::Shelf:
	case ELTTBarkPropKind::Fabric:
	{
		if (!KindsDone.Contains(Kind) && IsKindDone(Kind))
		{
			KindsDone.Add(Kind);
			return Trigger(Kind == ELTTBarkPropKind::Camera ? FName("Lens.Done")
				: Kind == ELTTBarkPropKind::Shelf ? FName("Reel.Done") : FName("Costume.Done"));
		}
		return Kind == ELTTBarkPropKind::Fabric && Trigger("Costume.Hung");
	}
	default:
		return false;
	}
}

bool ALTTCleanupBarker::IsKindDone(ELTTBarkPropKind Kind) const
{
	// The cleanup tasks of this kind; if none are tasks (the camera is set dressing), every prop of the kind.
	bool bAnyTask = false;
	for (TActorIterator<ALTTInteractableBase> It(GetWorld()); It; ++It)
	{
		if (IsValid(*It) && It->bIsCleanupTask && It->GetBarkKind() == Kind)
		{
			bAnyTask = true;
			break;
		}
	}
	int32 Count = 0;
	for (TActorIterator<ALTTInteractableBase> It(GetWorld()); It; ++It)
	{
		const ALTTInteractableBase* Prop = *It;
		if (!IsValid(Prop) || (bAnyTask && !Prop->bIsCleanupTask) || Prop->GetBarkKind() != Kind)
		{
			continue;
		}
		if (!Prop->IsPlaced())
		{
			return false;
		}
		++Count;
	}
	return Count > 0;
}

bool ALTTCleanupBarker::IsTrashDone() const
{
	int32 Count = 0;
	for (TActorIterator<ALTTInteractableBase> It(GetWorld()); It; ++It)
	{
		const ALTTInteractableBase* Prop = *It;
		if (!IsValid(Prop) || !Prop->bIsCleanupTask || !Prop->bIsTrash)
		{
			continue;
		}
		if (!Prop->IsTrashed())
		{
			return false;
		}
		++Count;
	}
	return Count > 0;
}

void ALTTCleanupBarker::NotifyEvent(const UObject* WorldContext, ELTTBarkEvent Event, ALTTInteractableBase* Prop)
{
	if (ALTTCleanupBarker* Barker = Get(WorldContext))
	{
		Barker->HandleEvent(Event, Prop);
	}
}

void ALTTCleanupBarker::HandleEvent(ELTTBarkEvent Event, ALTTInteractableBase* Prop)
{
	if (!bStarted || bFinished)
	{
		return;
	}
	LastActivityTime = Now();
	switch (Event)
	{
	case ELTTBarkEvent::Crash:
		Trigger("Crash");
		break;
	case ELTTBarkEvent::TrashMiss:
		Trigger("Trash.Miss");
		break;
	case ELTTBarkEvent::BinWrongItem:
		NoWrongDropFor = Prop; // already told off; not "wrong spot" as well
		Trigger("BinWrongItem");
		break;
	case ELTTBarkEvent::WrongOrder:
		if (Prop)
		{
			NoWrongDropFor = Prop;
			const ALTTInteractableBase* Blocking = Prop->GetBlockingSupport();
			FFormatOrderedArguments Args;
			Args.Add(Prop->GetPropName());
			Args.Add(Blocking ? Blocking->GetPropName() : LOCTEXT("TheTable", "table"));
			Trigger("Reel.Wrong", Args);
		}
		break;
	}
}

void ALTTCleanupBarker::HandleFocusChanged(AActor* NewFocus, FText PromptText)
{
	if (!bStarted || bFinished || !NewFocus)
	{
		return;
	}
	if (NewFocus->IsA<ALTTPowerBox>())
	{
		Trigger("First.Cable");
		return;
	}
	if (const ALTTInteractableBase* Prop = Cast<ALTTInteractableBase>(NewFocus))
	{
		switch (Prop->GetBarkKind())
		{
		case ELTTBarkPropKind::Camera:   Trigger("First.Lens"); break;
		case ELTTBarkPropKind::Electric: Trigger("First.Cable"); break;
		default: break;
		}
	}
}

void ALTTCleanupBarker::HandleLightToggled(bool bIsOn)
{
	if (bIsOn && bStarted && !bFinished && ULTTPowerSubsystem::IsPowerOnIn(this))
	{
		LastActivityTime = Now();
		Trigger("Light.Done");
	}
}

void ALTTCleanupBarker::HandlePowerChanged(bool bPowerOn)
{
	if (!bPowerOn)
	{
		bPowerWasOff = true;
		return;
	}
	if (bPowerWasOff && bStarted && !bFinished)
	{
		bPowerWasOff = false;
		Trigger("Cable.Power");
	}
}

void ALTTCleanupBarker::CheckMilestones(int32 Placed, int32 Total)
{
	if (Total <= 0)
	{
		return;
	}
	const float Pct = 100.f * Placed / Total;

	// Story drips first (priority 3), then the % milestone (priority 2, waits behind them).
	for (const int32 Gate : { 20, 40, 60, 80 })
	{
		if (Pct >= Gate && !StoriesHit.Contains(Gate))
		{
			StoriesHit.Add(Gate);
			Trigger(*FString::Printf(TEXT("Story.%d"), Gate));
			break; // one chain at a time, in story order; a gate passed in the same jump plays on the next placement
		}
	}

	const int32 Left = Total - Placed;
	if (Left == 1 && Total >= 5 && !MilestonesHit.Contains(99))
	{
		for (const int32 Gate : { 10, 25, 50, 75, 90, 99 })
		{
			MilestonesHit.Add(Gate);
		}
		Trigger("Progress.99");
		return;
	}
	for (const int32 Gate : { 90, 75, 50, 25, 10 })
	{
		if (Pct >= Gate && !MilestonesHit.Contains(Gate))
		{
			// Only the highest one reached; the ones below it are skipped.
			for (const int32 Lower : { 90, 75, 50, 25, 10 })
			{
				if (Lower <= Gate)
				{
					MilestonesHit.Add(Lower);
				}
			}
			Trigger(*FString::Printf(TEXT("Progress.%d"), Gate));
			break;
		}
	}
}

void ALTTCleanupBarker::HandleAllPlaced()
{
	if (bFinished)
	{
		return;
	}
	bFinished = true;
	Queue.Reset();
	bTalking = false; // nothing cut off here comes back after the finale
	Trigger("Complete");
}

void ALTTCleanupBarker::HandleTimeUp()
{
	if (bFinished)
	{
		return;
	}
	Queue.Reset();
	bTalking = false;
	Trigger("Time.Up");
}

void ALTTCleanupBarker::HandleHint(ALTTInteractableBase* Target)
{
	if (!bStarted || bFinished || !Target)
	{
		return;
	}
	FFormatOrderedArguments Args;
	Args.Add(Target->GetPropName());
	Trigger("Hint", Args);
}

void ALTTCleanupBarker::HandlePickedUp(ALTTInteractableBase* Item)
{
	if (!bStarted || bFinished || !Item)
	{
		return;
	}
	LastActivityTime = Now();
	if (Item->bIsDishwashable && Trigger("First.Dish"))
	{
		return;
	}
	Trigger("First.Pickup");

	// Tutorials queue behind each other (priority 3), so the first lamp gets both lines.
	switch (Item->GetBarkKind())
	{
	case ELTTBarkPropKind::Furniture:
		// Picked up while lying on its side or upside down.
		if (Item->GetActorUpVector().Z < 0.6f)
		{
			FFormatOrderedArguments Args;
			Args.Add(Item->GetPropName());
			Trigger("First.RightIt", Args);
		}
		break;
	case ELTTBarkPropKind::Light:    Trigger("First.Light"); break;
	case ELTTBarkPropKind::Electric: Trigger("First.Cable"); break;
	case ELTTBarkPropKind::Camera:   Trigger("First.Lens"); break;
	case ELTTBarkPropKind::Shelf:    Trigger("First.Reel"); break;
	case ELTTBarkPropKind::Fabric:   Trigger("First.Costume"); break;
	case ELTTBarkPropKind::Broom:    Trigger("First.Broom"); break;
	default: break;
	}
}

void ALTTCleanupBarker::HandleReleased(ALTTInteractableBase* Item, bool bThrown)
{
	if (!bStarted || bFinished || !Item)
	{
		return;
	}
	LastActivityTime = Now();
	LastReleased = Item;
	if (bThrown)
	{
		FFormatOrderedArguments ThrowArgs;
		ThrowArgs.Add(ULTTInputDeviceSubsystem::Pick(LOCTEXT("ThrowKeyKb", "Right-click"), LOCTEXT("ThrowKeyPad", "Right trigger")));
		if (!Trigger("First.Throw", ThrowArgs))
		{
			Trigger("Throw");
		}
		return;
	}
	// PlaceHeld also reports a release (not thrown) while the prop slides home: look again once it has settled.
	FTimerHandle Handle;
	const TWeakObjectPtr<ALTTInteractableBase> Weak(Item);
	GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateUObject(this, &ALTTCleanupBarker::CheckWrongDrop, Weak), WrongDropCheckDelay, false);
}

void ALTTCleanupBarker::CheckWrongDrop(TWeakObjectPtr<ALTTInteractableBase> Item)
{
	ALTTInteractableBase* Prop = Item.Get();
	// Set dressing (not a cleanup task: some lamps, the camera) put back on its mark doesn't count as progress,
	// so its kind lines are said from here.
	if (!bFinished && IsValid(Prop) && Prop->IsPlaced() && !Prop->bIsCleanupTask)
	{
		LastActivityTime = Now();
		if (!OnKindCleaned(Prop))
		{
			Trigger("Place.Correct");
		}
		return;
	}
	if (NoWrongDropFor.Get() == Prop)
	{
		NoWrongDropFor.Reset();
		return;
	}
	if (bFinished || !IsValid(Prop) || Prop->IsPlaced() || Prop->IsHeld() || Prop->IsBrokenInPlace()
		|| !Prop->bIsCleanupTask || !Prop->HasHome() || Prop->HasNoHomeSpot())
	{
		return;
	}
	Trigger("Place.Wrong");
}

// ---- Console ----

static FAutoConsoleCommandWithWorldAndArgs GLTTDisableBarksCommand(
	TEXT("LTT.DisableBarks"),
	TEXT("LTT.DisableBarks [1|0] - 1 (default) stops Marlowe's Phase 1 barks and drops every queued line; 0 turns them back on."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		const bool bDisable = Args.Num() == 0 || Args[0].ToBool();
		int32 Count = 0;
		if (World)
		{
			for (TActorIterator<ALTTCleanupBarker> It(World); It; ++It)
			{
				It->SetBarksEnabled(!bDisable);
				++Count;
			}
		}
		UE_LOG(LogTemp, Display, TEXT("LTT.DisableBarks: barks %s on %d barker(s)."),
			bDisable ? TEXT("disabled") : TEXT("enabled"), Count);
	}));

#undef LOCTEXT_NAMESPACE
