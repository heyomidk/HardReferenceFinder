// Copyright Epic Games, Inc. All Rights Reserved.

#include "HardReferenceViewerSummoner.h"
#include "HardReferenceViewerStyle.h"
#include "BlueprintEditor.h"
#include "K2Node_CallFunction.h"
#include "K2Node_DynamicCast.h"
#include "SEditorViewportToolBarMenu.h"
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
	TSharedPtr<FBlueprintEditor> BlueprintEditorPtr = StaticCastSharedPtr<FBlueprintEditor>(HostingApp.Pin());
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	// Get this blueprints package dependencies from the blueprint editor 
	TArray<FName> PackageDependencies;
	{
		TSharedPtr<SSubobjectEditor> SubobjectEditorPtr = BlueprintEditorPtr->GetSubobjectEditor();
		SSubobjectEditor* SubobjectEditorWidget = SubobjectEditorPtr.Get();

		UObject* Object = SubobjectEditorWidget->GetObjectContext();
		FString ObjectPath = Object->GetPathName();

		FAssetData ExistingAsset = AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(ObjectPath));

		UE::AssetRegistry::FDependencyQuery Flags(UE::AssetRegistry::EDependencyQuery::Hard);
		AssetRegistryModule.GetDependencies(ExistingAsset.PackageName, PackageDependencies, UE::AssetRegistry::EDependencyCategory::Package, Flags);
	}


	// Populate display information from package dependencies
	int64 TotalSize = 0;
	TMap<FName, FHRVPackageData> ExternalPackageList;
	{
		// Get disk size for dependencies
		for(const FName& DependencyName : PackageDependencies)
		{
			FAssetPackageData AssetPackageData;
			AssetRegistryModule.TryGetAssetPackageData(DependencyName, AssetPackageData);

			FHRVPackageData& Header = ExternalPackageList.FindOrAdd(DependencyName);
			Header.DisplayText = FText::Format(LOCTEXT("HeaderEntry", "Package: {0}"), FText::FromName(DependencyName));
			Header.SizeOnDisk = AssetPackageData.DiskSize;

			TotalSize += AssetPackageData.DiskSize;
		}

		// Search through blueprint nodes for references to the dependent packages
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
							const FName PackageName = FunctionPackage->GetFName();
							if(FHRVPackageData* Header = ExternalPackageList.Find(PackageName))
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

	// Visualize external packages and linking nodes
	TSharedPtr<SVerticalBox> VerticalBox = SNew(SVerticalBox);
	{
		{
			FText SummaryText = FText::Format(LOCTEXT("SummaryMessage", "This blueprint makes {0} references to other packages. TotalSize={1}"), PackageDependencies.Num(), TotalSize);
			VerticalBox->AddSlot()
			.AutoHeight()
			[
				SNew(STextBlock).Text(SummaryText)
			];
		}

		for (auto MapIt = ExternalPackageList.CreateConstIterator(); MapIt; ++MapIt)
		{
			const FHRVPackageData& HeadingEntry = MapIt.Value();

			VerticalBox->AddSlot()
			.AutoHeight()
			.Padding(10.f, 1.f)
			[
				SNew(STextBlock).Text(HeadingEntry.DisplayText)
			];

			for(const FHRVNodeData& Link : HeadingEntry.ReferencingNodes)
			{
				VerticalBox->AddSlot()
				.AutoHeight()
				.Padding(20.f, 1.f)
				[
					SNew(STextBlock).Text(Link.DisplayText)
				];
			}
		}
	}
	
	return VerticalBox->AsShared();
}

FText FHardReferenceViewerSummoner::GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const
{
	return LOCTEXT("TabTooltip", "Shows the hard referencing nodes in this Blueprint");
}

#undef LOCTEXT_NAMESPACE
