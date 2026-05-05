// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Player/Character/GsPlayer.h"

void UPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	(void)DeltaSeconds;

	if (const APawn* OwningPawn = TryGetPawnOwner())
	{
		const FVector PawnVelocity = OwningPawn->GetVelocity();
		bIsMoving = PawnVelocity.SizeSquared2D() > KINDA_SMALL_NUMBER;

		const ACharacter* OwningCharacter = Cast<ACharacter>(OwningPawn);
		const UCharacterMovementComponent* MovementComponent = OwningCharacter ? OwningCharacter->GetCharacterMovement() : nullptr;
		const AGsPlayer* PlayerCharacter = Cast<AGsPlayer>(OwningPawn);
		const bool bIsOnGround = MovementComponent && MovementComponent->IsMovingOnGround();
		const bool bIsInFallingMovementMode = MovementComponent && MovementComponent->IsFalling();
		bIsSliding = PlayerCharacter && PlayerCharacter->IsSliding();
		const bool bIsExcludedAirAction = PlayerCharacter && (PlayerCharacter->IsWallRunning() || PlayerCharacter->IsDashing());

		if (bWasOnGroundLastFrame && bIsInFallingMovementMode && PawnVelocity.Z > 0.0f && !bIsExcludedAirAction)
		{
			bIsJumpStarting = true;
		}
		else if (!bIsInFallingMovementMode || PawnVelocity.Z <= 0.0f || bIsExcludedAirAction)
		{
			bIsJumpStarting = false;
		}

		bIsFalling = bIsInFallingMovementMode && PawnVelocity.Z < 0.0f && !bIsExcludedAirAction;
		bWasOnGroundLastFrame = bIsOnGround;
		return;
	}

	bIsMoving = false;
	bIsFalling = false;
	bIsJumpStarting = false;
	bIsSliding = false;
	bWasOnGroundLastFrame = false;
}
