#pragma once

#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"

class FBlueprintEditor;

class FHRVTreeViewItem : public TSharedFromThis<FHRVTreeViewItem>
{
public:
	bool bIsCategoryHeader = false;
	int CategorySizeOnDisk = 0;
	FText DisplayText;
	FGuid NodeGuid;
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
		FText DisplayText;
		FGuid NodeGuid;
	};

	struct FHRVPackageData
	{
		FText DisplayText;
		int64 SizeOnDisk = 0;
		TArray<FHRVNodeData> ReferencingNodes;
	};
	
	void Reset();

	int SizeOnDisk = 0;
	TMap<FName, FHRVPackageData> PackageMap;
};
