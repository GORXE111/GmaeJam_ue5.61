// ===================================================
// 文件：RealmRevealerComponent.cpp
// 说明：URealmRevealerComponent 的实现。维护全局活动列表，每帧
//       由首个 tick 的组件统一选出胜者：先比 Priority，再比离主
//       摄像机的距离。胜者把 RevealCenter/Radius/Softness/Enabled
//       写入 MPC_RealmReveal；其他组件本帧跳过。
// ===================================================

#include "RealmRevealerComponent.h"
#include "Materials/MaterialParameterCollection.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

TArray<TWeakObjectPtr<URealmRevealerComponent>> URealmRevealerComponent::ActiveRevealers;
uint64 URealmRevealerComponent::LastResolvedFrame = 0;
FVector URealmRevealerComponent::CachedActiveCenter = FVector::ZeroVector;
float URealmRevealerComponent::CachedActiveRadius = 0.0f;
bool URealmRevealerComponent::bCachedAnyActive = false;

URealmRevealerComponent::URealmRevealerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void URealmRevealerComponent::BeginPlay()
{
	Super::BeginPlay();
	ActiveRevealers.AddUnique(this);
}

void URealmRevealerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ActiveRevealers.RemoveAllSwap([this](const TWeakObjectPtr<URealmRevealerComponent>& W)
	{
		return !W.IsValid() || W.Get() == this;
	});
	Super::EndPlay(EndPlayReason);
}

void URealmRevealerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	ResolveAndApply();
}

void URealmRevealerComponent::ResolveAndApply()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const uint64 Frame = GFrameCounter;
	if (Frame == LastResolvedFrame)
	{
		return;
	}
	LastResolvedFrame = Frame;

	// 主摄像机位置（用于同优先级时的距离比较）
	FVector CameraLoc = FVector::ZeroVector;
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
	{
		FRotator Unused;
		PC->GetPlayerViewPoint(CameraLoc, Unused);
	}

	URealmRevealerComponent* Winner = nullptr;
	int32 BestPriority = TNumericLimits<int32>::Lowest();
	float BestDistSq = TNumericLimits<float>::Max();

	for (const TWeakObjectPtr<URealmRevealerComponent>& WP : ActiveRevealers)
	{
		URealmRevealerComponent* C = WP.Get();
		if (!C || !C->bEnabled || !C->GetOwner())
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(C->GetOwner()->GetActorLocation(), CameraLoc);
		if (C->Priority > BestPriority || (C->Priority == BestPriority && DistSq < BestDistSq))
		{
			Winner = C;
			BestPriority = C->Priority;
			BestDistSq = DistSq;
		}
	}

	if (!Winner || !Winner->RealmMPC)
	{
		// 没胜者：把 MPC 关掉
		CachedActiveCenter = FVector::ZeroVector;
		CachedActiveRadius = 0.0f;
		bCachedAnyActive = false;
		if (Winner && Winner->RealmMPC)
		{
			UKismetMaterialLibrary::SetScalarParameterValue(this, Winner->RealmMPC, FName("RevealEnabled"), 0.0f);
		}
		return;
	}

	const FVector WinnerLoc = Winner->GetOwner()->GetActorLocation();
	CachedActiveCenter = WinnerLoc;
	CachedActiveRadius = Winner->RevealRadius;
	bCachedAnyActive = true;

	UKismetMaterialLibrary::SetVectorParameterValue(this, Winner->RealmMPC,
		FName("RevealCenter"), FLinearColor(WinnerLoc));
	UKismetMaterialLibrary::SetScalarParameterValue(this, Winner->RealmMPC,
		FName("RevealRadius"), Winner->RevealRadius);
	UKismetMaterialLibrary::SetScalarParameterValue(this, Winner->RealmMPC,
		FName("EdgeSoftness"), Winner->EdgeSoftness);
	UKismetMaterialLibrary::SetScalarParameterValue(this, Winner->RealmMPC,
		FName("RevealEnabled"), 1.0f);
}

FVector URealmRevealerComponent::GetActiveCenter()  { return CachedActiveCenter; }
float   URealmRevealerComponent::GetActiveRadius()  { return CachedActiveRadius; }
bool    URealmRevealerComponent::IsAnyActive()      { return bCachedAnyActive; }
