// Copyright Epic Games, Inc. All Rights Reserved.

#include "HardReferenceViewerCommands.h"

#define LOCTEXT_NAMESPACE "FHardReferenceViewerModule"

void FHardReferenceViewerCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "HardReferenceViewer", "Bring up HardReferenceViewer window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
