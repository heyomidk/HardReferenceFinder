// Copyright Epic Games, Inc. All Rights Reserved.

#include "HardReferenceViewerSummoner.h"
#include "HardReferenceViewerStyle.h"
#include "BlueprintEditor.h"
#include "K2Node_CallFunction.h"
#include "K2Node_DynamicCast.h"
#include "SSubobjectEditor.h"
#include "AssetRegistry/AssetRegistryModule.h"

#define LOCTEXT_NAMESPACE "FHardReferenceViewerModule"

static const FName HardReferenceViewerID( TEXT( "HardReferenceViewer") );

FHardReferenceViewerSummoner::FHardReferenceViewerSummoner(TSharedPtr<FAssetEditorToolkit> InHostingApp)
	: FWorkflowTabFactory(HardReferenceViewerID, InHostingApp)
{
	TabLabel = LOCTEXT("HardReferenceViewerTabTitle", "Hard Reference Viewer");
	TabIcon = FSlateIcon(FHardReferenceViewerStyle::GetStyleSetName(), "HardReferenceViewer.TabIcon");

	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("HardReferenceViewerView", "Hard Reference Viewer");
	ViewMenuTooltip = LOCTEXT("HardReferenceViewerView_ToolTip", "Shows hard referencing nodes associated with this Blueprint");
}

TSharedRef<SWidget> FHardReferenceViewerSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	// stub a path to asset data / dependencies from the blueprint editor 
	TSharedPtr<FBlueprintEditor> BlueprintEditorPtr = StaticCastSharedPtr<FBlueprintEditor>(HostingApp.Pin());
	TSharedPtr<SSubobjectEditor> SubobjectEditorPtr = BlueprintEditorPtr->GetSubobjectEditor();
	SSubobjectEditor* SubobjectEditorWidget = SubobjectEditorPtr.Get();

	UObject* Object = SubobjectEditorWidget->GetObjectContext();
	FString ObjectPath = Object->GetPathName();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	FAssetData ExistingAsset = AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(ObjectPath));

	TArray<FName> Dependencies;
	UE::AssetRegistry::FDependencyQuery Flags(UE::AssetRegistry::EDependencyQuery::Hard);
	AssetRegistryModule.GetDependencies(ExistingAsset.PackageName, Dependencies, UE::AssetRegistry::EDependencyCategory::Package, Flags);

	// stub a calculation of size for dependencies
	int64 TotalSize = 0;
	for(const FName& Dependency : Dependencies)
	{
		FAssetPackageData AssetPackageData;
		AssetRegistryModule.TryGetAssetPackageData(Dependency, AssetPackageData);

		TotalSize += AssetPackageData.DiskSize;
	}

	// stub in a way to search blueprints for references to dependencies
	TArray<UEdGraphNode*> ReferencingNodes;
	if( UBlueprint* Blueprint = BlueprintEditorPtr->GetBlueprintObj() )
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
						FName Name = FunctionPackage->GetFName();
						if(Dependencies.Contains(Name))
						{
							ReferencingNodes.AddUnique(Node);
						}
					}
				}
			}
		}
	}

	TSharedPtr<SVerticalBox> VerticalBox = SNew(SVerticalBox);
	{
		FText SummaryText = FText::Format(LOCTEXT("SummaryMessage", "This object has {0} references to other packages. TotalSize={1}"), Dependencies.Num(), TotalSize);
		VerticalBox->AddSlot()
		[
			SNew(STextBlock).Text(SummaryText)
		];
	}

	for(UEdGraphNode* Node : ReferencingNodes)
	{
		FText EntryText = FText::Format(LOCTEXT("EntryMessage", "Node {0} "), Node->GetNodeTitle(ENodeTitleType::ListView));
		VerticalBox->AddSlot()
		[
			SNew(STextBlock).Text(EntryText)
		];
	}
		
	// TODO:
	//		- Figure out how to focus a node in the graph
	//		- Find a nice way to format the window
	
	return VerticalBox->AsShared();
}

FText FHardReferenceViewerSummoner::GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const
{
	return LOCTEXT("TabTooltip", "Shows the hard referencing nodes in this Blueprint");
}

#undef LOCTEXT_NAMESPACE
