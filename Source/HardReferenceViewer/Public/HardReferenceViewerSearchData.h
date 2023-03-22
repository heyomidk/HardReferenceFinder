#pragma once

#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"

class FBlueprintEditor;

struct FHRVDisplayData
{
	FText Name;
	FText Tooltip;
	FGuid NodeGuid;
	FSlateIcon SlateIcon;
	FLinearColor IconColor = FLinearColor::White;
};


class FHRVTreeViewItem : public TSharedFromThis<FHRVTreeViewItem>
{
public:

	bool bIsCategoryHeader = false;
	int CategorySizeOnDisk = 0;
	FHRVDisplayData DisplayData;
	TArray<TSharedPtr<FHRVTreeViewItem>> Children;
};

class FHardReferenceViewerSearchData
{
public:
	void GatherSearchData(TWeakPtr<FBlueprintEditor> BlueprintEditor);

	int GetSizeOnDisk() const { return SizeOnDisk; }
	int GetNumPackagesReferenced() const { return PackageMap.Num(); }
	TArray<TSharedPtr<FHRVTreeViewItem>> GetAsTreeViewResults();

private:
	struct FHRVNodeData
	{
		FHRVDisplayData DisplayData;
	};

	struct FHRVPackageData
	{
		FHRVDisplayData DisplayData;
		int64 SizeOnDisk = 0;
		TArray<FHRVNodeData> ReferencingNodes;
	};
	
	void Reset();

	int SizeOnDisk = 0;
	TMap<FName, FHRVPackageData> PackageMap;
};
