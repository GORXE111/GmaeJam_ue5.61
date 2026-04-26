// ===================================================
// 文件：MaterialExpressionRealmRevealMask.cpp
// 说明：UMaterialExpressionRealmRevealMask 的实现。
//       Compile 函数通过 Compiler->AccessCollectionParameter
//       直接访问 MPC 参数，UE 5.6 兼容。
// ===================================================

#include "MaterialExpressionRealmRevealMask.h"
#include "Materials/MaterialParameterCollection.h"
#include "UObject/UnrealType.h"

#if WITH_EDITOR
#include "MaterialCompiler.h"
#endif

#define LOCTEXT_NAMESPACE "MaterialExpressionRealmRevealMask"

UMaterialExpressionRealmRevealMask::UMaterialExpressionRealmRevealMask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	struct FConstructorStatics
	{
		FText NAME_Realm;
		FConstructorStatics() : NAME_Realm(LOCTEXT("Realm", "Realm")) {}
	};
	static FConstructorStatics ConstructorStatics;

	MenuCategories.Add(ConstructorStatics.NAME_Realm);
#endif
}

void UMaterialExpressionRealmRevealMask::PostLoad()
{
	// 跳过父类 PostLoad（未导出），直接调祖父类 UMaterialExpression::PostLoad
	UMaterialExpression::PostLoad();
}

#if WITH_EDITOR

void UMaterialExpressionRealmRevealMask::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	// 跳过父类（未导出），直接调祖父类
	UMaterialExpression::PostEditChangeProperty(PropertyChangedEvent);
}

bool UMaterialExpressionRealmRevealMask::MatchesSearchQuery(const TCHAR* SearchQuery)
{
	if (FCString::Stristr(TEXT("Realm Reveal Mask"), SearchQuery)
		|| FCString::Stristr(TEXT("realm"), SearchQuery)
		|| FCString::Stristr(TEXT("reveal"), SearchQuery))
	{
		return true;
	}
	return UMaterialExpression::MatchesSearchQuery(SearchQuery);
}

namespace
{
	// 通过 FName 查 MPC 参数索引（UE5.6: FName -> GUID -> Index）
	bool ResolveMPCParam(UMaterialParameterCollection* MPC, FName ParamName, int32& OutParamIdx, int32& OutCompIdx)
	{
		if (!MPC)
		{
			return false;
		}
		const FGuid Id = MPC->GetParameterId(ParamName);
		if (!Id.IsValid())
		{
			return false;
		}
		MPC->GetParameterIndex(Id, OutParamIdx, OutCompIdx);
		return OutParamIdx != INDEX_NONE;
	}
}

int32 UMaterialExpressionRealmRevealMask::Compile(FMaterialCompiler* Compiler, int32 OutputIndex)
{
	if (!Collection)
	{
		return Compiler->Errorf(TEXT("RealmRevealMask: 必须指定 Collection（MPC_RealmReveal）"));
	}

	int32 CenterParamIdx,   CenterCompIdx;
	int32 RadiusParamIdx,   RadiusCompIdx;
	int32 SoftnessParamIdx, SoftnessCompIdx;
	int32 EnabledParamIdx,  EnabledCompIdx;

	if (!ResolveMPCParam(Collection, TEXT("RevealCenter"),  CenterParamIdx,   CenterCompIdx)
		|| !ResolveMPCParam(Collection, TEXT("RevealRadius"),  RadiusParamIdx,   RadiusCompIdx)
		|| !ResolveMPCParam(Collection, TEXT("EdgeSoftness"),  SoftnessParamIdx, SoftnessCompIdx)
		|| !ResolveMPCParam(Collection, TEXT("RevealEnabled"), EnabledParamIdx,  EnabledCompIdx))
	{
		return Compiler->Errorf(TEXT("RealmRevealMask: MPC 缺少 RevealCenter / RevealRadius / EdgeSoftness / RevealEnabled"));
	}

	const int32 Center   = Compiler->AccessCollectionParameter(Collection, CenterParamIdx,   CenterCompIdx);
	const int32 Radius   = Compiler->AccessCollectionParameter(Collection, RadiusParamIdx,   RadiusCompIdx);
	const int32 Softness = Compiler->AccessCollectionParameter(Collection, SoftnessParamIdx, SoftnessCompIdx);
	const int32 Enabled  = Compiler->AccessCollectionParameter(Collection, EnabledParamIdx,  EnabledCompIdx);

	// Center 是 float4，取 RGB
	const int32 CenterRGB = Compiler->ComponentMask(Center, true, true, true, false);

	// 世界坐标 (LWC，编译器内部自动处理)
	const int32 WorldPos = Compiler->WorldPosition(WPT_Default);

	// 距离 = length(WorldPos - Center)
	const int32 Diff = Compiler->Sub(WorldPos, CenterRGB);
	const int32 Dist = Compiler->Length(Diff);

	// Mask = 1 - smoothstep(R-S, R+S, Dist)
	const int32 MinV     = Compiler->Sub(Radius, Softness);
	const int32 MaxV     = Compiler->Add(Radius, Softness);
	const int32 Smooth   = Compiler->SmoothStep(MinV, MaxV, Dist);
	const int32 OneMinus = Compiler->Sub(Compiler->Constant(1.0f), Smooth);

	// 乘上 Enabled 开关
	const int32 Result = Compiler->Mul(OneMinus, Enabled);

	return Compiler->Saturate(Result);
}

void UMaterialExpressionRealmRevealMask::GetCaption(TArray<FString>& OutCaptions) const
{
	OutCaptions.Add(TEXT("Realm Reveal Mask"));
}

FString UMaterialExpressionRealmRevealMask::GetDescription() const
{
	return TEXT("里世界圆形揭示 Mask（接 Opacity Mask 使用，材质需设为 Masked）");
}

FText UMaterialExpressionRealmRevealMask::GetKeywords() const
{
	return FText::FromString(TEXT("realm reveal mask 里世界 揭示"));
}

#endif // WITH_EDITOR

#undef LOCTEXT_NAMESPACE
