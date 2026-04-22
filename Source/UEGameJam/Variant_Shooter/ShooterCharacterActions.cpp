// Copyright Epic Games, Inc. All Rights Reserved.


#include "ShooterCharacter.h"
#include "AI/ShooterNPC.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

bool AShooterCharacter::IsCharacterActionActive() const
{
	return CurrentAction != EShooterCharacterAction::None;
}

bool AShooterCharacter::IsSliding() const
{
	return CurrentAction == EShooterCharacterAction::Slide;
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
	else if (FMath::IsNearlyZero(Duration))
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

void AShooterCharacter::DoMove(float Right, float Forward)
{
	if (IsSliding())
	{
		return;
	}

	Super::DoMove(Right, Forward);
}

void AShooterCharacter::DoJumpStart()
{
	if (IsSliding())
	{
		if (!StopSlide(false))
		{
			return;
		}
	}

	Super::DoJumpStart();
}

void AShooterCharacter::DoSlide()
{
	if (IsSliding())
	{
		StopSlide(false);
		return;
	}

	StartSlide();
}

bool AShooterCharacter::StartSlide()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	UCapsuleComponent* SelfCapsule = GetCapsuleComponent();

	if (!MovementComponent || !SelfCapsule || !MovementComponent->IsMovingOnGround())
	{
		return false;
	}

	SlideDirection = GetVelocity();
	SlideDirection.Z = 0.0f;
	if (!SlideDirection.Normalize())
	{
		return false;
	}

	FVector ActorForward = GetActorForwardVector();
	ActorForward.Z = 0.0f;
	if (!ActorForward.Normalize() || FVector::DotProduct(SlideDirection, ActorForward) < 0.0f)
	{
		return false;
	}

	if (!TryStartCharacterAction(EShooterCharacterAction::Slide, -1.0f))
	{
		return false;
	}

	OriginalSlideCapsuleHalfHeight = SelfCapsule->GetUnscaledCapsuleHalfHeight();
	OriginalSlideMaxWalkSpeed = MovementComponent->MaxWalkSpeed;

	const float TargetCapsuleHalfHeight = FMath::Clamp(SlideCapsuleHalfHeight, SelfCapsule->GetUnscaledCapsuleRadius(), OriginalSlideCapsuleHalfHeight);
	const float HalfHeightDelta = OriginalSlideCapsuleHalfHeight - TargetCapsuleHalfHeight;
	const float WorldHalfHeightDelta = HalfHeightDelta * SelfCapsule->GetShapeScale();

	SelfCapsule->SetCapsuleHalfHeight(TargetCapsuleHalfHeight, true);
	AddActorWorldOffset(FVector(0.0f, 0.0f, -WorldHalfHeightDelta), false);

	MovementComponent->MaxWalkSpeed = SlideSpeed;
	CurrentSlideSpeed = SlideSpeed;
	FVector NewVelocity = SlideDirection * CurrentSlideSpeed;
	NewVelocity.Z = MovementComponent->Velocity.Z;
	MovementComponent->Velocity = NewVelocity;

	return true;
}

bool AShooterCharacter::StopSlide(bool bForceRestore)
{
	if (!IsSliding())
	{
		return true;
	}

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	UCapsuleComponent* SelfCapsule = GetCapsuleComponent();

	if (!MovementComponent || !SelfCapsule)
	{
		CurrentSlideSpeed = 0.0f;
		FinishCharacterAction();
		return true;
	}

	if (!bForceRestore && !CanRestoreSlideCapsule())
	{
		return false;
	}

	const float CurrentCapsuleHalfHeight = SelfCapsule->GetUnscaledCapsuleHalfHeight();
	const float HalfHeightDelta = OriginalSlideCapsuleHalfHeight - CurrentCapsuleHalfHeight;
	const float WorldHalfHeightDelta = HalfHeightDelta * SelfCapsule->GetShapeScale();

	if (HalfHeightDelta > 0.0f)
	{
		AddActorWorldOffset(FVector(0.0f, 0.0f, WorldHalfHeightDelta), false);
		SelfCapsule->SetCapsuleHalfHeight(OriginalSlideCapsuleHalfHeight, true);
	}

	MovementComponent->MaxWalkSpeed = OriginalSlideMaxWalkSpeed;
	CurrentSlideSpeed = 0.0f;
	FinishCharacterAction();

	return true;
}

bool AShooterCharacter::CanRestoreSlideCapsule() const
{
	UCapsuleComponent* SelfCapsule = GetCapsuleComponent();
	UWorld* World = GetWorld();

	if (!SelfCapsule || !World)
	{
		return true;
	}

	const float CurrentCapsuleHalfHeight = SelfCapsule->GetUnscaledCapsuleHalfHeight();
	const float HalfHeightDelta = OriginalSlideCapsuleHalfHeight - CurrentCapsuleHalfHeight;
	if (HalfHeightDelta <= 0.0f)
	{
		return true;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SlideCapsuleRestore), false, this);
	FCollisionResponseParams ResponseParams;
	SelfCapsule->InitSweepCollisionParams(QueryParams, ResponseParams);

	const float ShapeScale = SelfCapsule->GetShapeScale();
	const float RestoredCapsuleRadius = SelfCapsule->GetUnscaledCapsuleRadius() * ShapeScale;
	const float RestoredCapsuleHalfHeight = OriginalSlideCapsuleHalfHeight * ShapeScale;
	const FVector RestoreLocation = GetActorLocation() + FVector(0.0f, 0.0f, HalfHeightDelta * ShapeScale);
	const FCollisionShape RestoredCapsuleShape = FCollisionShape::MakeCapsule(RestoredCapsuleRadius, RestoredCapsuleHalfHeight);

	return !World->OverlapBlockingTestByChannel(
		RestoreLocation,
		GetActorQuat(),
		SelfCapsule->GetCollisionObjectType(),
		RestoredCapsuleShape,
		QueryParams,
		ResponseParams);
}

void AShooterCharacter::UpdateSlide(float DeltaSeconds)
{
	if (!IsSliding())
	{
		return;
	}

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		StopSlide(true);
		return;
	}

	if (!MovementComponent->IsMovingOnGround())
	{
		StopSlide(true);
		return;
	}

	CurrentSlideSpeed = FMath::Max(0.0f, CurrentSlideSpeed - (SlideDeceleration * DeltaSeconds));

	if (CurrentSlideSpeed < SlideStopSpeed)
	{
		if (StopSlide(false))
		{
			return;
		}

		CurrentSlideSpeed = SlideStopSpeed;
	}

	FVector NewVelocity = SlideDirection * CurrentSlideSpeed;
	NewVelocity.Z = MovementComponent->Velocity.Z;
	MovementComponent->Velocity = NewVelocity;
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
