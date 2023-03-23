
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
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.Padding(8.f,4.f)
			.VAlign(VAlign_Center)
			[
				SAssignNew(HeaderText, STextBlock)
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Right)
			.Padding(0.f, 0.0f)
			[
				SNew(SButton)
				.ButtonStyle( &FAppStyle::Get().GetWidgetStyle< FButtonStyle >( "Button" ) )
				.OnClicked(this, &SHardReferenceViewerWindow::OnRefreshClicked)
				.ForegroundColor(FSlateColor::UseStyle())
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Padding(0.,3.f)
					.AutoWidth()
					[
						SNew(SImage)
						.ColorAndOpacity(FSlateColor::UseForeground())
						.Image(FAppStyle::GetBrush("Icons.Refresh"))
					]
					+ SHorizontalBox::Slot()
					.Padding(FMargin(8., 0, 0, 0))
					.VAlign(VAlign_Center)
					.AutoWidth()
					[
						SNew(STextBlock)
						.TextStyle(FAppStyle::Get(), "SmallButtonText")
						.Text(LOCTEXT("Refresh", "Refresh"))
					]
				]
			]
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

TSet<FName> SHardReferenceViewerWindow::GetCollapsedPackages() const
{
	TSet<FHRVTreeViewItemPtr> ExpandedItems;
	TreeView->GetExpandedItems(ExpandedItems);

	TSet<FName> CollapsedPackages;
	for(const FHRVTreeViewItemPtr Item : TreeViewData)
	{
		if(!ExpandedItems.Contains(Item))
		{
			CollapsedPackages.Add(Item->PackageId);
		}
	}
	return CollapsedPackages;
}

void SHardReferenceViewerWindow::InitiateSearch()
{
	if(TreeView.IsValid())
	{
		TSet<FName> UserCollapsedPackages = GetCollapsedPackages();

		TreeViewData = SearchData.GatherSearchData(BlueprintGraph);
		TreeView->RebuildList();

		// expand new items by default, unless they were intentionally collapsed.
		for(const FHRVTreeViewItemPtr Item : TreeViewData)
		{
			const bool bWasCollapsed = UserCollapsedPackages.Contains(Item->PackageId);
			const bool bShouldExpandItem = !bWasCollapsed;	
			TreeView->SetItemExpansion(Item, bShouldExpandItem);
		}
	}
	
	const FText SummaryText = FText::Format(LOCTEXT("SummaryMessage", "This blueprint makes {0} references to other packages."), SearchData.GetNumPackagesReferenced());
	HeaderText->SetText(SummaryText);
}

FReply SHardReferenceViewerWindow::OnRefreshClicked()
{
	InitiateSearch();
	return FReply::Handled();
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
