// ===================================================
// 文件：RealmEditorHelper.cpp
// 说明：URealmEditorHelper 的实现。仅在 WITH_EDITOR 下提供
//       实际逻辑，运行时是空壳。
// ===================================================

#include "RealmEditorHelper.h"
#include "Materials/MaterialParameterCollection.h"

void URealmEditorHelper::RebuildMPC(UMaterialParameterCollection* MPC)
{
#if WITH_EDITOR
	if (!MPC)
	{
		return;
	}

	// MPC::PostEditChangeProperty 通过比较 Pre/Post 参数数量来决定是否重建
	// UniformBufferStruct。PreEditChange 时清空数组让 Previous=0，
	// 恢复后 PostEditChange 看到 Current=N != 0，触发 CreateBufferStruct。
	TArray<FCollectionScalarParameter> SavedScalar = MPC->ScalarParameters;
	TArray<FCollectionVectorParameter> SavedVector = MPC->VectorParameters;

	MPC->ScalarParameters.Empty();
	MPC->VectorParameters.Empty();
	MPC->PreEditChange(nullptr);

	MPC->ScalarParameters = MoveTemp(SavedScalar);
	MPC->VectorParameters = MoveTemp(SavedVector);

	FPropertyChangedEvent Event(nullptr);
	MPC->PostEditChangeProperty(Event);
#endif
}
