// Clean! Cut! Action!

#pragma once

#include "CoreMinimal.h"

/**
 * NVIDIA DLSS settings for the Windows build (DLSS Super Resolution / DLAA, Frame Generation, Ray Reconstruction).
 * Compiled in only when WITH_LTT_DLSS=1 (Win64, see CleanCutAction.Build.cs). On other platforms and on GPUs without
 * DLSS every call is a no-op and IsDLSSAvailable() returns false, so the pause menu hides the toggles and TSR stays on.
 */
namespace LTTGraphics
{
	enum class EUpscaler : uint8
	{
		Off,        // TSR (the project default)
		DLAA,       // DLSS at native resolution
		Quality,
		Balanced,
		Performance,
	};

	/** Turns on the defaults once per run: DLSS Quality, Frame Generation and Ray Reconstruction where supported. */
	CLEANCUTACTION_API void ApplyDefaultsOnce();

	CLEANCUTACTION_API bool IsDLSSAvailable();
	CLEANCUTACTION_API bool IsFrameGenAvailable();
	CLEANCUTACTION_API bool IsRayReconstructionAvailable();

	CLEANCUTACTION_API EUpscaler GetUpscaler();
	CLEANCUTACTION_API void SetUpscaler(EUpscaler Mode);
	/** Next supported mode: Off > DLAA > Quality > Balanced > Performance > Off. */
	CLEANCUTACTION_API void CycleUpscaler();

	CLEANCUTACTION_API bool IsFrameGenOn();
	CLEANCUTACTION_API void SetFrameGen(bool bOn);

	CLEANCUTACTION_API bool IsRayReconstructionOn();
	CLEANCUTACTION_API void SetRayReconstruction(bool bOn);

	CLEANCUTACTION_API FText GetUpscalerLabel();
	CLEANCUTACTION_API FText GetFrameGenLabel();
	CLEANCUTACTION_API FText GetRayReconstructionLabel();
}
