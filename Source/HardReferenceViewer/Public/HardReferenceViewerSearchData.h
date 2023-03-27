#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Templates/SharedPointer.h"

class FBlueprintEditor;

class FHRVTreeViewItem : public TSharedFromThis<FHRVTreeViewItem>
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
	TArray<TSharedPtr<FHRVTreeViewItem>> Children;
};
typedef TSharedPtr<FHRVTreeViewItem> FHRVTreeViewItemPtr;

#if ENGINE_MAJOR_VERSION < 5
typedef const TArray<UEdGraph*> FEdGraphArray;
#else
typedef const TArray<TObjectPtr<UEdGraph>> FEdGraphArray;
#endif

class FHardReferenceViewerSearchData
{
public:
	TArray<FHRVTreeViewItemPtr> GatherSearchData(TWeakPtr<FBlueprintEditor> BlueprintEditor);

	int GetSizeOnDisk() const { return SizeOnDisk; }
	int GetNumPackagesReferenced() const { return TreeView.Num(); }

private:	
	void Reset();
	UObject* GetObjectContext(TWeakPtr<FBlueprintEditor> BlueprintEditor) const;
	void GetPackageDependencies(TArray<FName>& OutPackageDependencies, int& OutSizeOnDisk, FAssetRegistryModule& AssetRegistryModule, TWeakPtr<FBlueprintEditor> BlueprintEditor) const;
	void SearchGraphNodes(TMap<FName, FHRVTreeViewItemPtr>& OutPackageMap, const FAssetRegistryModule& AssetRegistryModule, const FEdGraphArray& EdGraphList) const;
	void SearchNodePins(TMap<FName, FHRVTreeViewItemPtr>& OutPackageMap, const FAssetRegistryModule& AssetRegistryModule, const UEdGraphNode* Node) const;
	void SearchMemberVariables(TMap<FName, FHRVTreeViewItemPtr>& OutPackageMap, const FAssetRegistryModule& AssetRegistryModule, UBlueprint* Blueprint);
	FHRVTreeViewItemPtr CheckAddPackageResult(TMap<FName, FHRVTreeViewItemPtr>& OutPackageMap, const FAssetRegistryModule& AssetRegistryModule, const UPackage* Package) const;

	void GetAssetForPackages(const TArray<FName>& PackageNames, TMap<FName, FAssetData>& OutPackageToAssetData) const;
	bool TryGetAssetPackageData(FName PathName, FAssetPackageData& OutPackageData) const;
	FString GetAssetTypeName(const FAssetData& AssetData) const;
	FAssetData GetAssetDataForObject(const UObject* Object) const;
	
	int SizeOnDisk = 0;
	TArray<FHRVTreeViewItemPtr> TreeView;
};

