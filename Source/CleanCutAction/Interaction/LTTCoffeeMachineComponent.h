// Clean! Cut! Action! - Espresso machine routine for a placed prop (BP_Prop_EspressoMachine):
// grind (portafilter + grounds fill up, grind sound) -> a mug pops onto the drip tray -> coffee pours into it
// (stream + rising coffee, fill sound) -> buzz. The finished mug stays until the next brew.
// All the pieces are built at BeginPlay from the meshes / materials set here; positions are in the machine's space.

#pragma once

#include "CoreMinimal.h"
#include "Interaction/LTTUsableComponent.h"
#include "LTTCoffeeMachineComponent.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;

UENUM()
enum class ELTTCoffeePhase : uint8
{
	Idle,
	Grinding,
	MugIn,
	Filling
};

UCLASS(ClassGroup = (LTT), meta = (BlueprintSpawnableComponent))
class CLEANCUTACTION_API ULTTCoffeeMachineComponent : public ULTTUsableComponent
{
	GENERATED_BODY()

public:
	ULTTCoffeeMachineComponent();

	// ---- Layout (machine space, cm) ----

	/** Under the group head: the top of the portafilter basket. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coffee|Layout")
	FVector GroupHead = FVector(-1.f, 9.f, 31.f);

	/** Drip tray under the spout: where the mug's base sits. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coffee|Layout")
	FVector CupSpot = FVector(-1.f, 10.f, 9.5f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coffee|Layout")
	FRotator PortafilterRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coffee|Layout")
	FRotator CupRotation = FRotator(0.f, 90.f, 0.f);

	/** Basket depth below the group head (grounds fill this, the stream starts at its bottom). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coffee|Layout", meta = (ClampMin = "1"))
	float BasketDepth = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coffee|Layout", meta = (ClampMin = "1"))
	float BasketRadius = 3.5f;

	// ---- Look ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coffee|Look")
	TObjectPtr<UStaticMesh> PortafilterMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coffee|Look")
	TObjectPtr<UStaticMesh> CupMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coffee|Look")
	TObjectPtr<UMaterialInterface> GroundsMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coffee|Look")
	TObjectPtr<UMaterialInterface> CoffeeMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coffee|Look")
	TObjectPtr<UMaterialInterface> StreamMaterial;

	/** Stream thickness (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coffee|Look", meta = (ClampMin = "0.1"))
	float StreamWidth = 0.8f;

	// ---- Timing / sound ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coffee", meta = (ClampMin = "0.1"))
	float GrindTime = 1.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coffee", meta = (ClampMin = "0.1"))
	float FillTime = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coffee")
	TObjectPtr<USoundBase> GrindSound;

	/** Loops while the coffee pours. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coffee")
	TObjectPtr<USoundBase> FillSound;

	/** Plays once when the coffee is ready. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coffee")
	TObjectPtr<USoundBase> DoneSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coffee")
	FText IdlePrompt = NSLOCTEXT("LTT", "CoffeeMake", "Make coffee");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coffee")
	FText BusyPrompt = NSLOCTEXT("LTT", "CoffeeBusy", "Brewing...");

	// ULTTUsableComponent
	virtual bool Use() override;
	virtual bool IsBusy() const override { return Phase != ELTTCoffeePhase::Idle; }
	virtual FText GetPrompt() const override { return IsBusy() ? BusyPrompt : IdlePrompt; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UStaticMeshComponent* MakePiece(const TCHAR* Name, UStaticMesh* Mesh, UMaterialInterface* Material);
	void SetGrounds(float Alpha);
	void SetCoffeeLevel(float Alpha);
	void StopLoop();
	void MeasureCup();

	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Portafilter;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Grounds;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Cup;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Coffee;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Stream;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> LoopAudio;

	/** Mug bottom / fill range inside the mug (machine space z), and the coffee disc radius. */
	float CupBottomZ = 0.f;
	float CoffeeLowZ = 0.f;
	float CoffeeHighZ = 0.f;
	float CoffeeRadius = 2.f;

	ELTTCoffeePhase Phase = ELTTCoffeePhase::Idle;
	float PhaseTime = 0.f;
};
