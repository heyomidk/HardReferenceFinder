#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class FBlueprintEditor;
class FHardReferenceViewerSearchData;

class SHardReferenceViewerWindow : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SHardReferenceViewerWindow) {};
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<FBlueprintEditor> BlueprintGraph);

private:
	TSharedPtr<FHardReferenceViewerSearchData> SearchData;
};
