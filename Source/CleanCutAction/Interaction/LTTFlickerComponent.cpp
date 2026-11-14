// Clean! Cut! Action!

#include "Interaction/LTTFlickerComponent.h"
#include "Components/LightComponent.h"
#include "Components/MeshComponent.h"
#include "Engine/Light.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

ULTTFlickerComponent::ULTTFlickerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void ULTTFlickerComponent::BeginPlay()
{
	Super::BeginPlay();
	AActor* Owner = GetOwner();
	UMeshComponent* Mesh = Owner ? Cast<UMeshComponent>(Owner->GetRootComponent()) : nullptr;
	if (!Mesh && Owner)
	{
		Mesh = Owner->FindComponentByClass<UMeshComponent>();
	}

	// Glow: a dynamic instance for each material slot that has one of the parameters.
	if (Mesh)
	{
		for (int32 i = 0; i < Mesh->GetNumMaterials(); ++i)
		{
			UMaterialInterface* Mat = Mesh->GetMaterial(i);
			if (!Mat)
			{
				continue;
			}
			TArray<TPair<FName, float>> Found;
			for (const FName& Param : EmissiveParameters)
			{
				float Value = 0.f;
				if (Mat->GetScalarParameterValue(FHashedMaterialParameterInfo(Param), Value) && Value > 0.f)
				{
					Found.Emplace(Param, Value);
				}
			}
			if (Found.IsEmpty())
			{
				continue;
			}
			UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Mat);
			if (!MID)
			{
				MID = Mesh->CreateAndSetMaterialInstanceDynamic(i);
			}
			if (MID)
			{
				GlowMIDs.Add(MID);
				for (const TPair<FName, float>& P : Found)
				{
					GlowParams.Add({ MID, P.Key, P.Value });
				}
			}
		}
	}

	// Lights: the assigned ones, or the nearest one.
	FlickerLights.RemoveAll([](const TObjectPtr<ALight>& L) { return !IsValid(L); });
	if (FlickerLights.IsEmpty() && AutoFindRadius > 0.f && GetWorld())
	{
		const FVector Center = Mesh ? Mesh->Bounds.Origin : Owner->GetActorLocation();
		ALight* Best = nullptr;
		double BestDistSq = FMath::Square(AutoFindRadius);
		for (TActorIterator<ALight> It(GetWorld()); It; ++It)
		{
			const double D = FVector::DistSquared(It->GetActorLocation(), Center);
			if (D <= BestDistSq)
			{
				BestDistSq = D;
				Best = *It;
			}
		}
		if (Best)
		{
			FlickerLights.Add(Best);
		}
	}
	for (const ALight* Light : FlickerLights)
	{
		BaseLightIntensity.Add(Light && Light->GetLightComponent() ? Light->GetLightComponent()->Intensity : 0.f);
	}

	NoiseSeed = FMath::FRandRange(0.f, 1000.f);
	NextEventTime = RandIn(BurstInterval);
	if (GlowParams.IsEmpty() && FlickerLights.IsEmpty())
	{
		SetComponentTickEnabled(false); // nothing to flicker
	}
}

void ULTTFlickerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bFlickerEnabled)
	{
		Apply(1.f);
		return;
	}
	Clock += DeltaTime;

	// Burst state machine: wait -> dip -> gap -> dip ... -> wait.
	if (Clock >= NextEventTime)
	{
		if (bInDip)
		{
			bInDip = false;
			NextEventTime = Clock + (DipsLeft > 0 ? RandIn(DipGap) : RandIn(BurstInterval));
		}
		else
		{
			if (DipsLeft <= 0)
			{
				DipsLeft = FMath::RandRange(FMath::Max(1, DipsPerBurst.X), FMath::Max(1, DipsPerBurst.Y));
			}
			--DipsLeft;
			bInDip = true;
			DipLevel = RandIn(DipBrightness);
			NextEventTime = Clock + RandIn(DipDuration);
		}
	}

	const float Wobble = 1.f + Shimmer * FMath::PerlinNoise1D(NoiseSeed + Clock * 6.f);
	Apply((bInDip ? DipLevel : 1.f) * Wobble);
}

void ULTTFlickerComponent::Apply(float Brightness)
{
	for (const FGlowParam& P : GlowParams)
	{
		if (UMaterialInstanceDynamic* MID = P.MID.Get())
		{
			MID->SetScalarParameterValue(P.Name, P.BaseValue * Brightness);
		}
	}
	for (int32 i = 0; i < FlickerLights.Num(); ++i)
	{
		ALight* Light = FlickerLights[i];
		if (IsValid(Light) && Light->GetLightComponent() && BaseLightIntensity.IsValidIndex(i))
		{
			Light->GetLightComponent()->SetIntensity(BaseLightIntensity[i] * Brightness);
		}
	}
}
