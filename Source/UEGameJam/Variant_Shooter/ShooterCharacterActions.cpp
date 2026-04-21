// Copyright Epic Games, Inc. All Rights Reserved.


#include "ShooterCharacter.h"
#include "AI/ShooterNPC.h"
#include "Animation/AnimInstance.h"
#include "Components/SphereComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

bool AShooterCharacter::IsCharacterActionActive() const
{
	return CurrentAction != EShooterCharacterAction::None;
}

bool AShooterCharacter::TryStartCharacterAction(EShooterCharacterAction Action, float Duration)
{
	if (IsCharacterActionActive())
	{
		return false;
	}

	CurrentAction = Action;

	if (Duration > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(ActionTimer, this, &AShooterCharacter::FinishCharacterAction, Duration, false);
	}
	else
	{
		FinishCharacterAction();
	}

	return true;
}

void AShooterCharacter::FinishCharacterAction()
{
	GetWorld()->GetTimerManager().ClearTimer(ActionTimer);
	CurrentAction = EShooterCharacterAction::None;
}

void AShooterCharacter::DoKick()
{
	if (IsCharacterActionActive())
	{
		return;
	}

	float ActionDuration = KickFallbackDuration;

	if (KickMontage)
	{
		if (UAnimInstance* AnimInstance = GetFirstPersonMesh()->GetAnimInstance())
		{
			const float MontageDuration = AnimInstance->Montage_Play(KickMontage);
			if (MontageDuration > 0.0f)
			{
				ActionDuration = MontageDuration;
			}
		}
	}

	if (!TryStartCharacterAction(EShooterCharacterAction::Kick, ActionDuration))
	{
		return;
	}

	if (!KickDamageCollision || KickDamage <= 0.0f)
	{
		return;
	}

	KickDamageCollision->UpdateOverlaps();

	TArray<AActor*> OverlappingActors;
	KickDamageCollision->GetOverlappingActors(OverlappingActors);

	const TSubclassOf<UDamageType> DamageTypeClass = KickDamageType;

	for (AActor* OverlappingActor : OverlappingActors)
	{
		if (!IsValid(OverlappingActor) || OverlappingActor == this)
		{
			continue;
		}

		UGameplayStatics::ApplyDamage(OverlappingActor, KickDamage, GetController(), this, DamageTypeClass);

		AShooterNPC* HitNPC = Cast<AShooterNPC>(OverlappingActor);
		if (HitNPC && KickPushStrength > 0.0f)
		{
			FVector PushDirection = HitNPC->GetActorLocation() - GetActorLocation();
			PushDirection.Z = 0.0f;

			if (!PushDirection.Normalize())
			{
				PushDirection = GetActorForwardVector();
				PushDirection.Z = 0.0f;
				PushDirection.Normalize();
			}

			HitNPC->LaunchCharacter(PushDirection * KickPushStrength, true, false);
		}
	}
}
