// Clean! Cut! Action! - Placeable mirror (see LTTMirror.h).

#include "World/LTTMirror.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "StereoRendering.h"
#include "GameFramework/WorldSettings.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// A mirror never shows what's behind its plane. The reflected camera sits behind the glass, so anything back
	// there (the wall the mirror hangs on, the street outside) would block the view. Hide every actor whose bounds
	// lie entirely behind the plane. Actors that cross it (floor, ceiling, side walls) stay; the near plane set per
	// capture handles those.
	void HideActorsBehindMirror(USceneCaptureComponent2D* Capture, const AActor* Mirror)
	{
		UWorld* World = Mirror->GetWorld();
		if (!World)
		{
			return;
		}
		const FVector PlanePoint = Mirror->GetActorLocation();
		const FVector Normal = Mirror->GetActorForwardVector();
		constexpr float Margin = 0.5f;

		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (Actor == Mirror || Actor->IsA<APawn>() || Actor->IsA<AController>() || !Actor->GetRootComponent())
			{
				continue;
			}
			FVector Origin, Extent;
			Actor->GetActorBounds(/*bOnlyCollidingComponents*/ false, Origin, Extent);
			if (Extent.IsNearlyZero())
			{
				continue;
			}
			// Furthest point of the box toward the room side of the mirror.
			const float FrontMost = FVector::DotProduct(Origin - PlanePoint, Normal)
				+ FMath::Abs(Extent.X * Normal.X) + FMath::Abs(Extent.Y * Normal.Y) + FMath::Abs(Extent.Z * Normal.Z);
			if (FrontMost < Margin)
			{
				Capture->HiddenActors.AddUnique(Actor);
			}
		}
	}
}

const FName ALTTMirror::TextureParameterName(TEXT("MirrorTexture"));
const FName ALTTMirror::TextureRightParameterName(TEXT("MirrorTextureRight"));
const FName ALTTMirror::TintParameterName(TEXT("Tint"));

namespace
{
	/** True while a headset renders the game (two eye views). */
	bool LTTMirrorIsStereo()
	{
		return GEngine && GEngine->StereoRenderingDevice.IsValid() && GEngine->StereoRenderingDevice->IsStereoEnabled();
	}
}

ALTTMirror::ALTTMirror()
{
	PrimaryActorTick.bCanEverTick = true;
	// Capture after the camera has moved this frame, so the reflection doesn't lag a frame behind.
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GlassMat(TEXT("/Game/_CleanCutAction/Materials/Mirror/M_Mirror.M_Mirror"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FrameMat(TEXT("/Game/_CleanCutAction/Materials/Mirror/M_MirrorFrame.M_MirrorFrame"));

	Glass = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Glass"));
	Glass->SetupAttachment(Root);
	Glass->SetStaticMesh(PlaneMesh.Object);
	Glass->SetCollisionProfileName(TEXT("BlockAll"));
	Glass->CastShadow = false;

	Frame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Frame"));
	Frame->SetupAttachment(Root);
	Frame->SetStaticMesh(CubeMesh.Object);
	Frame->SetCollisionProfileName(TEXT("BlockAll"));

	Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Capture"));
	Capture->SetupAttachment(Root);
	Capture->bCaptureEveryFrame = false;
	Capture->bCaptureOnMovement = false;
	Capture->bAlwaysPersistRenderingState = true;
	// HDR scene colour: the main view's exposure and tonemapper then treat the reflection like the rest of the frame.
	Capture->CaptureSource = SCS_SceneColorHDRNoAlpha;
	Capture->ShowFlags.SetMotionBlur(false);

	CaptureRight = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("CaptureRight"));
	CaptureRight->SetupAttachment(Root);
	CaptureRight->bCaptureEveryFrame = false;
	CaptureRight->bCaptureOnMovement = false;
	CaptureRight->bAlwaysPersistRenderingState = true;
	CaptureRight->CaptureSource = SCS_SceneColorHDRNoAlpha;
	CaptureRight->ShowFlags.SetMotionBlur(false);

	MirrorMaterial = GlassMat.Object;
	FrameMaterial = FrameMat.Object;
}

void ALTTMirror::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	LayoutPieces();
}

void ALTTMirror::LayoutPieces()
{
	// Engine plane: 100x100 facing +Z. Pitch -90 turns its normal to the actor's +X; it sits just in front of the frame.
	Glass->SetRelativeLocationAndRotation(FVector(0.05f, 0.f, 0.f), FRotator(-90.f, 0.f, 0.f));
	Glass->SetRelativeScale3D(FVector(MirrorSize.Y / 100.f, MirrorSize.X / 100.f, 1.f));
	if (MirrorMaterial)
	{
		Glass->SetMaterial(0, MirrorMaterial);
	}

	// Engine cube: 100 cm, centred. Backing board behind the glass, bigger by the frame border.
	const bool bHasFrame = FrameDepth > 0.f;
	Frame->SetVisibility(bHasFrame);
	Frame->SetCollisionEnabled(bHasFrame ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	Frame->SetRelativeLocation(FVector(-FrameDepth * 0.5f, 0.f, 0.f));
	Frame->SetRelativeScale3D(FVector(FrameDepth, MirrorSize.X + 2.f * FrameWidth, MirrorSize.Y + 2.f * FrameWidth) / 100.f);
	if (FrameMaterial)
	{
		Frame->SetMaterial(0, FrameMaterial);
	}
}

void ALTTMirror::BeginPlay()
{
	Super::BeginPlay();

#if PLATFORM_ANDROID
	// Phones: the reflection is a second full scene render, so draw it smaller, without shadows, only up close.
	ResolutionScale = FMath::Min(ResolutionScale, 0.35f);
	MaxResolution = FMath::Min(MaxResolution, 512);
	bReflectShadows = false;
	UpdateDistance = UpdateDistance > 0.f ? FMath::Min(UpdateDistance, 1200.f) : 1200.f;
	MaxReflectionDistance = MaxReflectionDistance > 0.f ? FMath::Min(MaxReflectionDistance, 1500.f) : 1500.f;
#endif

	// The mirror never shows up in its own reflection (its frame sits behind the glass, right in front of the capture).
	for (USceneCaptureComponent2D* EyeCapture : { Capture.Get(), CaptureRight.Get() })
	{
		EyeCapture->HiddenActors.AddUnique(this);
		HideActorsBehindMirror(EyeCapture, this);
		EyeCapture->MaxViewDistanceOverride = MaxReflectionDistance > 0.f ? MaxReflectionDistance : -1.f;
		EyeCapture->ShowFlags.SetDynamicShadows(bReflectShadows);
		// Cuts away everything behind the mirror plane (the wall it hangs on). Needs r.AllowGlobalClipPlane; the
		// custom near plane set each capture covers the common case without it.
		EyeCapture->bEnableClipPlane = true;
	}

	if (MirrorMaterial)
	{
		GlassMID = Glass->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MirrorMaterial);
		GlassMID->SetVectorParameterValue(TintParameterName, Tint);
	}
	EnsureRenderTarget(LTTMirrorIsStereo());
}

void ALTTMirror::EnsureRenderTarget(bool bStereo)
{
	int32 Width = 16;
	int32 Height = 16;
	if (bStereo)
	{
		// Each eye's view is close to square; the capture uses the eye's own projection, so a square target maps
		// onto it exactly.
		Width = Height = FMath::Clamp(MaxResolution, 256, 1024);
	}
	else
	{
		FIntPoint ViewSize(1280, 720);
		if (GEngine && GEngine->GameViewport)
		{
			FVector2D Size;
			GEngine->GameViewport->GetViewportSize(Size);
			if (Size.X > 1.f && Size.Y > 1.f)
			{
				ViewSize = FIntPoint(FMath::RoundToInt(Size.X), FMath::RoundToInt(Size.Y));
			}
		}
		float Scale = ResolutionScale;
		const int32 LongEdge = FMath::Max(ViewSize.X, ViewSize.Y);
		if (LongEdge * Scale > MaxResolution)
		{
			Scale = float(MaxResolution) / float(LongEdge);
		}
		Width = FMath::Max(16, FMath::RoundToInt(ViewSize.X * Scale));
		Height = FMath::Max(16, FMath::RoundToInt(ViewSize.Y * Scale));
	}

	auto Prepare = [this, Width, Height](TObjectPtr<UTextureRenderTarget2D>& Target, const TCHAR* Name, USceneCaptureComponent2D* EyeCapture, FName Parameter)
	{
		if (Target && Target->SizeX == Width && Target->SizeY == Height)
		{
			return;
		}
		if (!Target)
		{
			Target = NewObject<UTextureRenderTarget2D>(this, Name);
			Target->RenderTargetFormat = RTF_RGBA16f;
			Target->ClearColor = FLinearColor::Black;
			Target->bAutoGenerateMips = false;
		}
		Target->InitAutoFormat(Width, Height);
		Target->UpdateResourceImmediate(true);
		EyeCapture->TextureTarget = Target;
		if (GlassMID)
		{
			GlassMID->SetTextureParameterValue(Parameter, Target);
		}
	};
	Prepare(RenderTarget, TEXT("MirrorRT"), Capture, TextureParameterName);
	if (bStereo)
	{
		Prepare(RenderTargetRight, TEXT("MirrorRTRight"), CaptureRight, TextureRightParameterName);
	}
}

float ALTTMirror::NearPlaneFor(const FVector& MirroredLocation, const FVector& Forward) const
{
	const FVector PlanePoint = Glass->GetComponentLocation();
	const FVector Right = GetActorRightVector() * (MirrorSize.X * 0.5f);
	const FVector UpEdge = GetActorUpVector() * (MirrorSize.Y * 0.5f);
	float MinDepth = TNumericLimits<float>::Max();
	for (const FVector& Corner : { PlanePoint + Right + UpEdge, PlanePoint + Right - UpEdge, PlanePoint - Right + UpEdge, PlanePoint - Right - UpEdge })
	{
		MinDepth = FMath::Min(MinDepth, static_cast<float>(FVector::DotProduct(Corner - MirroredLocation, Forward)));
	}
	return FMath::Max(1.f, MinDepth - 1.f);
}

void ALTTMirror::CaptureEye(USceneCaptureComponent2D* EyeCapture, int32 EyeIndex, const FVector& HeadLocation, const FRotator& HeadRotation)
{
	IStereoRendering* Stereo = GEngine->StereoRenderingDevice.Get();
	FVector EyeLocation = HeadLocation;
	FRotator EyeRotation = HeadRotation;
	const float WorldToMeters = GetWorld() && GetWorld()->GetWorldSettings() ? GetWorld()->GetWorldSettings()->WorldToMeters : 100.f;
	Stereo->CalculateStereoViewOffset(EyeIndex, EyeRotation, WorldToMeters, EyeLocation);

	const FVector PlanePoint = Glass->GetComponentLocation();
	const FVector Normal = GetActorForwardVector();
	const float EyeSide = FVector::DotProduct(EyeLocation - PlanePoint, Normal);
	if (EyeSide <= 0.f)
	{
		return;
	}
	auto Reflect = [&Normal](const FVector& V) { return V - 2.f * FVector::DotProduct(V, Normal) * Normal; };
	const FVector MirroredLocation = EyeLocation - 2.f * EyeSide * Normal;
	const FVector Forward = Reflect(EyeRotation.Vector());
	const FVector Up = Reflect(FRotationMatrix(EyeRotation).GetScaledAxis(EAxis::Z));
	EyeCapture->SetWorldLocationAndRotation(MirroredLocation, FRotationMatrix::MakeFromXZ(Forward, Up).Rotator());

	// The eye's own (off-centre) projection, so the reflection lines up with what that eye sees. The capture image
	// is mirrored left-right (the glass flips it back), so the frustum's horizontal offset flips too. The near plane
	// sits at the glass.
	FMatrix Projection = Stereo->GetStereoProjectionMatrix(EyeIndex);
	Projection.M[2][0] = -Projection.M[2][0];
	Projection.M[3][2] = NearPlaneFor(MirroredLocation, Forward);
	EyeCapture->bUseCustomProjectionMatrix = true;
	EyeCapture->CustomProjectionMatrix = Projection;
	EyeCapture->bOverride_CustomNearClippingPlane = false;
	EyeCapture->ClipPlaneBase = PlanePoint;
	EyeCapture->ClipPlaneNormal = Normal;
	EyeCapture->CaptureScene();
}

bool ALTTMirror::GetViewerCamera(FVector& OutLocation, FRotator& OutRotation, float& OutFOV) const
{
	const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC || !PC->PlayerCameraManager)
	{
		return false;
	}
	OutLocation = PC->PlayerCameraManager->GetCameraLocation();
	OutRotation = PC->PlayerCameraManager->GetCameraRotation();
	OutFOV = PC->PlayerCameraManager->GetFOVAngle();
	return true;
}

void ALTTMirror::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Skip the (expensive) capture when nobody can see the glass.
	if (!Glass->WasRecentlyRendered(0.2f))
	{
		return;
	}

	FVector CamLoc;
	FRotator CamRot;
	float FOV = 90.f;
	if (!GetViewerCamera(CamLoc, CamRot, FOV))
	{
		return;
	}

	const FVector PlanePoint = Glass->GetComponentLocation();
	const FVector Normal = GetActorForwardVector();
	const float CamSide = FVector::DotProduct(CamLoc - PlanePoint, Normal);
	if (CamSide <= 0.f || (UpdateDistance > 0.f && FVector::DistSquared(CamLoc, PlanePoint) > FMath::Square(UpdateDistance)))
	{
		return; // behind the mirror or too far away
	}

	const bool bStereo = LTTMirrorIsStereo();
	EnsureRenderTarget(bStereo);
	if (!RenderTarget)
	{
		return;
	}
	if (bStereo)
	{
		// VR: one reflection per eye (the glass material picks the right one per eye).
		CaptureEye(Capture, 0, CamLoc, CamRot);
		if (RenderTargetRight)
		{
			CaptureEye(CaptureRight, 1, CamLoc, CamRot);
		}
		return;
	}
	Capture->bUseCustomProjectionMatrix = false;

	// Reflect the camera across the mirror plane. The reflected basis is left-handed, so the camera built from it
	// sees the scene flipped left-right; the glass material flips the screen U back.
	auto Reflect = [&Normal](const FVector& V) { return V - 2.f * FVector::DotProduct(V, Normal) * Normal; };
	const FVector MirroredLoc = CamLoc - 2.f * CamSide * Normal;
	const FVector Forward = Reflect(CamRot.Vector());
	const FVector Up = Reflect(FRotationMatrix(CamRot).GetScaledAxis(EAxis::Z));
	const FRotator MirroredRot = FRotationMatrix::MakeFromXZ(Forward, Up).Rotator();

	Capture->SetWorldLocationAndRotation(MirroredLoc, MirroredRot);
	Capture->FOVAngle = FOV;

	// Near plane at the closest corner of the glass, so the wall behind the mirror isn't drawn.
	Capture->bOverride_CustomNearClippingPlane = true;
	Capture->CustomNearClippingPlane = NearPlaneFor(MirroredLoc, Forward);
	Capture->ClipPlaneBase = PlanePoint;
	Capture->ClipPlaneNormal = Normal;

	Capture->CaptureScene();
}
