#pragma once

#include "CoreMinimal.h"
#include "HardReferenceFinderSearchData.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/STreeView.h"

class FBlueprintEditor;

class SHardReferenceFinderWindow : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SHardReferenceFinderWindow) {};
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<FBlueprintEditor> InBlueprintGraph);

private:
	typedef STreeView<FHRFTreeViewItemPtr> SHRFTreeType;
	
	TSet<FName> GetCollapsedPackages() const;
	void InitiateSearch();
	FReply OnRefreshClicked();
	void OnDoubleClickTreeEntry(TSharedPtr<FHRFTreeViewItem> Item) const;
	void OnGetChildren(FHRFTreeViewItemPtr InItem, TArray< FHRFTreeViewItemPtr >& OutChildren) const;
	TSharedRef<ITableRow> OnGenerateRow(FHRFTreeViewItemPtr Item, const TSharedRef<STableViewBase>& TableViewBase) const;

	const FSlateBrush* GetBrush_MenuBackground() const;
	const FSlateBrush* GetBrush_RefreshIcon() const;
	const FTableRowStyle* GetStyle_HeaderRow() const;
	
	/* The graph this window is operating on */
	TWeakPtr<FBlueprintEditor> BlueprintGraph;
	
	/* Stores the data from searching the graph for references*/
	FHardReferenceFinderSearchData SearchData;

	/* Stores the list of items dispalyed by the tree view widget */
	TArray<TSharedPtr<FHRFTreeViewItem>> TreeViewData;

	/* Holds a reference to the header widget */
	TSharedPtr<STextBlock> HeaderText;

	/* Holds a reference to the tree view*/
	TSharedPtr<SHRFTreeType> TreeView;
};


