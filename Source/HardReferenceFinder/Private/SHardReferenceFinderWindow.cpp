
#include "SHardReferenceFinderWindow.h"
#include "BlueprintEditor.h"
#include "GraphEditorSettings.h"
#include "HardReferenceFinderSearchData.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"

#if ENGINE_MAJOR_VERSION < 5
#include "EditorStyleSet.h"
#endif

#define LOCTEXT_NAMESPACE "FHardReferenceFinderModule"

namespace HardReferenceInternals
{
	static FText MakeBestSizeString(const SIZE_T SizeInBytes)
	{
		FText SizeText;

		if (SizeInBytes < 1000)
		{
			// We ended up with bytes, so show a decimal number
			SizeText = FText::AsMemory(SizeInBytes, EMemoryUnitStandard::SI);
		}
		else
		{
			// Show a fractional number with the best possible units
			FNumberFormattingOptions NumberFormattingOptions;
			NumberFormattingOptions.MaximumFractionalDigits = 1;
			NumberFormattingOptions.MinimumFractionalDigits = 0;
			NumberFormattingOptions.MinimumIntegralDigits = 1;

			SizeText = FText::AsMemory(SizeInBytes, &NumberFormattingOptions, nullptr, EMemoryUnitStandard::SI);
		}

		return SizeText;
	}	
}

void SHardReferenceFinderWindow::Construct(const FArguments& InArgs, TSharedPtr<FBlueprintEditor> InBlueprintGraph)
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
				.OnClicked(this, &SHardReferenceFinderWindow::OnRefreshClicked)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Padding(0.,3.f)
					.AutoWidth()
					[
						SNew(SImage)
						.Image(GetBrush_RefreshIcon())
					]
					+ SHorizontalBox::Slot()
					.Padding(FMargin(8., 0, 0, 0))
					.VAlign(VAlign_Center)
					.AutoWidth()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Refresh", "Refresh"))
					]
				]
			]
		]
		+ SVerticalBox::Slot()
		[
			SNew(SBorder)
			.BorderImage(GetBrush_MenuBackground())
			.Padding(FMargin(8.f, 8.f, 4.f, 0.f))
			[
				SAssignNew(TreeView, SHRFTreeType)
				.TreeItemsSource(&TreeViewData)
				.OnGetChildren(this, &SHardReferenceFinderWindow::OnGetChildren)
				.OnGenerateRow(this, &SHardReferenceFinderWindow::OnGenerateRow)
				.OnMouseButtonDoubleClick(this, &SHardReferenceFinderWindow::OnDoubleClickTreeEntry)
			]
		]
	];

	InitiateSearch();
}

TSet<FName> SHardReferenceFinderWindow::GetCollapsedPackages() const
{
	TSet<FHRFTreeViewItemPtr> ExpandedItems;
	TreeView->GetExpandedItems(ExpandedItems);

	TSet<FName> CollapsedPackages;
	for(const FHRFTreeViewItemPtr Item : TreeViewData)
	{
		if(!ExpandedItems.Contains(Item))
		{
			CollapsedPackages.Add(Item->PackageId);
		}
	}
	return CollapsedPackages;
}

void SHardReferenceFinderWindow::InitiateSearch()
{
	if(TreeView.IsValid())
	{
		TSet<FName> UserCollapsedPackages = GetCollapsedPackages();

		TreeViewData = SearchData.GatherSearchData(BlueprintGraph);
		TreeView->RebuildList();

		// expand new items by default, unless they were intentionally collapsed.
		for(const FHRFTreeViewItemPtr Item : TreeViewData)
		{
			const bool bWasCollapsed = UserCollapsedPackages.Contains(Item->PackageId);
			const bool bShouldExpandItem = !bWasCollapsed;	
			TreeView->SetItemExpansion(Item, bShouldExpandItem);
		}
	}
	
	const FText SummaryText = FText::Format(LOCTEXT("SummaryMessage", "This blueprint makes {0} references to other packages."), SearchData.GetNumPackagesReferenced());
	HeaderText->SetText(SummaryText);
}

FReply SHardReferenceFinderWindow::OnRefreshClicked()
{
	InitiateSearch();
	return FReply::Handled();
}

void SHardReferenceFinderWindow::OnDoubleClickTreeEntry(TSharedPtr<FHRFTreeViewItem> Item) const
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

void SHardReferenceFinderWindow::OnGetChildren(FHRFTreeViewItemPtr InItem, TArray<FHRFTreeViewItemPtr>& OutChildren) const
{
	OutChildren += InItem->Children;
}

TSharedRef<ITableRow> SHardReferenceFinderWindow::OnGenerateRow(FHRFTreeViewItemPtr Item, const TSharedRef<STableViewBase>& TableViewBase) const
{
	if(Item->bIsHeader)
	{
		const FText SizeText = HardReferenceInternals::MakeBestSizeString(Item->SizeOnDisk);
		const FText CategoryHeaderText = FText::Format(LOCTEXT("CategoryHeader", "{1} ({0})"), SizeText, Item->Name);

		return SNew(STableRow<TSharedPtr<FName>>, TableViewBase)
			.Style( GetStyle_HeaderRow() )
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

const FSlateBrush* SHardReferenceFinderWindow::GetBrush_MenuBackground() const
{
#if ENGINE_MAJOR_VERSION < 5
	return FEditorStyle::GetBrush("Menu.Background");
#else
	return FAppStyle::Get().GetBrush("Brushes.Recessed");
#endif
}

const FSlateBrush* SHardReferenceFinderWindow::GetBrush_RefreshIcon() const
{
#if ENGINE_MAJOR_VERSION < 5
	return FEditorStyle::GetBrush("Icons.Refresh");
#else
	return FAppStyle::GetBrush("Icons.Refresh");
#endif
}

const FTableRowStyle* SHardReferenceFinderWindow::GetStyle_HeaderRow() const
{
#if ENGINE_MAJOR_VERSION < 5
	return &FCoreStyle::Get().GetWidgetStyle<FTableRowStyle>("TableView.Row");
#else
	return &FAppStyle::Get().GetWidgetStyle<FTableRowStyle>("ShowParentsTableView.Row");
#endif
}

#undef LOCTEXT_NAMESPACE
