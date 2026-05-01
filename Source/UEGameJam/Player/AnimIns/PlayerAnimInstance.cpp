// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Player/Character/GsPlayer.h"

void UPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (const APawn* OwningPawn = TryGetPawnOwner())
	{
		const FVector PawnVelocity = OwningPawn->GetVelocity();
		bIsMoving = PawnVelocity.SizeSquared2D() > KINDA_SMALL_NUMBER;

		const ACharacter* OwningCharacter = Cast<ACharacter>(OwningPawn);
		const UCharacterMovementComponent* MovementComponent = OwningCharacter ? OwningCharacter->GetCharacterMovement() : nullptr;
		const AGsPlayer* PlayerCharacter = Cast<AGsPlayer>(OwningPawn);
		const bool bIsInFallingMovementMode = MovementComponent && MovementComponent->IsFalling();
		const bool bIsExcludedAirAction = PlayerCharacter && (PlayerCharacter->IsWallRunning() || PlayerCharacter->IsDashing());

		bIsFalling = bIsInFallingMovementMode && PawnVelocity.Z < 0.0f && !bIsExcludedAirAction;
		return;
	}

	bIsMoving = false;
	bIsFalling = false;
}
