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
	void SearchGraphNodes(TMap<FName, FHRVTreeViewItemPtr>& OutPackageMap, const FAssetRegistryModule& AssetRegistryModule, const TArray<TObjectPtr<UEdGraph>>& EdGraphList) const;
	void SearchNodePins(TMap<FName, FHRVTreeViewItemPtr>& OutPackageMap, const FAssetRegistryModule& AssetRegistryModule, const UEdGraphNode* Node) const;
	FHRVTreeViewItemPtr CheckAddPackageResult(TMap<FName, FHRVTreeViewItemPtr>& OutPackageMap, const FAssetRegistryModule& AssetRegistryModule, const UPackage* Package) const;

	int SizeOnDisk = 0;
	TArray<FHRVTreeViewItemPtr> TreeView;
};
