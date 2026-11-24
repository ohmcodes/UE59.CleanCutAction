// Clean! Cut! Action!

#include "Interaction/LTTLightFixture.h"
#include "Interaction/LTTLightSwitchComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"

ALTTLightFixture::ALTTLightFixture()
{
	PrimaryActorTick.bCanEverTick = false;

	LampMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LampMesh"));
	LampMesh->SetMobility(EComponentMobility::Static);
	LampMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	RootComponent = LampMesh;

	LightSwitch = CreateDefaultSubobject<ULTTLightSwitchComponent>(TEXT("LightSwitch"));
}

void ALTTLightFixture::ToggleLight()
{
	LightSwitch->Toggle();
	ReceiveToggled(LightSwitch->IsLightOn());
}

FText ALTTLightFixture::GetPromptText_Implementation(AActor* InstigatorActor) const
{
	return LightSwitch->GetToggleText();
}

void ALTTLightFixture::OnFocusBegin_Implementation(AActor* InstigatorActor)
{
	// Stencil 1 = focus outline, same as the props.
	LampMesh->SetRenderCustomDepth(true);
	LampMesh->SetCustomDepthStencilValue(1);
}

void ALTTLightFixture::OnFocusEnd_Implementation(AActor* InstigatorActor)
{
	LampMesh->SetRenderCustomDepth(false);
}
