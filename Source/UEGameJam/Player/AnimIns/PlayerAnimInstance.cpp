// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerAnimInstance.h"
#include "GameFramework/Pawn.h"

void UPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (const APawn* OwningPawn = TryGetPawnOwner())
	{
		bIsMoving = OwningPawn->GetVelocity().SizeSquared2D() > KINDA_SMALL_NUMBER;
		return;
	}

	bIsMoving = false;
}
