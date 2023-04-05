#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Templates/SharedPointer.h"

class FBlueprintEditor;

class FHRFTreeViewItem : public TSharedFromThis<FHRFTreeViewItem>
{
public:

	bool bIsHeader = false;
	int SizeOnDisk = 0;
	FName PackageId = NAME_None;
	FText Name;
	FText Tooltip;
	FGuid NodeGuid;
	FSlateIcon SlateIcon;
	FLinearColor IconColor = FLinearColor::White;
	TArray<TSharedPtr<FHRFTreeViewItem>> Children;
};
typedef TSharedPtr<FHRFTreeViewItem> FHRFTreeViewItemPtr;

#if ENGINE_MAJOR_VERSION < 5
typedef const TArray<UEdGraph*> FEdGraphArray;
#else
typedef const TArray<TObjectPtr<UEdGraph>> FEdGraphArray;
#endif

class FHardReferenceFinderSearchData
{
public:
	TArray<FHRFTreeViewItemPtr> GatherSearchData(TWeakPtr<FBlueprintEditor> BlueprintEditor);

	int GetNumPackagesReferenced() const { return TreeView.Num(); }

private:	
	void Reset();
	UObject* GetObjectContext(TWeakPtr<FBlueprintEditor> BlueprintEditor) const;
	void GetBlueprintDependencies(TArray<FName>& OutPackageDependencies, FAssetRegistryModule& AssetRegistryModule, TWeakPtr<FBlueprintEditor> BlueprintEditor) const;
	void SearchGraphNodes(TMap<FName, FHRFTreeViewItemPtr>& OutPackageMap, const FAssetRegistryModule& AssetRegistryModule, const FEdGraphArray& EdGraphList) const;
	void SearchNodePins(TMap<FName, FHRFTreeViewItemPtr>& OutPackageMap, const FAssetRegistryModule& AssetRegistryModule, const UEdGraphNode* Node) const;
	void SearchMemberVariables(TMap<FName, FHRFTreeViewItemPtr>& OutPackageMap, const FAssetRegistryModule& AssetRegistryModule, UBlueprint* Blueprint);
	FHRFTreeViewItemPtr CheckAddPackageResult(TMap<FName, FHRFTreeViewItemPtr>& OutPackageMap, const FAssetRegistryModule& AssetRegistryModule, const UPackage* Package) const;
	
	void GetAssetForPackages(const TArray<FName>& PackageNames, TMap<FName, FAssetData>& OutPackageToAssetData) const;
	bool TryGetAssetPackageData(FName PathName, FAssetPackageData& OutPackageData, const FAssetRegistryModule& AssetRegistryModule) const;
	FString GetAssetTypeName(const FAssetData& AssetData) const;
	FAssetData GetAssetDataForObject(const UObject* Object) const;
	int64 GatherAssetSizeByName(const FName& AssetName, FAssetRegistryModule& AssetRegistryModule) const;
	int64 GatherAssetSizeRecursive(const FName& OutFrontier, TSet<FName>& OutVisited, FAssetRegistryModule& AssetRegistryModule) const;
	
	TArray<FHRFTreeViewItemPtr> TreeView;
};

