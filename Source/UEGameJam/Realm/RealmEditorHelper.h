// ===================================================
// 文件：RealmEditorHelper.h
// 说明：编辑器辅助函数。Python 创建 MPC 后无法触发
//       PostEditChangeProperty，导致 UniformBufferStruct 不构建，
//       引用此 MPC 的材质 shader 报 "MaterialCollection0 undeclared"。
//       本文件提供 C++ 函数让 Python 调用，强制走完整流程。
// ===================================================

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RealmEditorHelper.generated.h"

class UMaterialParameterCollection;

UCLASS()
class UEGAMEJAM_API URealmEditorHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 强制 MPC 重建 UniformBufferStruct（Python 创建 MPC 后必须调用一次） */
	UFUNCTION(BlueprintCallable, Category="Realm|Editor")
	static void RebuildMPC(UMaterialParameterCollection* MPC);
};
