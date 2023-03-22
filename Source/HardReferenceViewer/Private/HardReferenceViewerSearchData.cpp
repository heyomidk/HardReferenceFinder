#include "HardReferenceViewerSearchData.h"
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "BlueprintEditor.h"
#include "K2Node_CallFunction.h"
#include "K2Node_DynamicCast.h"
#include "SSubobjectEditor.h"

#define LOCTEXT_NAMESPACE "FHardReferenceViewerModule"

TArray<FHRVTreeViewItemPtr> FHardReferenceViewerSearchData::GatherSearchData(TWeakPtr<FBlueprintEditor> BlueprintEditor)
{
	Reset();

	if(!BlueprintEditor.IsValid())
	{
		return TArray<FHRVTreeViewItemPtr>();
	}

	TMap<FName, FHRVTreeViewItemPtr> PackageMap;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	
	// Get this blueprints package dependencies from the blueprint editor 
	TArray<FName> PackageDependencies;
	{
		TSharedPtr<SSubobjectEditor> SubobjectEditorPtr = BlueprintEditor.Pin()->GetSubobjectEditor();
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
		TMap<FName, FAssetData> DependencyToAssetDataMap;
		UE::AssetRegistry::GetAssetForPackages(PackageDependencies, DependencyToAssetDataMap);

		for (auto MapIt = DependencyToAssetDataMap.CreateConstIterator(); MapIt; ++MapIt)
		{
			const FName& PathName = MapIt.Key();
			const FAssetData& AssetData = MapIt.Value();
			FString AssetTypeName = AssetData.AssetClassPath.GetAssetName().ToString();
			FString FileName = FPaths::GetCleanFilename(PathName.ToString());
		
			FAssetPackageData AssetPackageData;
			AssetRegistryModule.TryGetAssetPackageData(PathName, AssetPackageData);

			if( FHRVTreeViewItemPtr Header = MakeShared<FHRVTreeViewItem>() )
			{
				Header->bIsHeader = true;
				Header->Tooltip = FText::FromName(PathName);
				Header->Name = FText::FromString(FileName);
				Header->SizeOnDisk = AssetPackageData.DiskSize;
				Header->SlateIcon = FSlateIcon("EditorStyle", FName( *("ClassIcon." + AssetTypeName))); 

				FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));	
				if (UClass* AssetClass = AssetData.GetClass())
				{
					TWeakPtr<IAssetTypeActions> AssetTypeActions = AssetToolsModule.Get().GetAssetTypeActionsForClass(AssetData.GetClass());
					if(AssetTypeActions.IsValid())
					{
						Header->IconColor = AssetTypeActions.Pin()->GetTypeColor();
					}
				}
			
				PackageMap.Add(PathName, Header);
				TreeView.Add(Header);
			}
		}
	}
	
	{
		// Search through blueprint nodes for references to the dependent packages
		if( UBlueprint* Blueprint = BlueprintEditor.Pin()->GetBlueprintObj() )
		{
			for(UEdGraph* Graph : Blueprint->UbergraphPages)
			{
				if(Graph)
				{
					// @omidk todo: Handle spawn nodes somehow? Find a generic way parse function input parameters  
					// @omidk todo: Handle member variables
					// @omidk todo: Handle Components
					
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
							if(FHRVTreeViewItemPtr* FoundHeader = PackageMap.Find(PackageName))
							{
								FHRVTreeViewItemPtr Header = *FoundHeader;
								
								FAssetPackageData AssetPackageData;
								UE::AssetRegistry::EExists Result = AssetRegistryModule.TryGetAssetPackageData(PackageName, AssetPackageData);
								if(ensure(Result == UE::AssetRegistry::EExists::Exists))
								{
									FHRVTreeViewItemPtr Link = MakeShared<FHRVTreeViewItem>();
								
									Link->Name = Node->GetNodeTitle(ENodeTitleType::ListView);
									Link->NodeGuid = Node->NodeGuid;
									Link->SlateIcon = Node->GetIconAndTint(Link->IconColor);

									Header->Children.Add(Link);
								}
							}
						}
					}
				}
			}
		}
	}

	// If we didn't discover any references to a package make a note
	for(FHRVTreeViewItemPtr HeaderItem : TreeView)
	{
		if(HeaderItem->Children.Num() <= 0)
		{
			FHRVTreeViewItemPtr ChildItem = MakeShared<FHRVTreeViewItem>();
			HeaderItem->Children.Add(ChildItem);
			ChildItem->Name = LOCTEXT("UnknownSource", "Unknown source");
		}
	}
	
	// sort from largest to smallest
	TreeView.Sort([](FHRVTreeViewItemPtr Lhs, FHRVTreeViewItemPtr Rhs)
	{
		return Lhs->SizeOnDisk > Rhs->SizeOnDisk;
	});
	
	return TreeView;
}

void FHardReferenceViewerSearchData::Reset()
{
	SizeOnDisk = 0;
	TreeView.Reset();
}

#undef LOCTEXT_NAMESPACE
