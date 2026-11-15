// Clean! Cut! Action! - Placeable mirror. A scene capture renders the view reflected across the mirror plane,
// and the surface shows it with screen-space UVs, so the reflection lines up from any angle and at any size.
// It only captures while the surface is on screen and the player is within UpdateDistance.
// The surface faces the actor's +X (forward arrow), and the pivot is the centre of the glass.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LTTMirror.generated.h"

class UStaticMeshComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class UMaterialInterface;
class UMaterialInstanceDynamic;

UCLASS(Blueprintable, meta = (DisplayName = "LTT Mirror"))
class CLEANCUTACTION_API ALTTMirror : public AActor
{
	GENERATED_BODY()

public:
	ALTTMirror();

	/** Width (Y) and height (Z) of the glass in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror", meta = (ClampMin = "5"))
	FVector2D MirrorSize = FVector2D(80.f, 160.f);

	/** Frame border around the glass, in cm. 0 = no frame. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror", meta = (ClampMin = "0"))
	float FrameWidth = 6.f;

	/** How far the frame sticks out behind the glass, in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror", meta = (ClampMin = "0.5"))
	float FrameDepth = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror")
	TObjectPtr<UMaterialInterface> FrameMaterial;

	/** Material for the glass. Needs a texture parameter named by TextureParameterName. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror")
	TObjectPtr<UMaterialInterface> MirrorMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror")
	FLinearColor Tint = FLinearColor(0.92f, 0.95f, 0.97f);

	/** Reflection resolution as a fraction of the game viewport. Lower = cheaper (it's a second scene render). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror|Performance", meta = (ClampMin = "0.1", ClampMax = "1"))
	float ResolutionScale = 0.5f;

	/** Longest edge of the reflection texture, in pixels. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror|Performance", meta = (ClampMin = "64"))
	int32 MaxResolution = 1024;

	/** The mirror stops updating when the camera is farther away than this (cm). 0 = always. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror|Performance", meta = (ClampMin = "0"))
	float UpdateDistance = 2000.f;

	/** Objects farther than this from the reflected camera aren't drawn in the reflection (cm). 0 = no limit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror|Performance", meta = (ClampMin = "0"))
	float MaxReflectionDistance = 3000.f;

	/** Draw dynamic shadows in the reflection (costs more). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror|Performance")
	bool bReflectShadows = true;

	UFUNCTION(BlueprintPure, Category = "Mirror")
	UTextureRenderTarget2D* GetRenderTarget() const { return RenderTarget; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mirror")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mirror")
	TObjectPtr<UStaticMeshComponent> Glass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mirror")
	TObjectPtr<UStaticMeshComponent> Frame;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mirror")
	TObjectPtr<USceneCaptureComponent2D> Capture;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> RenderTarget;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GlassMID;

	static const FName TextureParameterName;
	static const FName TintParameterName;

	void LayoutPieces();
	void EnsureRenderTarget();
	bool GetViewerCamera(FVector& OutLocation, FRotator& OutRotation, float& OutFOV) const;
};
