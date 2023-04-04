// Copyright Epic Games, Inc. All Rights Reserved.

#include "HardReferenceFinder.h"
#include "HardReferenceFinderStyle.h"
#include "WorkflowOrientedApp/WorkflowTabManager.h"
#include "BlueprintEditor.h"
#include "HardReferenceFinderSummoner.h"

static const FName HardReferenceFinderTabName("HardReferenceFinder");

#define LOCTEXT_NAMESPACE "FHardReferenceFinderModule"

void FHardReferenceFinderModule::StartupModule()
{
	FHardReferenceFinderStyle::Initialize();
	FHardReferenceFinderStyle::ReloadTextures();

	FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
	BlueprintEditorModule.OnRegisterTabsForEditor().AddRaw(this, &FHardReferenceFinderModule::RegisterBlueprintTabs);
}

void FHardReferenceFinderModule::ShutdownModule()
{
	FHardReferenceFinderStyle::Shutdown();
	
	FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
	BlueprintEditorModule.OnRegisterTabsForEditor().RemoveAll(this);
}

void FHardReferenceFinderModule::RegisterBlueprintTabs(FWorkflowAllowedTabSet& TabFactory, FName ModeName, TSharedPtr<FBlueprintEditor> InBlueprintEditor) const
{
	TabFactory.RegisterFactory(MakeShareable(new FHardReferenceFinderSummoner(InBlueprintEditor)));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FHardReferenceFinderModule, HardReferenceFinder)
