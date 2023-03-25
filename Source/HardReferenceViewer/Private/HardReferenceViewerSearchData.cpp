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

	TMap<FName, FHRVTreeViewItemPtr> DependentPackageMap;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	
	// Get this blueprints package dependencies from the blueprint editor 
	TArray<FName> PackageDependencies;
	GetPackageDependencies(PackageDependencies, SizeOnDisk, AssetRegistryModule, BlueprintEditor);

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
				Header->PackageId = PathName;
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
			
				DependentPackageMap.Add(PathName, Header);
				TreeView.Add(Header);
			}
		}
	}
	
	{
		// Search through blueprint nodes for references to the dependent packages
		if( UBlueprint* Blueprint = BlueprintEditor.Pin()->GetBlueprintObj() )
		{
			SearchGraphNodes(DependentPackageMap, AssetRegistryModule, Blueprint->UbergraphPages);
			SearchGraphNodes(DependentPackageMap, AssetRegistryModule, Blueprint->FunctionGraphs);
			// @omidk todo: Search member variables
			// @omidk todo: Search components
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

UObject* FHardReferenceViewerSearchData::GetObjectContext(TWeakPtr<FBlueprintEditor> BlueprintEditor) const
{
	if(!BlueprintEditor.IsValid())
	{
		return nullptr;
	}
	
	TSharedPtr<SSubobjectEditor> SubobjectEditorPtr = BlueprintEditor.Pin()->GetSubobjectEditor();
	SSubobjectEditor* SubobjectEditorWidget = SubobjectEditorPtr.Get();
	if(SubobjectEditorWidget == nullptr)
	{
		return nullptr;
	}
		
	UObject* Object = SubobjectEditorWidget->GetObjectContext();
	return Object;
}

void FHardReferenceViewerSearchData::GetPackageDependencies(TArray<FName>& OutPackageDependencies, int& OutSizeOnDisk, FAssetRegistryModule& AssetRegistryModule, TWeakPtr<FBlueprintEditor> BlueprintEditor) const
{
	UObject* Object = GetObjectContext(BlueprintEditor);
	if(Object == nullptr)
	{
		return;
	}
	
	FString ObjectPath = Object->GetPathName();
	FAssetData ExistingAsset = AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(ObjectPath));

	UE::AssetRegistry::FDependencyQuery Flags(UE::AssetRegistry::EDependencyQuery::Hard);
	AssetRegistryModule.GetDependencies(ExistingAsset.PackageName, OutPackageDependencies, UE::AssetRegistry::EDependencyCategory::Package, Flags);
	
	FAssetPackageData AssetPackageData;
	AssetRegistryModule.TryGetAssetPackageData(ExistingAsset.PackageName, AssetPackageData);
	OutSizeOnDisk = AssetPackageData.DiskSize;
}

void FHardReferenceViewerSearchData::SearchGraphNodes(TMap<FName, FHRVTreeViewItemPtr>& OutPackageMap, const FAssetRegistryModule& AssetRegistryModule, const TArray<TObjectPtr<UEdGraph>>& EdGraphList) const
{
	for(UEdGraph* Graph : EdGraphList)
	{
		if(Graph)
		{
			for (const UEdGraphNode* Node : Graph->Nodes)
			{
				UPackage* FunctionPackage = nullptr;
				if(const UK2Node_CallFunction* CallFunctionNode = Cast<UK2Node_CallFunction>(Node))
				{
					FunctionPackage = CallFunctionNode->FunctionReference.GetMemberParentPackage();
				}
				else if(const UK2Node_DynamicCast* CastNode = Cast<UK2Node_DynamicCast>(Node))
				{
					if(CastNode->TargetType)
					{
						FunctionPackage = CastNode->TargetType->GetPackage();
					}
				}

				if( FHRVTreeViewItemPtr Result = CheckAddPackageResult(OutPackageMap, AssetRegistryModule, FunctionPackage) )
				{
					Result->Name = Node->GetNodeTitle(ENodeTitleType::ListView);
					Result->NodeGuid = Node->NodeGuid;
					Result->SlateIcon = Node->GetIconAndTint(Result->IconColor);
				}

				// Also search the pins of this node for any references to other packages
				SearchNodePins(OutPackageMap, AssetRegistryModule, Node);
			}
		}
	}
}

void FHardReferenceViewerSearchData::SearchNodePins(TMap<FName, FHRVTreeViewItemPtr>& OutPackageMap, const FAssetRegistryModule& AssetRegistryModule, const UEdGraphNode* Node) const
{
	if(Node == nullptr)
	{
		return;
	}
	
	for(const UEdGraphPin* Pin : Node->Pins)
	{
		if(Pin->bHidden)
		{
			// skip hidden pins
			continue;
		}
					
		if(Pin->Direction == EGPD_Input)
		{
			if(const UObject* PinObject = Pin->DefaultObject)
			{
				UPackage* FunctionPackage = PinObject->GetPackage();
				if( FHRVTreeViewItemPtr Result = CheckAddPackageResult(OutPackageMap, AssetRegistryModule, FunctionPackage) )
				{
					Result->Name = FText::Format(LOCTEXT("FunctionInput","{0} ({1})"), FText::FromString(Pin->GetName()), Node->GetNodeTitle(ENodeTitleType::ListView));
					Result->NodeGuid = Node->NodeGuid;
					if( const UEdGraphSchema* Schema = Pin->GetSchema() )
					{
						Result->IconColor = Schema->GetPinTypeColor(Pin->PinType);
					}
					Result->SlateIcon = FSlateIcon("EditorStyle", "Graph.Pin.Disconnected_VarA");
				}
			}
		}
	}
}

FHRVTreeViewItemPtr FHardReferenceViewerSearchData::CheckAddPackageResult(TMap<FName, FHRVTreeViewItemPtr>& OutPackageMap, const FAssetRegistryModule& AssetRegistryModule, const UPackage* Package) const
{
	if( Package )
	{
		const FName PackageName = Package->GetFName();
		if(FHRVTreeViewItemPtr* FoundHeader = OutPackageMap.Find(PackageName))
		{
			FHRVTreeViewItemPtr Header = *FoundHeader;
								
			FAssetPackageData AssetPackageData;
			UE::AssetRegistry::EExists Result = AssetRegistryModule.TryGetAssetPackageData(PackageName, AssetPackageData);
			if(ensure(Result == UE::AssetRegistry::EExists::Exists))
			{
				FHRVTreeViewItemPtr Link = MakeShared<FHRVTreeViewItem>();
				Header->Children.Add(Link);

				return Link; 
			}
		}
	}

	return  nullptr;
}


#undef LOCTEXT_NAMESPACE
