// Clean! Cut! Action!

#include "Interaction/LTTInteractableBase.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"

ALTTInteractableBase::ALTTInteractableBase()
{
	PrimaryActorTick.bCanEverTick = false;
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
		if (Other && Other != this && !Other->IsHeld() && Other->Mesh)
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

	WakeTouchingProps();
	Mesh->SetSimulatePhysics(false);
	SavedCollisionProfile = Mesh->GetCollisionProfileName();
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Render like the first person arms: own FOV + depth scale so the prop never clips into walls.
	Mesh->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
	Mesh->SetRenderCustomDepth(false);

	ReceivePickedUp(InHolder);
}

void ALTTInteractableBase::EndHold(const FVector& LinearVelocity, const FVector& AngularVelocityDeg)
{
	AActor* OldHolder = Holder;
	Holder = nullptr;

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

bool ALTTInteractableBase::CanInteract_Implementation(AActor* InstigatorActor) const
{
	return bCanBePickedUp && !IsHeld();
}

FText ALTTInteractableBase::GetPromptText_Implementation(AActor* InstigatorActor) const
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
	return FText::Format(NSLOCTEXT("LTT", "PromptFmt", "{0} {1}"), PromptVerb, Name);
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
