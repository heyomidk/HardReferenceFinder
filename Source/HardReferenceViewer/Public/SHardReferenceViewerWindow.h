#pragma once

#include "CoreMinimal.h"
#include "HardReferenceViewerSearchData.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/STreeView.h"

class FBlueprintEditor;

class SHardReferenceViewerWindow : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SHardReferenceViewerWindow) {};
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<FBlueprintEditor> InBlueprintGraph);

private:
	typedef STreeView<FHRVTreeViewItemPtr> SHRVReferenceTreeType;
	
	TSet<FName> GetCollapsedPackages() const;
	void InitiateSearch();
	FReply OnRefreshClicked();
	void OnDoubleClickTreeEntry(TSharedPtr<FHRVTreeViewItem> Item) const;
	void OnGetChildren(FHRVTreeViewItemPtr InItem, TArray< FHRVTreeViewItemPtr >& OutChildren) const;
	TSharedRef<ITableRow> OnGenerateRow(FHRVTreeViewItemPtr Item, const TSharedRef<STableViewBase>& TableViewBase) const;
	
	/* The graph this window is operating on */
	TWeakPtr<FBlueprintEditor> BlueprintGraph;
	
	/* Stores the data from searching the graph for references*/
	FHardReferenceViewerSearchData SearchData;

	/* Stores the list of items dispalyed by the tree view widget */
	TArray<TSharedPtr<FHRVTreeViewItem>> TreeViewData;

	/* Holds a reference to the header widget */
	TSharedPtr<STextBlock> HeaderText;

	/* Holds a reference to the tree view*/
	TSharedPtr<SHRVReferenceTreeType> TreeView;
};


