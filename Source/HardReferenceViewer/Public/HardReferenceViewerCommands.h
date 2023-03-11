// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "HardReferenceViewerStyle.h"

class FHardReferenceViewerCommands : public TCommands<FHardReferenceViewerCommands>
{
public:

	FHardReferenceViewerCommands()
		: TCommands<FHardReferenceViewerCommands>(TEXT("HardReferenceViewer"), NSLOCTEXT("Contexts", "HardReferenceViewer", "HardReferenceViewer Plugin"), NAME_None, FHardReferenceViewerStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};