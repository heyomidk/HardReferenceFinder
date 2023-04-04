// Copyright Epic Games, Inc. All Rights Reserved.

#include "HardReferenceFinderSummoner.h"
#include "HardReferenceFinderStyle.h"
#include "BlueprintEditor.h"
#include "SHardReferenceFinderWindow.h"

#define LOCTEXT_NAMESPACE "FHardReferenceFinderModule"

static const FName HardReferenceFinderID( TEXT( "HardReferenceFinder") );

FHardReferenceFinderSummoner::FHardReferenceFinderSummoner(TSharedPtr<FAssetEditorToolkit> InHostingApp)
	: FWorkflowTabFactory(HardReferenceFinderID, InHostingApp)
{
	TabLabel = LOCTEXT("HardReferenceFinderTabTitle", "Hard References");
	TabIcon = FSlateIcon("EditorStyle","ContentBrowser.ReferenceViewer");

	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("HardReferenceFinderView", "Hard Reference Viewer");
	ViewMenuTooltip = LOCTEXT("HardReferenceFinderView_ToolTip", "Shows hard referencing nodes associated with this Blueprint");
}

TSharedRef<SWidget> FHardReferenceFinderSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	const TSharedPtr<FBlueprintEditor> BlueprintEditorPtr = StaticCastSharedPtr<FBlueprintEditor>(HostingApp.Pin());
	return SNew(SHardReferenceFinderWindow, BlueprintEditorPtr);	
}

FText FHardReferenceFinderSummoner::GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const
{
	return LOCTEXT("TabTooltip", "Shows the hard referencing nodes in this Blueprint");
}

#undef LOCTEXT_NAMESPACE
