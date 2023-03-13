// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WorkflowOrientedApp/WorkflowTabFactory.h"

struct FHardReferenceViewerSummoner : public FWorkflowTabFactory
{
public:
	struct FHRVNodeData
	{
		FText DisplayText;
		// TODO: Add some data that lets us link to the node in the blueprint graph
	};

	struct FHRVPackageData
	{
		FText DisplayText;
		int64 SizeOnDisk = 0;
		TArray<FHRVNodeData> ReferencingNodes;
	};

	FHardReferenceViewerSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp);

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;

	virtual FText GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const override;
};
