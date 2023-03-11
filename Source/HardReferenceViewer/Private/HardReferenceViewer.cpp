// Copyright Epic Games, Inc. All Rights Reserved.

#include "HardReferenceViewer.h"
#include "HardReferenceViewerStyle.h"
#include "HardReferenceViewerCommands.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"

static const FName HardReferenceViewerTabName("HardReferenceViewer");

#define LOCTEXT_NAMESPACE "FHardReferenceViewerModule"

void FHardReferenceViewerModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FHardReferenceViewerStyle::Initialize();
	FHardReferenceViewerStyle::ReloadTextures();

	FHardReferenceViewerCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FHardReferenceViewerCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FHardReferenceViewerModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FHardReferenceViewerModule::RegisterMenus));
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(HardReferenceViewerTabName, FOnSpawnTab::CreateRaw(this, &FHardReferenceViewerModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FHardReferenceViewerTabTitle", "HardReferenceViewer"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FHardReferenceViewerModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FHardReferenceViewerStyle::Shutdown();

	FHardReferenceViewerCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(HardReferenceViewerTabName);
}

TSharedRef<SDockTab> FHardReferenceViewerModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	FText WidgetText = FText::Format(
		LOCTEXT("WindowWidgetText", "Add code to {0} in {1} to override this window's contents"),
		FText::FromString(TEXT("FHardReferenceViewerModule::OnSpawnPluginTab")),
		FText::FromString(TEXT("HardReferenceViewer.cpp"))
		);

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			// Put your tab content here!
			SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(WidgetText)
			]
		];
}

void FHardReferenceViewerModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(HardReferenceViewerTabName);
}

void FHardReferenceViewerModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FHardReferenceViewerCommands::Get().OpenPluginWindow, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FHardReferenceViewerCommands::Get().OpenPluginWindow));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FHardReferenceViewerModule, HardReferenceViewer)