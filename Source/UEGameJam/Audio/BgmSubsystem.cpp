#include "Audio/BgmSubsystem.h"

#include "Audio/AudioDataAsset.h"
#include "Components/AudioComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Settings/GsProjectResourceSettings.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

void UBgmSubsystem::Deinitialize()
{
	StopBGM();
	Super::Deinitialize();
}

UBgmSubsystem* UBgmSubsystem::Get(const UObject* WorldContext)
{
	if (!WorldContext)
	{
		return nullptr;
	}

	UWorld* World = WorldContext->GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UBgmSubsystem>() : nullptr;
}

void UBgmSubsystem::PlayDefaultBGM()
{
	ClearIntroTimer();
	bCombatLoopStarted = false;
	StopComponent(IntroBGMComponent, 0.f);
	StopComponent(SurfaceBGMComponent, 0.f);
	StopComponent(RealmBGMComponent, 0.f);

	UAudioDataAsset* AudioData = GetAudioData();
	if (!AudioData || !AudioData->DefaultBGM)
	{
		StopComponent(DefaultBGMComponent, StopFadeOutTime);
		return;
	}

	StopComponent(DefaultBGMComponent, 0.f);
	DefaultBGMComponent = CreateBGMComponent(AudioData->DefaultBGM, 0.f);
	if (DefaultBGMComponent)
	{
		DefaultBGMComponent->FadeIn(DefaultFadeInTime, 1.f);
	}
}

void UBgmSubsystem::PlayCombatBGM(ERealmType InitialRealm)
{
	ClearIntroTimer();
	PendingCombatRealm = InitialRealm;
	CurrentCombatRealm = InitialRealm;
	bCombatLoopStarted = false;

	StopComponent(DefaultBGMComponent, StopFadeOutTime);
	StopComponent(IntroBGMComponent, 0.f);
	StopComponent(SurfaceBGMComponent, 0.f);
	StopComponent(RealmBGMComponent, 0.f);

	UAudioDataAsset* AudioData = GetAudioData();
	if (!AudioData)
	{
		return;
	}

	if (!AudioData->IntroBGM)
	{
		StartCombatLoop();
		return;
	}

	IntroBGMComponent = CreateBGMComponent(AudioData->IntroBGM, 1.f);
	if (!IntroBGMComponent)
	{
		StartCombatLoop();
		return;
	}

	IntroBGMComponent->Play();

	const float IntroDuration = AudioData->IntroBGM->GetDuration();
	if (IntroDuration > 0.f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(IntroTimerHandle, this, &UBgmSubsystem::StartCombatLoop, IntroDuration, false);
		}
		else
		{
			StartCombatLoop();
		}
	}
	else
	{
		StartCombatLoop();
	}
}

void UBgmSubsystem::SwitchCombatBGM(ERealmType NewRealm)
{
	PendingCombatRealm = NewRealm;

	if (!bCombatLoopStarted)
	{
		return;
	}

	CurrentCombatRealm = NewRealm;
	ApplyCombatMix(CombatCrossFadeTime);
}

void UBgmSubsystem::StopBGM()
{
	ClearIntroTimer();
	bCombatLoopStarted = false;

	StopComponent(DefaultBGMComponent, StopFadeOutTime);
	StopComponent(IntroBGMComponent, StopFadeOutTime);
	StopComponent(SurfaceBGMComponent, StopFadeOutTime);
	StopComponent(RealmBGMComponent, StopFadeOutTime);
}

UAudioDataAsset* UBgmSubsystem::GetAudioData() const
{
	const UGsProjectResourceSettings* ResourceSettings = GetDefault<UGsProjectResourceSettings>();
	return ResourceSettings ? ResourceSettings->AudioDataAsset.LoadSynchronous() : nullptr;
}

UAudioComponent* UBgmSubsystem::CreateBGMComponent(USoundBase* Sound, float Volume)
{
	if (!Sound)
	{
		return nullptr;
	}

	UAudioComponent* AudioComponent = UGameplayStatics::CreateSound2D(this, Sound, Volume, 1.f, 0.f, nullptr, false, true);
	if (AudioComponent)
	{
		AudioComponent->bIsUISound = true;
		AudioComponent->SetVolumeMultiplier(Volume);
	}
	return AudioComponent;
}

void UBgmSubsystem::StartCombatLoop()
{
	ClearIntroTimer();
	StopComponent(IntroBGMComponent, 0.f);

	UAudioDataAsset* AudioData = GetAudioData();
	if (!AudioData)
	{
		return;
	}

	CurrentCombatRealm = PendingCombatRealm;
	bCombatLoopStarted = true;

	SurfaceBGMComponent = CreateBGMComponent(AudioData->SurfaceBGM, CurrentCombatRealm == ERealmType::Surface ? 1.f : 0.f);
	RealmBGMComponent = CreateBGMComponent(AudioData->RealmBGM, CurrentCombatRealm == ERealmType::Realm ? 1.f : 0.f);

	if (SurfaceBGMComponent)
	{
		SurfaceBGMComponent->Play();
	}
	if (RealmBGMComponent)
	{
		RealmBGMComponent->Play();
	}
}

void UBgmSubsystem::ApplyCombatMix(float FadeTime)
{
	const bool bPlaySurface = CurrentCombatRealm == ERealmType::Surface;

	if (SurfaceBGMComponent)
	{
		SurfaceBGMComponent->AdjustVolume(FadeTime, bPlaySurface ? 1.f : 0.f);
	}
	if (RealmBGMComponent)
	{
		RealmBGMComponent->AdjustVolume(FadeTime, bPlaySurface ? 0.f : 1.f);
	}
}

void UBgmSubsystem::StopComponent(TObjectPtr<UAudioComponent>& Component, float FadeTime)
{
	if (!IsValid(Component))
	{
		Component = nullptr;
		return;
	}

	if (FadeTime > 0.f)
	{
		Component->FadeOut(FadeTime, 0.f);
	}
	else
	{
		Component->Stop();
	}

	Component = nullptr;
}

void UBgmSubsystem::ClearIntroTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(IntroTimerHandle);
	}
}
