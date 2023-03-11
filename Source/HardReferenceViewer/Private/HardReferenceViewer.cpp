// Copyright Epic Games, Inc. All Rights Reserved.

#include "HardReferenceViewer.h"
#include "HardReferenceViewerStyle.h"
#include "WorkflowOrientedApp/WorkflowTabManager.h"
#include "BlueprintEditor.h"
#include "HardReferenceViewerSummoner.h"

static const FName HardReferenceViewerTabName("HardReferenceViewer");

#define LOCTEXT_NAMESPACE "FHardReferenceViewerModule"

void FHardReferenceViewerModule::StartupModule()
{
	FHardReferenceViewerStyle::Initialize();
	FHardReferenceViewerStyle::ReloadTextures();

	FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
	BlueprintEditorModule.OnRegisterTabsForEditor().AddRaw(this, &FHardReferenceViewerModule::RegisterBlueprintTabs);
}

void FHardReferenceViewerModule::ShutdownModule()
{
	FHardReferenceViewerStyle::Shutdown();
	
	FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
	BlueprintEditorModule.OnRegisterTabsForEditor().RemoveAll(this);
}

void FHardReferenceViewerModule::RegisterBlueprintTabs(FWorkflowAllowedTabSet& TabFactory, FName ModeName, TSharedPtr<FBlueprintEditor> InBlueprintEditor) const
{
	TabFactory.RegisterFactory(MakeShareable(new FHardReferenceViewerSummoner(InBlueprintEditor)));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FHardReferenceViewerModule, HardReferenceViewer)
