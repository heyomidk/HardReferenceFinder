#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Templates/SharedPointer.h"
#include "Textures/SlateIcon.h"

class FBlueprintEditor;
class UEdGraph;
class UEdGraphNode;
class UK2Node_FunctionEntry;
class USCS_Node;

class FHRFTreeViewItem : public TSharedFromThis<FHRFTreeViewItem>
{
public:

	bool bIsHeader = false;
	int SizeOnDisk = 0;
	FName PackageId = NAME_None;
	FText Name;
	FText Tooltip;
	FGuid NodeGuid;
	FName SCSIdentifier = NAME_None;
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
	void SearchBlueprintClassProperties(TMap<FName, FHRFTreeViewItemPtr>& OutPackageMap, const FAssetRegistryModule& AssetRegistryModule, UBlueprint* Blueprint) const;
	void SearchFunctionReferences(TMap<FName, FHRFTreeViewItemPtr>& OutPackageMap, const FAssetRegistryModule& AssetRegistryModule, const UBlueprint* Blueprint) const;
	void SearchSimpleConstructionScript(TMap<FName, FHRFTreeViewItemPtr>& OutPackageMap, const FAssetRegistryModule& AssetRegistryModule, const UBlueprint* Blueprint) const;

	UK2Node_FunctionEntry* FindGraphNodeForFunction(const UBlueprint* Blueprint, UFunction* FunctionToFind) const;
	void FindPackagesInSCSNode(TSet<UPackage*>& OutReferencedPackages,const USCS_Node* SCSNode) const;
	TArray<UPackage*> FindPackagesForProperty(FSlateIcon& OutResultIcon, const UObject* ContainerPtr, const FProperty* TargetProperty) const;
	FHRFTreeViewItemPtr CheckAddPackageResult(TMap<FName, FHRFTreeViewItemPtr>& OutPackageMap, const FAssetRegistryModule& AssetRegistryModule, const UPackage* Package) const;
	
	void GetAssetForPackages(const TArray<FName>& PackageNames, TMap<FName, FAssetData>& OutPackageToAssetData) const;
	bool TryGetAssetPackageData(FName PathName, FAssetPackageData& OutPackageData, const FAssetRegistryModule& AssetRegistryModule) const;
	FString GetAssetTypeName(const FAssetData& AssetData) const;
	FAssetData GetAssetDataForObject(const UObject* Object) const;
	int64 GatherAssetSizeByName(const FName& AssetName, FAssetRegistryModule& AssetRegistryModule) const;
	int64 GatherAssetSizeRecursive(const FName& OutFrontier, TSet<FName>& OutVisited, FAssetRegistryModule& AssetRegistryModule) const;
	
	TArray<FHRFTreeViewItemPtr> TreeView;
};

