// Clean! Cut! Action!

#include "Interaction/LTTPropSoundSet.h"
#include "Sound/SoundBase.h"

USoundBase* ULTTPropSoundSet::GetSound(ELTTPropMaterial Material, ELTTPropSoundEvent Event) const
{
	const FLTTPropSounds* Set = Sounds.Find(Material);
	if (!Set)
	{
		return nullptr;
	}
	switch (Event)
	{
	case ELTTPropSoundEvent::Pickup: return Set->Pickup;
	case ELTTPropSoundEvent::Place: return Set->Place;
	case ELTTPropSoundEvent::Break: return Set->Break;
	default: return nullptr;
	}
}
