
#include "SHardReferenceViewerWindow.h"
#include "HardReferenceViewerSearchData.h"

#define LOCTEXT_NAMESPACE "FHardReferenceViewerModule"

void SHardReferenceViewerWindow::Construct(const FArguments& InArgs, TSharedPtr<FBlueprintEditor> BlueprintGraph)
{
	SearchData = MakeShared<FHardReferenceViewerSearchData>();
	SearchData->Generate(BlueprintGraph);

	// @omidk TODO: Use SAssignNew(TreeView, STreeView<...>) to create a collapsible list of headers and elements
	// Visualize external packages and linking nodes
	TSharedPtr<SVerticalBox> ResultsData = SNew(SVerticalBox);
	{
		for (auto MapIt = SearchData->GetPackageMap().CreateConstIterator(); MapIt; ++MapIt)
		{
			const FHRVPackageData& HeadingEntry = MapIt.Value();

			ResultsData->AddSlot()
			.AutoHeight()
			.Padding(10.f, 1.f)
			[
				SNew(STextBlock).Text(HeadingEntry.DisplayText)
			];

			for(const FHRVNodeData& Link : HeadingEntry.ReferencingNodes)
			{
				ResultsData->AddSlot()
				.AutoHeight()
				.Padding(20.f, 1.f)
				[
					SNew(STextBlock).Text(Link.DisplayText)
				];

				// @omidk TODO: On click, use a function like this to zoom in on the selected node
				//if(	UEdGraphNode* GraphNode = FBlueprintEditorUtils::GetNodeByGUID(Blueprint, NodeGuid) )
				//{
				//	FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(GraphNode);
				//	return FReply::Handled();
				//}
			}
		}
	}

	const FText SummaryText = FText::Format(LOCTEXT("SummaryMessage", "This blueprint makes {0} references to other packages. DiskSize={1}MB"), SearchData->GetPackageMap().Num(), SearchData->GetSizeOnDisk());
	
	ChildSlot[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(10.f)
		[
			SNew(STextBlock)
			.Text(SummaryText)
		]
		+ SVerticalBox::Slot()
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
			.Padding(FMargin(8.f, 8.f, 4.f, 0.f))
			[
				ResultsData->AsShared()
			]
		]
	];
}

#undef LOCTEXT_NAMESPACE
