// Copyright Epic Games, Inc. All Rights Reserved.

#include "HardReferenceViewerSummoner.h"
#include "HardReferenceViewerStyle.h"
#include "BlueprintEditor.h"
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
	{
		// stub code to demonstrates a path to asset data from the blueprint editor 
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

		FText StubText = FText::Format(LOCTEXT("EmptyTabMessage", "This object has {0} references to other packages."), Dependencies.Num());
		return SNew(STextBlock).Text(StubText);
	}
	
	return FWorkflowTabFactory::CreateTabBody(Info);
}

FText FHardReferenceViewerSummoner::GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const
{
	return LOCTEXT("TabTooltip", "Shows the hard referencing nodes in this Blueprint");
}

#undef LOCTEXT_NAMESPACE
