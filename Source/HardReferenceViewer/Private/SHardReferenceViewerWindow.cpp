
#include "SHardReferenceViewerWindow.h"
#include "BlueprintEditor.h"
#include "GraphEditorSettings.h"
#include "HardReferenceViewerSearchData.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"

#define LOCTEXT_NAMESPACE "FHardReferenceViewerModule"

void SHardReferenceViewerWindow::Construct(const FArguments& InArgs, TSharedPtr<FBlueprintEditor> InBlueprintGraph)
{
	BlueprintGraph = InBlueprintGraph;
	
	ChildSlot[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(10.f)
		[
			SAssignNew(HeaderText, STextBlock)
		]
		+ SVerticalBox::Slot()
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
			.Padding(FMargin(8.f, 8.f, 4.f, 0.f))
			[
				SAssignNew(TreeView, SHRVReferenceTreeType)
				.TreeItemsSource(&TreeViewData)
				.OnGetChildren(this, &SHardReferenceViewerWindow::OnGetChildren)
				.OnGenerateRow(this, &SHardReferenceViewerWindow::OnGenerateRow)
				.OnMouseButtonDoubleClick(this, &SHardReferenceViewerWindow::OnDoubleClickTreeEntry)
			]
		]
	];

	InitiateSearch();
}

void SHardReferenceViewerWindow::InitiateSearch()
{
	TreeViewData = SearchData.GatherSearchData(BlueprintGraph);
	
	const FText SummaryText = FText::Format(LOCTEXT("SummaryMessage", "This blueprint makes {0} references to other packages."), SearchData.GetNumPackagesReferenced());
	HeaderText->SetText(SummaryText);
	if(TreeView.IsValid())
	{
		TreeView->RequestTreeRefresh();
	}
}

void SHardReferenceViewerWindow::OnDoubleClickTreeEntry(TSharedPtr<FHRVTreeViewItem> Item) const
{
	if(Item.IsValid() && BlueprintGraph.IsValid())
	{
		if( UBlueprint* BlueprintObj = BlueprintGraph.Pin()->GetBlueprintObj() )
		{
			if( const UEdGraphNode* GraphNode = FBlueprintEditorUtils::GetNodeByGUID(BlueprintObj, Item->NodeGuid) )
			{
				FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(GraphNode);
			}
		}
	}
}

void SHardReferenceViewerWindow::OnGetChildren(FHRVTreeViewItemPtr InItem, TArray<FHRVTreeViewItemPtr>& OutChildren) const
{
	OutChildren += InItem->Children;
}

TSharedRef<ITableRow> SHardReferenceViewerWindow::OnGenerateRow(FHRVTreeViewItemPtr Item, const TSharedRef<STableViewBase>& TableViewBase) const
{
	if(Item->bIsHeader)
	{
		const FText CategoryHeaderText = FText::Format(LOCTEXT("CategoryHeader", "{1} ({0}MB)"), Item->SizeOnDisk/1000.f, Item->Name);

		return SNew(STableRow<TSharedPtr<FName>>, TableViewBase)
			.Style( &FAppStyle::Get().GetWidgetStyle<FTableRowStyle>("ShowParentsTableView.Row") )
			.Padding(FMargin(2.f, 3.f, 2.f, 3.f))
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.Padding(FMargin(0.f, 0.f, 8.f, 0.f))
				.AutoWidth()
				[
					SNew(SImage)
					.Image(Item->SlateIcon.GetOptionalIcon())
					.ColorAndOpacity(Item->IconColor)
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(2.f)
				[
					SNew(STextBlock).Text(CategoryHeaderText)
				]
			];
	}
	else
	{
		return SNew(STableRow<TSharedPtr<FName>>, TableViewBase)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.f, 0.f, 8.f, 0.f))
			.AutoWidth()
			[
				SNew(SImage)
				.Image(Item->SlateIcon.GetOptionalIcon())
				.ColorAndOpacity(Item->IconColor)
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(2.f)
			[
				SNew(STextBlock).Text(Item->Name)
			]
		];
	}
}

#undef LOCTEXT_NAMESPACE
