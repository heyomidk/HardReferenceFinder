// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WorkflowOrientedApp/WorkflowTabFactory.h"

class FBlueprintEditor;

struct FHardReferenceViewerSummoner : public FWorkflowTabFactory
{
public:
	// ---------------------------------------------------------------------------------
	// TODO: @omidk move me to a better home
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

	struct FHRVSearchData
	{
		int SizeOnDisk = 0;
		TMap<FName, FHRVPackageData> PackageMap;
	};
	static FHRVSearchData BuildSearchData(TSharedPtr<FBlueprintEditor> BlueprintEditor);
	// ---------------------------------------------------------------------------------

	FHardReferenceViewerSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp);

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;

	virtual FText GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const override;
};
