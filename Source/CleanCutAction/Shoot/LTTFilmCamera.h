// Clean! Cut! Action! - The film camera of the Phase 2 shoot and its frame cone (Docs/04 §1.2, §4).
// Place one in the level, pointing its red X arrow at the actors. Put an anchor actor (a Target Point or
// Camera Actor) where the camera stands in each act and list them in ActAnchors ("A", "B", "C").
// The cone is what the camera sees: the player loses take meter while standing in it.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LTTFilmCamera.generated.h"

class UStaticMeshComponent;
class UArrowComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

UENUM(BlueprintType)
enum class ELTTConeLook : uint8
{
	Hidden,
	/** Rolling, nobody in shot. */
	Idle,
	/** The player is in shot. */
	Alert,
	/** The slate: the one time the player may stand in shot. */
	Allowed
};

UCLASS(Blueprintable)
class CLEANCUTACTION_API ALTTFilmCamera : public AActor
{
	GENERATED_BODY()

public:
	ALTTFilmCamera();

	static ALTTFilmCamera* Get(const UObject* WorldContext);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Film Camera")
	TObjectPtr<USceneComponent> Root;

	/** The camera prop. Set any mesh; it has no effect on the frame test. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Film Camera")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	/** The see-through frame cone (engine cone stretched to the frustum). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Film Camera")
	TObjectPtr<UStaticMeshComponent> ConeMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Film Camera")
	TObjectPtr<UArrowComponent> Arrow;

	// ---- Frame ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frame", meta = (ClampMin = "5", ClampMax = "120"))
	float HorizontalFOV = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frame", meta = (ClampMin = "0.5"))
	float AspectRatio = 1.777f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frame", meta = (ClampMin = "0"))
	float NearDist = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frame", meta = (ClampMin = "100"))
	float MaxDist = 2200.f;

	/** Walls between the camera and a point hide it from the shot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frame")
	bool bCheckOcclusion = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frame", meta = (EditCondition = "bCheckOcclusion"))
	TEnumAsByte<ECollisionChannel> OcclusionChannel = ECC_Visibility;

	UFUNCTION(BlueprintPure, Category = "Frame")
	bool IsPointInFrame(const FVector& Point, const AActor* IgnoreActor = nullptr) const;

	/** Tests the pawn's eyes and its centre. */
	UFUNCTION(BlueprintPure, Category = "Frame")
	bool IsPawnInFrame(const APawn* Pawn) const;

	// ---- Cone look ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cone")
	bool bShowCone = true;

	/** Translucent material with a vector parameter "Color". Empty = M_GhostSilhouette. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cone")
	TSoftObjectPtr<UMaterialInterface> ConeMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cone")
	FLinearColor IdleColor = FLinearColor(1.f, 0.85f, 0.3f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cone")
	FLinearColor AlertColor = FLinearColor(1.f, 0.08f, 0.04f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cone")
	FLinearColor AllowedColor = FLinearColor(0.2f, 1.f, 0.3f);

	UFUNCTION(BlueprintCallable, Category = "Cone")
	void SetConeLook(ELTTConeLook Look);

	// ---- Act positions ----

	/** Where the camera stands in each act: "A", "B", "C" -> an actor placed in the level (its transform is used). */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Acts")
	TMap<FName, TObjectPtr<AActor>> ActAnchors;

	/** Seconds the camera takes to travel between act positions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acts", meta = (ClampMin = "0"))
	float MoveTime = 3.f;

	/** Starts the move to the act's anchor. Returns the seconds it will take (0 if there is no anchor or bInstant). */
	UFUNCTION(BlueprintCallable, Category = "Acts")
	float MoveToAct(FName Act, bool bInstant = false);

	UFUNCTION(BlueprintPure, Category = "Acts")
	bool IsMoving() const { return bMoving; }

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void UpdateConeShape();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ConeMID;

	bool bMoving = false;
	float MoveElapsed = 0.f;
	FTransform MoveFrom;
	FTransform MoveTarget;
};
