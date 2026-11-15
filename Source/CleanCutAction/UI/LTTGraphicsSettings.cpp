// Clean! Cut! Action!

#include "UI/LTTGraphicsSettings.h"
#include "HAL/IConsoleManager.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"

#if WITH_LTT_DLSS
#include "DLSSLibrary.h"
#include "StreamlineLibraryDLSSG.h"
#include "StreamlineLibraryReflex.h"
#endif

#define LOCTEXT_NAMESPACE "LTTGraphics"

namespace LTTGraphics
{
	namespace
	{
		EUpscaler CurrentUpscaler = EUpscaler::Off;
		bool bDefaultsApplied = false;

		void SetScreenPercentage(float Percent)
		{
			// 0 = back to r.ScreenPercentage.Default.Desktop.Mode (TSR's own resolution choice).
			if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage")))
			{
				CVar->Set(Percent, ECVF_SetByCode);
			}
		}

#if WITH_LTT_DLSS
		UDLSSMode ToDLSSMode(EUpscaler Mode)
		{
			switch (Mode)
			{
			case EUpscaler::DLAA:        return UDLSSMode::DLAA;
			case EUpscaler::Quality:     return UDLSSMode::Quality;
			case EUpscaler::Balanced:    return UDLSSMode::Balanced;
			case EUpscaler::Performance: return UDLSSMode::Performance;
			default:                     return UDLSSMode::Off;
			}
		}

		FVector2D ViewportSize()
		{
			FVector2D Size(1920.f, 1080.f);
			if (GEngine && GEngine->GameViewport)
			{
				GEngine->GameViewport->GetViewportSize(Size);
			}
			return Size;
		}
#endif

		bool IsModeSupported(EUpscaler Mode)
		{
			if (Mode == EUpscaler::Off)
			{
				return true;
			}
#if WITH_LTT_DLSS
			return IsDLSSAvailable() && UDLSSLibrary::IsDLSSModeSupported(ToDLSSMode(Mode));
#else
			return false;
#endif
		}
	}

	bool IsDLSSAvailable()
	{
#if WITH_LTT_DLSS
		return UDLSSLibrary::IsDLSSSupported();
#else
		return false;
#endif
	}

	bool IsFrameGenAvailable()
	{
#if WITH_LTT_DLSS
		return UStreamlineLibraryDLSSG::IsDLSSGSupported();
#else
		return false;
#endif
	}

	bool IsRayReconstructionAvailable()
	{
#if WITH_LTT_DLSS
		return UDLSSLibrary::IsDLSSRRSupported() && UDLSSLibrary::IsRayTracingAvailable();
#else
		return false;
#endif
	}

	void ApplyDefaultsOnce()
	{
		if (bDefaultsApplied)
		{
			return;
		}
		bDefaultsApplied = true;
		if (!IsDLSSAvailable())
		{
			return; // non-RTX GPU or non-Windows build: TSR stays on
		}
		SetUpscaler(IsModeSupported(EUpscaler::Quality) ? EUpscaler::Quality : EUpscaler::DLAA);
		SetFrameGen(IsFrameGenAvailable());
		SetRayReconstruction(IsRayReconstructionAvailable());
	}

	EUpscaler GetUpscaler()
	{
		return CurrentUpscaler;
	}

	void SetUpscaler(EUpscaler Mode)
	{
		if (!IsModeSupported(Mode))
		{
			Mode = EUpscaler::Off;
		}
#if WITH_LTT_DLSS
		if (Mode == EUpscaler::Off)
		{
			UDLSSLibrary::EnableDLSS(false);
			SetScreenPercentage(0.f);
		}
		else
		{
			bool bSupported = false, bFixed = false;
			float Optimal = 100.f, MinPct = 100.f, MaxPct = 100.f, Sharpness = 0.f;
			UDLSSLibrary::GetDLSSModeInformation(ToDLSSMode(Mode), ViewportSize(), bSupported, Optimal, bFixed, MinPct, MaxPct, Sharpness);
			UDLSSLibrary::EnableDLSS(true);
			SetScreenPercentage(Mode == EUpscaler::DLAA ? 100.f : Optimal);
		}
#endif
		CurrentUpscaler = Mode;
	}

	void CycleUpscaler()
	{
		static const EUpscaler Order[] = { EUpscaler::Off, EUpscaler::DLAA, EUpscaler::Quality, EUpscaler::Balanced, EUpscaler::Performance };
		int32 Index = 0;
		for (int32 i = 0; i < UE_ARRAY_COUNT(Order); ++i)
		{
			if (Order[i] == CurrentUpscaler)
			{
				Index = i;
			}
		}
		for (int32 Step = 1; Step <= UE_ARRAY_COUNT(Order); ++Step)
		{
			const EUpscaler Next = Order[(Index + Step) % UE_ARRAY_COUNT(Order)];
			if (IsModeSupported(Next))
			{
				SetUpscaler(Next);
				return;
			}
		}
	}

	bool IsFrameGenOn()
	{
#if WITH_LTT_DLSS
		return IsFrameGenAvailable() && UStreamlineLibraryDLSSG::GetDLSSGMode() != EStreamlineDLSSGMode::Off;
#else
		return false;
#endif
	}

	void SetFrameGen(bool bOn)
	{
#if WITH_LTT_DLSS
		if (!IsFrameGenAvailable())
		{
			return;
		}
		// Frame Generation needs Reflex for latency; NVIDIA notes VSync can misbehave with it on.
		if (bOn && UStreamlineLibraryReflex::IsReflexSupported())
		{
			UStreamlineLibraryReflex::SetReflexMode(EStreamlineReflexMode::Enabled);
		}
		if (bOn)
		{
			if (IConsoleVariable* VSync = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VSync")))
			{
				VSync->Set(0, ECVF_SetByCode);
			}
		}
		// Auto: 2x on RTX 40, multi-frame on RTX 50.
		const EStreamlineDLSSGMode Mode = !bOn ? EStreamlineDLSSGMode::Off
			: UStreamlineLibraryDLSSG::IsDLSSGModeSupported(EStreamlineDLSSGMode::Auto) ? EStreamlineDLSSGMode::Auto
			: EStreamlineDLSSGMode::On2X;
		UStreamlineLibraryDLSSG::SetDLSSGMode(Mode);
#endif
	}

	bool IsRayReconstructionOn()
	{
#if WITH_LTT_DLSS
		return IsRayReconstructionAvailable() && UDLSSLibrary::IsDLSSRREnabled();
#else
		return false;
#endif
	}

	void SetRayReconstruction(bool bOn)
	{
#if WITH_LTT_DLSS
		if (IsRayReconstructionAvailable())
		{
			UDLSSLibrary::EnableDLSSRR(bOn);
		}
#endif
	}

	FText GetUpscalerLabel()
	{
		switch (CurrentUpscaler)
		{
		case EUpscaler::DLAA:        return LOCTEXT("DLAA", "NVIDIA DLSS: DLAA");
		case EUpscaler::Quality:     return LOCTEXT("Quality", "NVIDIA DLSS: Quality");
		case EUpscaler::Balanced:    return LOCTEXT("Balanced", "NVIDIA DLSS: Balanced");
		case EUpscaler::Performance: return LOCTEXT("Performance", "NVIDIA DLSS: Performance");
		default:                     return LOCTEXT("Off", "NVIDIA DLSS: Off (TSR)");
		}
	}

	FText GetFrameGenLabel()
	{
		return IsFrameGenOn() ? LOCTEXT("FGOn", "Frame Generation: On") : LOCTEXT("FGOff", "Frame Generation: Off");
	}

	FText GetRayReconstructionLabel()
	{
		return IsRayReconstructionOn() ? LOCTEXT("RROn", "Ray Reconstruction: On") : LOCTEXT("RROff", "Ray Reconstruction: Off");
	}
}

#undef LOCTEXT_NAMESPACE
