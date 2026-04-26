// ===================================================
// 文件：MaterialExpressionRealmRevealMask.h
// 说明：自定义材质节点，里世界圆形揭示 Mask。
//       继承自 UMaterialExpressionCollectionParameter 是为了让
//       引擎自动把 Collection 注册到 Material 的
//       ReferencedParameterCollections，否则 MPC uniform 不绑定。
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "Materials/MaterialExpressionCollectionParameter.h"
#include "MaterialExpressionRealmRevealMask.generated.h"

/**
 * 里世界揭示 Mask 节点
 * 输出：float Mask（0=裁剪 / 1=显示，边缘平滑）
 * 用法：在材质里右键搜 "Realm Reveal Mask"，指定 Collection = MPC_RealmReveal
 *      把输出连到 Opacity Mask（材质需设为 Masked）
 *
 * 继承的字段：
 *   Collection   - 指向 MPC_RealmReveal（必填）
 *   ParameterName / ParameterId - 父类字段，本节点不使用
 */
UCLASS(MinimalAPI, CollapseCategories, HideCategories=(Object))
class UMaterialExpressionRealmRevealMask : public UMaterialExpressionCollectionParameter
{
	GENERATED_BODY()

public:
	UMaterialExpressionRealmRevealMask(const FObjectInitializer& ObjectInitializer);

	//~ UObject —— 父类是 MinimalAPI，这些虚函数没导出，必须在子类全部 override
	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual bool MatchesSearchQuery(const TCHAR* SearchQuery) override;

	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	virtual FString GetDescription() const override;
	virtual FText GetKeywords() const override;
#endif
};
