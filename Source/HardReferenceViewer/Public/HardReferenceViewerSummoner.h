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
		FGuid NodeGuid;
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
