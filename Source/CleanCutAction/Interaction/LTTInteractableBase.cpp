// Clean! Cut! Action!

#include "Interaction/LTTInteractableBase.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Interaction/LTTMessDirector.h"
#include "Interaction/LTTLightSwitchComponent.h"
#include "Interaction/LTTUsableComponent.h"
#include "Audio/LTTMusicPlayer.h"
#include "GeometryCollection/GeometryCollectionActor.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "GeometryCollection/GeometryCollectionObject.h"

namespace
{
	constexpr float PlaceTweenTime = 0.2f;
	const FLinearColor GhostColorIdle(0.25f, 0.75f, 1.f);      // carried: "it goes here"
	const FLinearColor GhostColorTargeted(1.f, 0.8f, 0.15f);    // aiming at it: place now
	const FLinearColor GhostColorBlocked(1.f, 0.38f, 0.f);      // orange: its table/plate isn't back yet
	const FLinearColor GhostColorSupport(1.f, 0.38f, 0.f);      // orange: the table/plate the held prop needs first
}

ALTTInteractableBase::ALTTInteractableBase()
{
	// Ticks only while sliding into its home spot.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	GhostMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/_CleanCutAction/Materials/M_GhostSilhouette.M_GhostSilhouette")));
	// Cooked through the "BreakFX" AlwaysCook entry in Config/DefaultGame.ini (nothing in content references it).
	// Lives outside /Game/_CleanCutAction/Destruction: that folder (the old per-prop GCs) is never cooked.
	GenericBreakCollection = TSoftObjectPtr<UGeometryCollection>(FSoftObjectPath(TEXT("/Game/_CleanCutAction/BreakFX/GC_Generic_Shatter.GC_Generic_Shatter")));
	// Cooked through a +MapsToCook entry in Config/DefaultGame.ini (C++ soft defaults aren't seen by the cooker).
	BrokenMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/_CleanCutAction/Materials/M_BrokenProp.M_BrokenProp")));
	// The mesh component lives in BP_MasterInteractables (Blueprint SCS) so content can be built before code.
}

void ALTTInteractableBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	Mesh = Cast<UStaticMeshComponent>(GetRootComponent());
	if (!Mesh)
	{
		Mesh = FindComponentByClass<UStaticMeshComponent>();
	}
	CachedLightSwitch = FindComponentByClass<ULTTLightSwitchComponent>();
	CachedAppliance = FindComponentByClass<ULTTUsableComponent>();
}

void ALTTInteractableBase::BeginPlay()
{
	Super::BeginPlay();
	if (!Mesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s has no Static Mesh component; it can't be picked up."), *GetName());
		bCanBePickedUp = false;
		return;
	}
	// The level is authored clean, so where the prop starts is where it belongs.
	RecordHome();

	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->BodyInstance.bUseCCD = true;
	ApplyPhysicsSettings();

	// Props resting on something simulate from the start so nothing is locked in place.
	// Wall/ceiling-mounted props (nothing underneath) stay put until grabbed or disturbed.
	if (bSimulatePhysicsOnBeginPlay && HasSupportBelow())
	{
		Mesh->SetSimulatePhysics(true);
		Mesh->PutAllRigidBodiesToSleep();
	}

	// A lamp is dark until it's put back (the switch found its light at home, before the mess moves it).
	if (CachedLightSwitch)
	{
		CachedLightSwitch->SetLightOn(bPlaced);
	}
}

bool ALTTInteractableBase::HasSupportBelow() const
{
	const FBoxSphereBounds& B = Mesh->Bounds;
	const FVector Start = B.Origin;
	const FVector End = B.Origin - FVector(0.f, 0.f, B.BoxExtent.Z + 6.f);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(LTTSupportTrace), false, this);
	FHitResult Hit;
	return GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
}

void ALTTInteractableBase::WakeTouchingProps()
{
	// Everything touching this prop (e.g. a cup on a table) loses its support when this prop is taken.
	const FBox Box = Mesh->Bounds.GetBox();
	FCollisionObjectQueryParams ObjectTypes;
	ObjectTypes.AddObjectTypesToQuery(ECC_PhysicsBody);
	ObjectTypes.AddObjectTypesToQuery(ECC_WorldDynamic);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(LTTWakeTouching), false, this);

	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, Box.GetCenter(), FQuat::Identity, ObjectTypes,
		FCollisionShape::MakeBox(Box.GetExtent() + FVector(4.f)), Params);

	for (const FOverlapResult& O : Overlaps)
	{
		ALTTInteractableBase* Other = Cast<ALTTInteractableBase>(O.GetActor());
		if (Other && Other != this && !Other->IsHeld() && !Other->IsPlaced() && Other->Mesh)
		{
			if (!Other->Mesh->IsSimulatingPhysics())
			{
				Other->Mesh->SetSimulatePhysics(true);
			}
			Other->Mesh->WakeAllRigidBodies();
		}
	}
}

void ALTTInteractableBase::ApplyPhysicsSettings()
{
	if (MassOverrideKg > 0.f)
	{
		Mesh->SetMassOverrideInKg(NAME_None, MassOverrideKg, true);
	}

	// Standing on a simulating body makes the first person camera jitter:
	// the player can't step up onto props, and small props don't collide with the player at all
	// (walk over cups and books instead of riding them). Big props (chairs, tables) still block.
	Mesh->CanCharacterStepUpOn = ECB_No;
	// Hit events drive the break-on-throw logic in BP_MasterInteractables.
	Mesh->SetNotifyRigidBodyCollision(true);
	constexpr float PawnIgnoreRadius = 30.f;
	Mesh->SetCollisionResponseToChannel(ECC_Pawn, GetPropRadius() < PawnIgnoreRadius ? ECR_Ignore : ECR_Block);
}

float ALTTInteractableBase::GetPropRadius() const
{
	if (Mesh && Mesh->GetStaticMesh())
	{
		const FBoxSphereBounds B = Mesh->GetStaticMesh()->GetBounds();
		const FVector Scale = Mesh->GetComponentScale().GetAbs();
		return (B.BoxExtent * Scale).Size();
	}
	return 15.f;
}

bool ALTTInteractableBase::UsesTwoHands() const
{
	return bAutoTwoHanded ? GetPropRadius() > TwoHandedRadius : bTwoHanded;
}

float ALTTInteractableBase::GetEffectiveThrowMultiplier() const
{
	if (ThrowMultiplier > 0.f)
	{
		return ThrowMultiplier;
	}
	// Small props (r ~ 10cm) fly full speed, chairs/tables (r ~ 60cm+) about a third.
	return FMath::Clamp(20.f / FMath::Max(GetPropRadius(), 1.f), 0.35f, 1.f);
}

void ALTTInteractableBase::BeginHold(AActor* InHolder)
{
	Holder = InHolder;
	bBreakOnLanding = false;
	bEverHeld = true;

	WakeTouchingProps();
	Mesh->SetSimulatePhysics(false);
	SavedCollisionProfile = Mesh->GetCollisionProfileName();
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Render like the first person arms: own FOV + depth scale so the prop never clips into walls.
	Mesh->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
	Mesh->SetRenderCustomDepth(false);

	UpdateGhost(true, false, IsHomeSupportReady());

	ReceivePickedUp(InHolder);
}

void ALTTInteractableBase::EndHold(const FVector& LinearVelocity, const FVector& AngularVelocityDeg)
{
	AActor* OldHolder = Holder;
	Holder = nullptr;
	UpdateGhost(false, false, false);

	Mesh->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::None);
	Mesh->SetCollisionProfileName(SavedCollisionProfile.IsNone() ? UCollisionProfile::PhysicsActor_ProfileName : SavedCollisionProfile);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetSimulatePhysics(true);
	ApplyPhysicsSettings();
	Mesh->SetPhysicsLinearVelocity(LinearVelocity);
	Mesh->SetPhysicsAngularVelocityInDegrees(AngularVelocityDeg);
	Mesh->WakeAllRigidBodies();

	ReceiveDropped(OldHolder, bLastReleaseWasThrow);
}

bool ALTTInteractableBase::IsSwitchableLamp() const
{
	return CachedLightSwitch && bPlaced && !bBrokenInPlace;
}

bool ALTTInteractableBase::IsUsableWhenPlaced() const
{
	return (CachedLightSwitch || CachedAppliance || bChangesMusic) && bPlaced && !bBrokenInPlace && !bTrashed;
}

bool ALTTInteractableBase::CanInteract_Implementation(AActor* InstigatorActor) const
{
	// Placed props are locked: no pickup, no outline. A placed lamp or appliance can still be used.
	if (IsUsableWhenPlaced())
	{
		return true;
	}
	return bCanBePickedUp && !IsHeld() && !bPlaced && !bPlacing;
}

ELTTInteractKind ALTTInteractableBase::GetInteractKind_Implementation() const
{
	return IsUsableWhenPlaced() ? ELTTInteractKind::Press : ELTTInteractKind::Pickup;
}

void ALTTInteractableBase::OnInteract_Implementation(AActor* InstigatorActor)
{
	if (!IsUsableWhenPlaced())
	{
		return;
	}
	if (CachedAppliance)
	{
		CachedAppliance->Use();
	}
	else if (CachedLightSwitch)
	{
		CachedLightSwitch->Toggle();
	}
	else if (bChangesMusic)
	{
		ChangeMusic();
	}
}

FText ALTTInteractableBase::GetPromptText_Implementation(AActor* InstigatorActor) const
{
	if (IsUsableWhenPlaced())
	{
		if (CachedAppliance)
		{
			return CachedAppliance->GetPrompt();
		}
		if (CachedLightSwitch)
		{
			return CachedLightSwitch->GetToggleText();
		}
		return ALTTMusicPlayer::IsMusicMuted() ? NSLOCTEXT("LTT", "MusicOnUse", "Turn music on") : NSLOCTEXT("LTT", "MusicNext", "Next song");
	}
	return FText::Format(NSLOCTEXT("LTT", "PromptFmt", "{0} {1}"), PromptVerb, GetPropName());
}

FText ALTTInteractableBase::GetPropName() const
{
	FText Name = DisplayName;
	if (Name.IsEmpty())
	{
		// "BP_Prop_CoffeeCup_01_C" -> "Coffee Cup"
		FString Raw = GetClass()->GetName();
		Raw.RemoveFromStart(TEXT("BP_Prop_"));
		Raw.RemoveFromEnd(TEXT("_C"));
		FString Pretty;
		for (int32 i = 0; i < Raw.Len(); ++i)
		{
			const TCHAR C = Raw[i];
			if (C == TEXT('_') || C == TEXT('-'))
			{
				if (!Pretty.EndsWith(TEXT(" "))) { Pretty.AppendChar(TEXT(' ')); }
				continue;
			}
			if (FChar::IsDigit(C))
			{
				continue; // drop variant numbers
			}
			if (FChar::IsUpper(C) && i > 0 && FChar::IsLower(Raw[i - 1]))
			{
				Pretty.AppendChar(TEXT(' '));
			}
			Pretty.AppendChar(C);
		}
		Pretty.TrimStartAndEndInline();
		while (Pretty.ReplaceInline(TEXT("  "), TEXT(" ")) > 0) {}
		Name = FText::FromString(Pretty.IsEmpty() ? GetActorNameOrLabel() : Pretty);
	}
	return Name;
}

// ---------------------------------------------------------------------------------------------
// Cleanup: home, silhouette, placing
// ---------------------------------------------------------------------------------------------

void ALTTInteractableBase::RecordHome()
{
	if (Mesh && !bHasHome)
	{
		HomeTransform = bUseCustomHome ? CustomHome : Mesh->GetComponentTransform();
		bHasHome = true;
	}
}

void ALTTInteractableBase::SetCustomHomeHere()
{
	Modify();
	CustomHome = GetActorTransform();
	bUseCustomHome = true;
	UpdateCustomHomePreview();
}

void ALTTInteractableBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateCustomHomePreview();
}

#if WITH_EDITOR
void ALTTInteractableBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	// Ticking Use Custom Home starts the home where the prop is now.
	if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(ALTTInteractableBase, bUseCustomHome)
		&& bUseCustomHome && CustomHome.Equals(FTransform::Identity))
	{
		CustomHome = GetActorTransform();
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UpdateCustomHomePreview();
}
#endif

void ALTTInteractableBase::UpdateCustomHomePreview()
{
#if WITH_EDITOR
	UWorld* World = GetWorld();
	const UStaticMeshComponent* Source = Cast<UStaticMeshComponent>(GetRootComponent());
	// Only once the prop has been moved away from its home: on top of it, the silhouette would catch the clicks.
	const bool bAway = !GetActorTransform().GetLocation().Equals(CustomHome.GetLocation(), 1.0)
		|| !GetActorQuat().Equals(CustomHome.GetRotation(), 0.01);
	const bool bShow = World && !World->IsGameWorld() && bUseCustomHome && bAway && Source && Source->GetStaticMesh();
	if (!bShow)
	{
		if (CustomHomePreview)
		{
			CustomHomePreview->DestroyComponent();
			CustomHomePreview = nullptr;
		}
		return;
	}
	if (!CustomHomePreview)
	{
		CustomHomePreview = NewObject<UStaticMeshComponent>(this, TEXT("CustomHomePreview"), RF_Transient | RF_TextExportTransient | RF_DuplicateTransient);
		CustomHomePreview->SetUsingAbsoluteLocation(true);
		CustomHomePreview->SetUsingAbsoluteRotation(true);
		CustomHomePreview->SetUsingAbsoluteScale(true);
		CustomHomePreview->SetMobility(EComponentMobility::Movable);
		CustomHomePreview->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CustomHomePreview->SetCastShadow(false);
		CustomHomePreview->SetCanEverAffectNavigation(false);
		CustomHomePreview->bIsEditorOnly = true;
		CustomHomePreview->bSelectable = false; // clicks go to the prop, never to its home silhouette
		CustomHomePreview->SetupAttachment(GetRootComponent());
		CustomHomePreview->RegisterComponent();
	}
	CustomHomePreview->SetStaticMesh(Source->GetStaticMesh());
	if (UMaterialInterface* GhostMat = GhostMaterial.LoadSynchronous())
	{
		for (int32 i = 0; i < CustomHomePreview->GetNumMaterials(); ++i)
		{
			CustomHomePreview->SetMaterial(i, GhostMat);
		}
	}
	CustomHomePreview->SetWorldTransform(CustomHome);
#endif
}

void ALTTInteractableBase::FindHomeSupport()
{
	HomeSupport = nullptr;
	if (!Mesh || !bHasHome)
	{
		return;
	}
	// Must run while the prop is still at home (before the earthquake).
	// A thin box just under the prop's bottom. An overlap (not a line trace from the centre) so thin props
	// like plates still find the table even when their centre sits inside the table's collision box.
	const FBoxSphereBounds& B = Mesh->Bounds;
	const float Bottom = static_cast<float>(B.Origin.Z - B.BoxExtent.Z);
	const FVector ProbeExtent(FMath::Max(B.BoxExtent.X * 0.5f, 1.f), FMath::Max(B.BoxExtent.Y * 0.5f, 1.f), 4.f);
	const FVector ProbeCenter(B.Origin.X, B.Origin.Y, Bottom - 2.f);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(LTTHomeSupport), false, this);
	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, ProbeCenter, FQuat::Identity,
		FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllObjects), FCollisionShape::MakeBox(ProbeExtent), Params);

	// The support is the cleanup prop with the highest top surface that is still below this prop's centre
	// (cup -> saucer, not the table under the saucer).
	float BestTop = -UE_BIG_NUMBER;
	for (const FOverlapResult& O : Overlaps)
	{
		ALTTInteractableBase* Under = Cast<ALTTInteractableBase>(O.GetActor());
		if (!Under || Under == this || !Under->Mesh || !Under->bIsCleanupTask)
		{
			continue;
		}
		const FBoxSphereBounds& UB = Under->Mesh->Bounds;
		const float UnderTop = static_cast<float>(UB.Origin.Z + UB.BoxExtent.Z);
		if (UB.Origin.Z >= B.Origin.Z || UnderTop <= BestTop)
		{
			continue;
		}
		// No loops (two props that each think the other is under them).
		bool bLoop = false;
		for (const ALTTInteractableBase* S = Under; IsValid(S); S = S->HomeSupport)
		{
			if (S == this)
			{
				bLoop = true;
				break;
			}
		}
		if (!bLoop)
		{
			BestTop = UnderTop;
			HomeSupport = Under;
		}
	}
}

bool ALTTInteractableBase::IsHomeSupportReady() const
{
	return GetBlockingSupport() == nullptr;
}

ALTTInteractableBase* ALTTInteractableBase::GetBlockingSupport() const
{
	// Walk the whole stack (croissant -> plate -> table). The lowest prop that isn't back yet is the one
	// to put back next. Broken/destroyed supports no longer block anything.
	ALTTInteractableBase* Lowest = nullptr;
	int32 Depth = 0;
	for (ALTTInteractableBase* S = HomeSupport; IsValid(S) && Depth < 16; S = S->HomeSupport, ++Depth)
	{
		if (!S->IsPlaced())
		{
			Lowest = S;
		}
	}
	return Lowest;
}

FVector ALTTInteractableBase::GetHomeCenter() const
{
	FVector LocalCenter = FVector::ZeroVector;
	if (Mesh && Mesh->GetStaticMesh())
	{
		LocalCenter = Mesh->GetStaticMesh()->GetBounds().Origin;
	}
	return HomeTransform.TransformPosition(LocalCenter);
}

float ALTTInteractableBase::GetSnapRadius() const
{
	return SnapRadius > 0.f ? SnapRadius : FMath::Clamp(GetPropRadius() * 0.75f, 18.f, 70.f);
}

bool ALTTInteractableBase::IsTargetingHome(const FVector& ViewLocation, const FVector& ViewDirection, float MaxDistance) const
{
	if (!bHasHome || bPlaced || !Mesh)
	{
		return false;
	}
	const FVector Home = GetHomeCenter();
	const float Radius = GetSnapRadius();

	// Walking the carried prop into its silhouette counts too.
	if (FVector::DistSquared(Mesh->Bounds.Origin, Home) <= FMath::Square(Radius))
	{
		return true;
	}

	const FVector Dir = ViewDirection.GetSafeNormal();
	const FVector ToHome = Home - ViewLocation;
	const float Along = FVector::DotProduct(ToHome, Dir);
	if (Along <= 0.f || Along > MaxDistance)
	{
		return false;
	}
	return (ToHome - Dir * Along).SizeSquared() <= FMath::Square(Radius);
}

bool ALTTInteractableBase::IsNearHome(const FVector& ViewLocation, const FVector& ViewDirection, float MaxDistance, float Margin) const
{
	if (!bHasHome || bPlaced || !Mesh)
	{
		return false;
	}
	const FVector Home = GetHomeCenter();
	const float Radius = GetSnapRadius() + FMath::Max(Margin, 0.f);
	if (FVector::DistSquared(Mesh->Bounds.Origin, Home) <= FMath::Square(Radius))
	{
		return true;
	}
	const FVector Dir = ViewDirection.GetSafeNormal();
	const FVector ToHome = Home - ViewLocation;
	const float Along = FVector::DotProduct(ToHome, Dir);
	if (Along <= 0.f || Along > MaxDistance + Margin)
	{
		return false;
	}
	return (ToHome - Dir * Along).SizeSquared() <= FMath::Square(Radius);
}

void ALTTInteractableBase::ArmBreakOnLanding()
{
	if (!bPlaced && !IsHeld())
	{
		bBreakOnLanding = true;
	}
}

void ALTTInteractableBase::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved,
	FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	// Trash that lands in (or hits) a bin is thrown away, before BP ReceiveHit gets a chance to break it.
	if (bIsTrash && bEverHeld && !bTrashed && !IsHeld() && !bPlacing)
	{
		const ALTTInteractableBase* OtherProp = Cast<ALTTInteractableBase>(Other);
		if (OtherProp && OtherProp->bIsTrashBin && !OtherProp->IsHeld())
		{
			ThrowAwayInto(Other);
			return;
		}
	}
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit); // BP ReceiveHit
	if (!bBreakOnLanding || IsHeld() || bPlaced || bPlacing || !IsValid(this) || IsActorBeingDestroyed())
	{
		return;
	}
	bBreakOnLanding = false;
	// Break with the prop's own Geometry Collection: BP_MasterInteractables' BreakFromImpact event spawns
	// BP_BreakDebris with BreakCollection (soft props with bBreakOnThrow off don't break).
	static const FName BreakFromImpactName(TEXT("BreakFromImpact"));
	if (UFunction* Break = FindFunction(BreakFromImpactName); Break && Break->ParmsSize == 0)
	{
		ProcessEvent(Break, nullptr);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: no BreakFromImpact event; destroying it."), *GetName());
		Destroy(); // still counts as broken
	}
}

void ALTTInteractableBase::UpdateGhost(bool bVisible, bool bTargeted, bool bReady, bool bAsRequiredSupport)
{
	const bool bShow = bVisible && bHasHome && bIsCleanupTask && !bIsTrash && !bPlaced && Mesh && Mesh->GetStaticMesh();
	if (!bShow)
	{
		if (Ghost)
		{
			Ghost->SetVisibility(false);
		}
		return;
	}

	if (!Ghost)
	{
		Ghost = NewObject<UStaticMeshComponent>(this, TEXT("HomeGhost"), RF_Transient);
		Ghost->SetUsingAbsoluteLocation(true);
		Ghost->SetUsingAbsoluteRotation(true);
		Ghost->SetUsingAbsoluteScale(true);
		Ghost->SetMobility(EComponentMobility::Movable);
		Ghost->SetStaticMesh(Mesh->GetStaticMesh());
		Ghost->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Ghost->SetCastShadow(false);
		Ghost->SetCanEverAffectNavigation(false);
		if (UMaterialInterface* Base = GhostMaterial.LoadSynchronous())
		{
			GhostMID = UMaterialInstanceDynamic::Create(Base, this);
			for (int32 i = 0; i < Ghost->GetNumMaterials(); ++i)
			{
				Ghost->SetMaterial(i, GhostMID);
			}
		}
		else
		{
			// Without the ghost material the silhouette would render with the prop's own textures.
			// Packaged builds only have it through the +MapsToCook entry in Config/DefaultGame.ini.
			UE_LOG(LogTemp, Warning, TEXT("%s: ghost material %s didn't load (not cooked?)."), *GetName(), *GhostMaterial.ToString());
		}
		Ghost->RegisterComponent();
		Ghost->SetWorldTransform(HomeTransform);
	}

	if (GhostMID)
	{
		const FLinearColor C = bAsRequiredSupport ? GhostColorSupport
			: !bReady ? GhostColorBlocked : (bTargeted ? GhostColorTargeted : GhostColorIdle);
		GhostMID->SetVectorParameterValue(TEXT("Color"), C);
	}
	Ghost->SetVisibility(true);
}

void ALTTInteractableBase::PlaceAtHome()
{
	if (!Mesh || !bHasHome || bPlaced || bPlacing)
	{
		return;
	}
	Holder = nullptr;
	UpdateGhost(false, false, false);

	Mesh->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::None);
	Mesh->SetSimulatePhysics(false);

	bPlacing = true;
	PlaceElapsed = 0.f;
	PlaceFrom = Mesh->GetComponentTransform();
	SetActorTickEnabled(true);
}

FVector ALTTInteractableBase::GetBinOpening(const AActor* Bin)
{
	if (!Bin)
	{
		return FVector::ZeroVector;
	}
	FVector Origin, Extent;
	Bin->GetActorBounds(true, Origin, Extent);
	return Origin + FVector(0.f, 0.f, Extent.Z * 0.6f);
}

ALTTInteractableBase* ALTTInteractableBase::FindNearestTrashBin(const UObject* WorldContext, const FVector& From)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}
	ALTTInteractableBase* Best = nullptr;
	double BestDistSq = TNumericLimits<double>::Max();
	for (TActorIterator<ALTTInteractableBase> It(World); It; ++It)
	{
		if (It->bIsTrashBin && !It->IsActorBeingDestroyed())
		{
			const double D = FVector::DistSquared(GetBinOpening(*It), From);
			if (D < BestDistSq)
			{
				BestDistSq = D;
				Best = *It;
			}
		}
	}
	return Best;
}

void ALTTInteractableBase::ThrowAwayInto(AActor* Bin)
{
	if (bTrashed || !Bin || !Mesh)
	{
		return;
	}
	bTrashed = true;
	bPlaced = true; // counts as cleaned up (IsPlaced), and nothing picks it up, focuses or hints it any more
	bPlacing = false;
	bBreakOnLanding = false;
	Mesh->SetSimulatePhysics(false);
	SetActorEnableCollision(false);
	Mesh->SetRenderCustomDepth(false);
	if (Ghost)
	{
		Ghost->SetVisibility(false);
	}
	TrashFrom = GetActorTransform();
	TrashTarget = GetBinOpening(Bin);
	TrashElapsed = 0.f;
	bTrashing = true;
	SetActorTickEnabled(true);

	ReceiveThrownAway(Bin);
	if (ALTTMessDirector* Director = ALTTMessDirector::Get(this))
	{
		Director->NotifyPlaced(this);
	}
}

bool ALTTInteractableBase::ChangeMusic()
{
	if (ALTTMusicPlayer::IsMusicMuted())
	{
		ALTTMusicPlayer::SetMusicMuted(false); // resumes the music
		return true;
	}
	for (TActorIterator<ALTTMusicPlayer> It(GetWorld()); It; ++It)
	{
		It->Skip(); // crossfades to the next track (starts the music if it wasn't playing)
		return true;
	}
	return false;
}

void ALTTInteractableBase::ShiftHome(FVector Delta)
{
	if (Delta.IsNearlyZero())
	{
		return;
	}
	HomeTransform.AddToTranslation(Delta);
	if (Ghost)
	{
		Ghost->SetWorldTransform(HomeTransform);
	}
	if (bPlaced || bPlacing)
	{
		AddActorWorldOffset(Delta, false, nullptr, ETeleportType::TeleportPhysics);
	}
	if (bPlacing)
	{
		PlaceFrom.AddToTranslation(Delta); // the slide-in tween follows too
	}
}

void ALTTInteractableBase::SnapToHome()
{
	if (!Mesh || !bHasHome || bPlaced)
	{
		return;
	}
	Mesh->SetSimulatePhysics(false);
	PlaceFrom = HomeTransform;
	FinishPlacing();
}

void ALTTInteractableBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bTrashing)
	{
		TrashElapsed += DeltaSeconds;
		const float A = FMath::Clamp(TrashElapsed / FMath::Max(TrashTweenTime, 0.05f), 0.f, 1.f);
		const float Ease = A * A; // InQuad: drops into the bin
		FTransform T = TrashFrom;
		T.SetLocation(FMath::Lerp(TrashFrom.GetLocation(), TrashTarget, Ease));
		T.SetScale3D(TrashFrom.GetScale3D() * FMath::Lerp(1.f, 0.05f, Ease));
		SetActorTransform(T, false, nullptr, ETeleportType::TeleportPhysics);
		if (A >= 1.f)
		{
			bTrashing = false;
			SetActorHiddenInGame(true);
			SetActorTickEnabled(false);
		}
		return;
	}

	if (!bPlacing)
	{
		SetActorTickEnabled(false);
		return;
	}

	PlaceElapsed += DeltaSeconds;
	const float A = FMath::Clamp(PlaceElapsed / PlaceTweenTime, 0.f, 1.f);
	const float Ease = 1.f - FMath::Pow(1.f - A, 3.f); // OutCubic

	FTransform T;
	T.SetLocation(FMath::Lerp(PlaceFrom.GetLocation(), HomeTransform.GetLocation(), Ease));
	T.SetRotation(FQuat::Slerp(PlaceFrom.GetRotation(), HomeTransform.GetRotation(), Ease));
	T.SetScale3D(FMath::Lerp(PlaceFrom.GetScale3D(), HomeTransform.GetScale3D(), Ease));
	SetActorTransform(T, false, nullptr, ETeleportType::TeleportPhysics);

	if (A >= 1.f)
	{
		FinishPlacing();
	}
}

void ALTTInteractableBase::LockAtHome()
{
	bPlacing = false;
	bPlaced = true;
	bBreakOnLanding = false;
	SetActorTickEnabled(false);

	// Locked: kinematic, solid, never woken or picked up again.
	SetActorTransform(HomeTransform, false, nullptr, ETeleportType::TeleportPhysics);
	Mesh->SetSimulatePhysics(false);
	Mesh->SetCollisionProfileName(SavedCollisionProfile.IsNone() ? Mesh->GetCollisionProfileName() : SavedCollisionProfile);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ApplyPhysicsSettings();
	Mesh->SetRenderCustomDepth(false);
	SetLifeSpan(0.f);

	if (Ghost)
	{
		Ghost->DestroyComponent();
		Ghost = nullptr;
	}
}

void ALTTInteractableBase::FinishPlacing()
{
	LockAtHome();
	if (CachedLightSwitch)
	{
		CachedLightSwitch->SetLightOn(true); // back on its mark: the lamp lights up
	}

	ReceivePlaced();
	if (ALTTMessDirector* Director = ALTTMessDirector::Get(this))
	{
		Director->NotifyPlaced(this);
	}
}

void ALTTInteractableBase::FellOutOfWorld(const UDamageType& DmgType)
{
	// Never lose a cleanup prop: drop it back near home instead of destroying it.
	if (!bHasHome || !Mesh)
	{
		Super::FellOutOfWorld(DmgType);
		return;
	}
	const FVector Offset(FMath::FRandRange(-60.f, 60.f), FMath::FRandRange(-60.f, 60.f), GetPropRadius() + 40.f);
	SetActorLocation(HomeTransform.GetLocation() + Offset, false, nullptr, ETeleportType::TeleportPhysics);
	Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
}

void ALTTInteractableBase::OnFocusBegin_Implementation(AActor* InstigatorActor)
{
	// Stencil 1 = focus white (PP_Outline, Docs/02 §5).
	Mesh->SetRenderCustomDepth(true);
	Mesh->SetCustomDepthStencilValue(1);
	ReceiveFocusChanged(true);
}

void ALTTInteractableBase::OnFocusEnd_Implementation(AActor* InstigatorActor)
{
	Mesh->SetRenderCustomDepth(false);
	ReceiveFocusChanged(false);
}

AActor* ALTTInteractableBase::ShatterGeneric(FVector ImpactPoint, bool bDestroyProp)
{
	UGeometryCollection* Collection = GenericBreakCollection.LoadSynchronous();
	UWorld* World = GetWorld();
	if (!Collection || !World || !Mesh || !Mesh->GetStaticMesh())
	{
		return nullptr;
	}

	const FTransform MeshXf = Mesh->GetComponentTransform();
	const FBoxSphereBounds PropLocal = Mesh->GetStaticMesh()->GetBounds();

	AGeometryCollectionActor* Debris = World->SpawnActorDeferred<AGeometryCollectionActor>(
		AGeometryCollectionActor::StaticClass(), MeshXf, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	UGeometryCollectionComponent* Pieces = Debris ? Debris->GetGeometryCollectionComponent() : nullptr;
	if (!Pieces)
	{
		return nullptr;
	}
	Pieces->SetRestCollection(Collection);

	// Stretch the cube to the prop's box (per axis), centred on the prop's bounds.
	const FBoxSphereBounds CubeLocal = Pieces->CalcBounds(FTransform::Identity);
	FVector Scale(
		PropLocal.BoxExtent.X / FMath::Max(CubeLocal.BoxExtent.X, 0.1),
		PropLocal.BoxExtent.Y / FMath::Max(CubeLocal.BoxExtent.Y, 0.1),
		PropLocal.BoxExtent.Z / FMath::Max(CubeLocal.BoxExtent.Z, 0.1));
	Scale = (Scale * MeshXf.GetScale3D()).ComponentMax(FVector(0.01));
	const FQuat Rotation = MeshXf.GetRotation();
	const FVector Center = MeshXf.TransformPosition(PropLocal.Origin);
	const FVector Location = Center - Rotation.RotateVector(CubeLocal.Origin * Scale);

	// Pieces shouldn't trip the player.
	Pieces->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Debris->FinishSpawning(FTransform(Rotation, Location, Scale));

	// The prop's look: every slot (outside and inside faces) uses its first material.
	if (UMaterialInterface* PropMaterial = Mesh->GetMaterial(0))
	{
		for (int32 i = 0; i < Pieces->GetNumMaterials(); ++i)
		{
			Pieces->SetMaterial(i, PropMaterial);
		}
	}

	Pieces->CrumbleActiveClusters();
	Pieces->AddRadialImpulse(ImpactPoint, GetPropRadius() * 2.f + 50.f, ShatterImpulse, RIF_Linear, true);
	Debris->SetLifeSpan(DebrisLifeSpan);

	if (bDestroyProp)
	{
		BreakAway(); // destroyed (counted as broken), or back home in red if other props rest on it
	}
	return Debris;
}

void ALTTInteractableBase::K2_DestroyActor()
{
	// BP_MasterInteractables' BreakFromImpact ends with DestroyActor after spawning its debris.
	BreakAway();
}

void ALTTInteractableBase::BreakAway()
{
	if (bBrokenInPlace || bTrashed || IsActorBeingDestroyed())
	{
		return;
	}
	ALTTMessDirector* Director = ALTTMessDirector::Get(this);
	if (!Mesh || !bHasHome || !bIsCleanupTask || !Director || !Director->HasDependents(this))
	{
		Destroy(); // mess director counts it as broken (OnDestroyed)
		return;
	}

	// Cups and plates still need this table: the debris has flown, now a red "broken" copy stands back on its mark
	// so they can be put back on it and the set can still be finished.
	bBrokenInPlace = true;
	Holder = nullptr;
	Mesh->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::None);
	LockAtHome();
	ApplyBrokenLook();
	if (CachedLightSwitch)
	{
		CachedLightSwitch->SetLightOn(false); // broken lamps stay dark
	}

	// Never break again (the BP checks bBreakOnThrow before spawning debris).
	if (FBoolProperty* BreakFlag = FindFProperty<FBoolProperty>(GetClass(), TEXT("bBreakOnThrow")))
	{
		BreakFlag->SetPropertyValue_InContainer(this, false);
	}

	Director->NotifyBrokenInPlace(this);
}

void ALTTInteractableBase::ApplyBrokenLook()
{
	UMaterialInterface* Base = BrokenMaterial.LoadSynchronous();
	if (!Base || !Mesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: broken material %s didn't load (not cooked?)."), *GetName(), *BrokenMaterial.ToString());
		return;
	}
	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, this);
	MID->SetVectorParameterValue(TEXT("Color"), BrokenColor);
	for (int32 i = 0; i < Mesh->GetNumMaterials(); ++i)
	{
		Mesh->SetMaterial(i, MID);
	}
}
