#pragma once

#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"

class FBlueprintEditor;

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

class FHardReferenceViewerSearchData : public TSharedFromThis<FHardReferenceViewerSearchData>
{
public:
	void Generate(TSharedPtr<FBlueprintEditor> BlueprintEditor);

	int GetSizeOnDisk() const { return SizeOnDisk; }
	const TMap<FName, FHRVPackageData>& GetPackageMap() const { return PackageMap; }
	
private:
	void Reset();

	int SizeOnDisk = 0;
	TMap<FName, FHRVPackageData> PackageMap;
};
