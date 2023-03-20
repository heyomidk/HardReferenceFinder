#include "HardReferenceViewerSearchData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "BlueprintEditor.h"
#include "K2Node_CallFunction.h"
#include "K2Node_DynamicCast.h"
#include "SSubobjectEditor.h"

#define LOCTEXT_NAMESPACE "FHardReferenceViewerModule"

void FHardReferenceViewerSearchData::GatherSearchData(TSharedPtr<FBlueprintEditor> BlueprintEditor)
{
	Reset();
	
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	
	// Get this blueprints package dependencies from the blueprint editor 
	TArray<FName> PackageDependencies;
	{
		TSharedPtr<SSubobjectEditor> SubobjectEditorPtr = BlueprintEditor->GetSubobjectEditor();
		SSubobjectEditor* SubobjectEditorWidget = SubobjectEditorPtr.Get();

		UObject* Object = SubobjectEditorWidget->GetObjectContext();
		FString ObjectPath = Object->GetPathName();

		FAssetData ExistingAsset = AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(ObjectPath));

		UE::AssetRegistry::FDependencyQuery Flags(UE::AssetRegistry::EDependencyQuery::Hard);
		AssetRegistryModule.GetDependencies(ExistingAsset.PackageName, PackageDependencies, UE::AssetRegistry::EDependencyCategory::Package, Flags);
		
		FAssetPackageData AssetPackageData;
		AssetRegistryModule.TryGetAssetPackageData(ExistingAsset.PackageName, AssetPackageData);
		SizeOnDisk = AssetPackageData.DiskSize; 
	}
	
	// Populate display information from package dependencies
	{
		// Get disk size for dependencies
		for(const FName& DependencyName : PackageDependencies)
		{
			FAssetPackageData AssetPackageData;
			AssetRegistryModule.TryGetAssetPackageData(DependencyName, AssetPackageData);

			FHRVPackageData& Header = PackageMap.FindOrAdd(DependencyName);
			Header.DisplayText = FText::Format(LOCTEXT("HeaderEntry", "Package: {0}"), FText::FromName(DependencyName));
			Header.SizeOnDisk = AssetPackageData.DiskSize;
		}

		// Search through blueprint nodes for references to the dependent packages
		if( UBlueprint* Blueprint = BlueprintEditor->GetBlueprintObj() )
		{
			for(UEdGraph* Graph : Blueprint->UbergraphPages)
			{
				if(Graph)
				{
					for (UEdGraphNode* Node : Graph->Nodes)
					{
						UPackage* FunctionPackage = nullptr;
						if(UK2Node_CallFunction* CallFunctionNode = Cast<UK2Node_CallFunction>(Node))
						{
							FunctionPackage = CallFunctionNode->FunctionReference.GetMemberParentPackage();
						}
						else if(UK2Node_DynamicCast* CastNode = Cast<UK2Node_DynamicCast>(Node))
						{
							if(CastNode->TargetType)
							{
								FunctionPackage = CastNode->TargetType->GetPackage();
							}
						}

						if( FunctionPackage )
						{
							const FName PackageName = FunctionPackage->GetFName();
							if(FHRVPackageData* Header = PackageMap.Find(PackageName))
							{
								FAssetPackageData AssetPackageData;
								AssetRegistryModule.TryGetAssetPackageData(PackageName, AssetPackageData);

								FHRVNodeData& LinkData  = Header->ReferencingNodes.AddDefaulted_GetRef();
								LinkData.DisplayText = Node->GetNodeTitle(ENodeTitleType::ListView);
								LinkData.NodeGuid = Node->NodeGuid;
							}
						}
					}
				}
			}
		}
	}
}

void FHardReferenceViewerSearchData::Reset()
{
	SizeOnDisk = 0;
	PackageMap.Reset();
}


TArray<TSharedPtr<FHRVTreeViewItem>> FHardReferenceViewerSearchData::GetAsTreeViewResults()
{
	TArray<TSharedPtr<FHRVTreeViewItem>> TreeResults;

	for (auto MapIt = PackageMap.CreateConstIterator(); MapIt; ++MapIt)
	{
		const FHRVPackageData& HeadingEntry = MapIt.Value();

		TSharedPtr<FHRVTreeViewItem> HeaderItem = MakeShared<FHRVTreeViewItem>();
		TreeResults.Add(HeaderItem);
		
		HeaderItem->bIsCategoryHeader = true;
		HeaderItem->CategorySizeOnDisk = HeadingEntry.SizeOnDisk;
		HeaderItem->DisplayText = HeadingEntry.DisplayText;

		for(const FHRVNodeData& Link : HeadingEntry.ReferencingNodes)
		{
			TSharedPtr<FHRVTreeViewItem> ChildItem = MakeShared<FHRVTreeViewItem>();
			HeaderItem->Children.Add(ChildItem);

			ChildItem->DisplayText = Link.DisplayText;
			ChildItem->NodeGuid = Link.NodeGuid;
		}
	}

	// sort from largest to smallest
	TreeResults.Sort([](TSharedPtr<FHRVTreeViewItem> Lhs, TSharedPtr<FHRVTreeViewItem> Rhs)
	{
		return Lhs->CategorySizeOnDisk > Rhs->CategorySizeOnDisk;
	});
	
	return TreeResults;	
}

#undef LOCTEXT_NAMESPACE
