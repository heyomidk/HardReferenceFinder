#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class FBlueprintEditor;

class SHardReferenceViewer : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SHardReferenceViewer) {};
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TWeakPtr<FBlueprintEditor> BlueprintGraph);
};
