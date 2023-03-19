// Copyright Epic Games, Inc. All Rights Reserved.

#include "HardReferenceViewerSummoner.h"
#include "HardReferenceViewerStyle.h"
#include "BlueprintEditor.h"
#include "SHardReferenceViewerWindow.h"

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
	const TSharedPtr<FBlueprintEditor> BlueprintEditorPtr = StaticCastSharedPtr<FBlueprintEditor>(HostingApp.Pin());
	return SNew(SHardReferenceViewerWindow, BlueprintEditorPtr);	
}

FText FHardReferenceViewerSummoner::GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const
{
	return LOCTEXT("TabTooltip", "Shows the hard referencing nodes in this Blueprint");
}

#undef LOCTEXT_NAMESPACE
