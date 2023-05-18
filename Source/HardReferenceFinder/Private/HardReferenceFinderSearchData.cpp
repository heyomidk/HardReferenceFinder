#include "HardReferenceFinderSearchData.h"
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "BlueprintEditor.h"
#include "EdGraph/EdGraph.h"
#include "K2Node_CallFunction.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_FunctionEntry.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Misc/EngineVersionComparison.h"
#include "Styling/SlateIconFinder.h"

#if ENGINE_MAJOR_VERSION < 5
#include "SSCSEditor.h"
#else
#include "SSubobjectEditor.h"
#endif

#define LOCTEXT_NAMESPACE "FHardReferenceFinderModule"

TArray<FHRFTreeViewItemPtr> FHardReferenceFinderSearchData::GatherSearchData(TWeakPtr<FBlueprintEditor> BlueprintEditor)
{
	Reset();

	TMap<FName, FHRFTreeViewItemPtr> DependentPackageMap;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	
	// Get this blueprints package dependencies from the blueprint editor 
	TArray<FName> BlueprintDependencies;
	GetBlueprintDependencies(BlueprintDependencies, AssetRegistryModule, BlueprintEditor);
	
	// Populate display information from package dependencies
	{
		TMap<FName, FAssetData> DependencyToAssetDataMap;
		GetAssetForPackages(BlueprintDependencies, DependencyToAssetDataMap);

		for (auto MapIt = DependencyToAssetDataMap.CreateConstIterator(); MapIt; ++MapIt)
		{
			const FName& PathName = MapIt.Key();
			const FAssetData& AssetData = MapIt.Value();
			FString AssetTypeName = GetAssetTypeName(AssetData);
			FString FileName = FPaths::GetCleanFilename(PathName.ToString());
		
			FAssetPackageData AssetPackageData;
			TryGetAssetPackageData(PathName, AssetPackageData, AssetRegistryModule);

			if( FHRFTreeViewItemPtr Header = MakeShared<FHRFTreeViewItem>() )
			{
				Header->bIsHeader = true;
				Header->PackageId = PathName;
				Header->Tooltip = FText::FromName(PathName);
				Header->Name = FText::FromString(FileName);
				Header->SizeOnDisk = GatherAssetSizeByName(PathName, AssetRegistryModule);
				Header->SlateIcon = FSlateIcon("EditorStyle", FName( *("ClassIcon." + AssetTypeName))); 

				FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));	
				if (UClass* AssetClass = AssetData.GetClass())
				{
					TWeakPtr<IAssetTypeActions> AssetTypeActions = AssetToolsModule.Get().GetAssetTypeActionsForClass(AssetData.GetClass());
					if(AssetTypeActions.IsValid())
					{
						Header->IconColor = AssetTypeActions.Pin()->GetTypeColor();
					}
				}
			
				DependentPackageMap.Add(PathName, Header);
				TreeView.Add(Header);
			}
		}
	}
	
	{
		// Search through blueprint nodes for references to the dependent packages
		if( UBlueprint* Blueprint = BlueprintEditor.Pin()->GetBlueprintObj() )
		{
			SearchGraphNodes(DependentPackageMap, AssetRegistryModule, Blueprint->UbergraphPages);
			SearchFunctionReferences(DependentPackageMap, AssetRegistryModule, Blueprint);
			SearchBlueprintClassProperties(DependentPackageMap, AssetRegistryModule, Blueprint);
			SearchSimpleConstructionScript(DependentPackageMap, AssetRegistryModule, Blueprint);
		}
	}

	// If we didn't discover any references to a package make a note
	for(FHRFTreeViewItemPtr HeaderItem : TreeView)
	{
		if(HeaderItem->Children.Num() <= 0)
		{
			FHRFTreeViewItemPtr ChildItem = MakeShared<FHRFTreeViewItem>();
			HeaderItem->Children.Add(ChildItem);
			ChildItem->Name = LOCTEXT("UnknownSource", "Unidentified source");
			ChildItem->Tooltip = LOCTEXT("UnknownSourceTooltip", "This package is being referenced but the plugin is unable to identify its source.");
		}
	}
	
	// sort from largest to smallest
	TreeView.Sort([](FHRFTreeViewItemPtr Lhs, FHRFTreeViewItemPtr Rhs)
	{
		return Lhs->SizeOnDisk > Rhs->SizeOnDisk;
	});
	
	return TreeView;
}

void FHardReferenceFinderSearchData::Reset()
{
	TreeView.Reset();
}

UObject* FHardReferenceFinderSearchData::GetObjectContext(TWeakPtr<FBlueprintEditor> BlueprintEditor) const
{
	if(!BlueprintEditor.IsValid())
	{
		return nullptr;
	}

#if ENGINE_MAJOR_VERSION < 5
	TSharedPtr<class SSCSEditor> SCSEditorPtr = BlueprintEditor.Pin()->GetSCSEditor();
	if(!SCSEditorPtr.IsValid())
	{
		return nullptr;
	}
	return SCSEditorPtr->GetActorContext();
#else
	TSharedPtr<SSubobjectEditor> SubobjectEditorPtr = BlueprintEditor.Pin()->GetSubobjectEditor();
	SSubobjectEditor* SubobjectEditorWidget = SubobjectEditorPtr.Get();
	if(SubobjectEditorWidget == nullptr)
	{
		class BlueprintEditorEditingObject_AccessHack : public FBlueprintEditor
		{
		public:
			UObject* GetEditingObject_Expose() const { return GetEditingObject(); }
		};
		return static_cast<BlueprintEditorEditingObject_AccessHack*>(BlueprintEditor.Pin().Get())->GetEditingObject_Expose();
	}
		
	UObject* Object = SubobjectEditorWidget->GetObjectContext();
	return Object;
#endif
}

void FHardReferenceFinderSearchData::GetBlueprintDependencies(TArray<FName>& OutPackageDependencies, FAssetRegistryModule& AssetRegistryModule, TWeakPtr<FBlueprintEditor> BlueprintEditor) const
{
	UObject* Object = GetObjectContext(BlueprintEditor);
	if(Object == nullptr)
	{
		return;
	}
	
	FAssetData ExistingAsset = GetAssetDataForObject(Object);

	UE::AssetRegistry::FDependencyQuery Flags(UE::AssetRegistry::EDependencyQuery::Hard);
	AssetRegistryModule.GetDependencies(ExistingAsset.PackageName, OutPackageDependencies, UE::AssetRegistry::EDependencyCategory::Package, Flags);
}

void FHardReferenceFinderSearchData::SearchGraphNodes(TMap<FName, FHRFTreeViewItemPtr>& OutPackageMap, const FAssetRegistryModule& AssetRegistryModule, const FEdGraphArray& EdGraphList) const
{
	for(UEdGraph* Graph : EdGraphList)
	{
		if(Graph)
		{
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				const UPackage* FunctionPackage = nullptr;
				if(const UK2Node_CallFunction* CallFunctionNode = Cast<UK2Node_CallFunction>(Node))
				{
					FunctionPackage = CallFunctionNode->FunctionReference.GetMemberParentPackage();
				}
				else if(const UK2Node_DynamicCast* CastNode = Cast<UK2Node_DynamicCast>(Node))
				{
					if(CastNode->TargetType)
					{
						FunctionPackage = CastNode->TargetType->GetPackage();
					}
				}

				if( const FHRFTreeViewItemPtr Result = CheckAddPackageResult(OutPackageMap, AssetRegistryModule, FunctionPackage) )
				{
					Result->Name = Node->GetNodeTitle(ENodeTitleType::ListView);
					Result->NodeGuid = Node->NodeGuid;
					Result->SlateIcon = Node->GetIconAndTint(Result->IconColor);
				}

				// Also search the pins of this node for any references to other packages, e.g. the 'Class' pin of a SpawnActor node.
				SearchNodePins(OutPackageMap, AssetRegistryModule, Node);
			}
		}
	}
}

void FHardReferenceFinderSearchData::SearchNodePins(TMap<FName, FHRFTreeViewItemPtr>& OutPackageMap, const FAssetRegistryModule& AssetRegistryModule, const UEdGraphNode* Node) const
{
	if(Node == nullptr)
	{
		return;
	}
	
	for(const UEdGraphPin* Pin : Node->Pins)
	{
		if(Pin->bHidden)
		{
			// skip hidden pins
			continue;
		}
					
		if(Pin->Direction == EGPD_Input)
		{
			if(const UObject* PinObject = Pin->DefaultObject)
			{
				const UPackage* FunctionPackage = PinObject->GetPackage();
				if( const FHRFTreeViewItemPtr Result = CheckAddPackageResult(OutPackageMap, AssetRegistryModule, FunctionPackage) )
				{
					Result->Name = FText::Format(LOCTEXT("FunctionInput","{0} ({1})"), FText::FromString(Pin->GetName()), Node->GetNodeTitle(ENodeTitleType::ListView));
					Result->NodeGuid = Node->NodeGuid;
					if( const UEdGraphSchema* Schema = Pin->GetSchema() )
					{
						Result->IconColor = Schema->GetPinTypeColor(Pin->PinType);
					}
					Result->SlateIcon = FSlateIcon("EditorStyle", "Graph.Pin.Disconnected_VarA");
				}
			}
		}
	}
}

void FHardReferenceFinderSearchData::SearchFunctionReferences(TMap<FName, FHRFTreeViewItemPtr>& OutPackageMap, const FAssetRegistryModule& AssetRegistryModule, const UBlueprint* Blueprint) const
{
	// @heyomidk This method is still imperfect as there are a number of ways references can be formed from functions
	//		1. From graph nodes inside the function (Cast, etc)
	//		2. From Local Variables inside the function
	//			a. Due to the variables type (ex: class is Blueprint)
	//			b. Due to the default value (ie Subclass of UObject but points to a Blueprint)
	//		3. From arguments to a function
	//			a. Due to the type of an input argument
	//			b. Due to the type of an output argument
	//			c. Due to the default value of an output argument
	//
	// Note that the UFunction* provided by a TFieldRange iterator accounts for reference types 2a/2b, while the cached
	// UFunction* in an EdGraphNode finds 2a but not 2b
	//
	// Note that neither method identifies all type 3 references so this approach may need to be amended/revised

	// This gathers type 1/3c references and creates links to the associated graph nodes.
	SearchGraphNodes(OutPackageMap, AssetRegistryModule, Blueprint->FunctionGraphs);

	// This gathers type 2 references and links them to the function entry node
	for( UFunction* Function : TFieldRange<UFunction>(Blueprint->GeneratedClass, EFieldIteratorFlags::ExcludeSuper) )
	{
		const UK2Node_FunctionEntry* GraphEntryNode = FindGraphNodeForFunction(Blueprint, Function);
		if(GraphEntryNode)
		{
			for( const UObject* ReferencedObject : Function->ScriptAndPropertyObjectReferences)
			{
				const UPackage* Package = ReferencedObject->GetPackage();
				if( const FHRFTreeViewItemPtr Result = CheckAddPackageResult(OutPackageMap, AssetRegistryModule, Package) )
				{
					Result->Name = FText::Format(LOCTEXT("FunctionReference","{0}"), GraphEntryNode->GetNodeTitle(ENodeTitleType::ListView));
					Result->NodeGuid = GraphEntryNode->NodeGuid;
					Result->SlateIcon = GraphEntryNode->GetIconAndTint(Result->IconColor);
				}
			}
		}
	}
}

void FHardReferenceFinderSearchData::SearchSimpleConstructionScript(TMap<FName, FHRFTreeViewItemPtr>& OutPackageMap, const FAssetRegistryModule& AssetRegistryModule, const UBlueprint* Blueprint) const
{
	const USimpleConstructionScript* SimpleConstructionScript = Blueprint->SimpleConstructionScript;
	if(SimpleConstructionScript == nullptr)
	{
		return;
	}
	
	const TArray<USCS_Node*>& RootNodes = SimpleConstructionScript->GetAllNodes();
	for(const USCS_Node* SCSNode : RootNodes)
	{
		if(SCSNode == nullptr)
		{
			continue;
		}

		const FName VarName = SCSNode->GetVariableName();
		
		TSet<UPackage*> OutReferencedPackages;
		FindPackagesInSCSNode(OutReferencedPackages, SCSNode);

		for(const UPackage* Package : OutReferencedPackages)
		{
			if(const FHRFTreeViewItemPtr Result = CheckAddPackageResult(OutPackageMap, AssetRegistryModule, Package))
			{
				Result->SlateIcon = FSlateIconFinder::FindIconForClass(SCSNode->ComponentClass, TEXT("SCS.Component"));
				Result->Name = FText::Format(LOCTEXT("ComponentReference", "{0}"), FText::FromName(VarName));
				Result->SCSIdentifier = SCSNode->GetFName();
			}
		}
	}
}

UK2Node_FunctionEntry* FHardReferenceFinderSearchData::FindGraphNodeForFunction(const UBlueprint* Blueprint, UFunction* FunctionToFind) const
{
	if(Blueprint == nullptr)
	{
		return nullptr;
	}
	
	if(FunctionToFind == nullptr)
	{
		return nullptr;
	}

	// search functions in the Graph
	for(UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		// find the entry point for the function in the graph
		UK2Node_FunctionEntry* GraphEntryNode = nullptr;
		for(UEdGraphNode* Node : Graph->Nodes)
		{
			GraphEntryNode = Cast<UK2Node_FunctionEntry>(Node);
			if(GraphEntryNode != nullptr)
			{
				break;
			}
		}
					
		if(GraphEntryNode)
		{
			// See if this matches the provided UFunction
			const TSharedPtr<FStructOnScope> NodeFunctionVarCache = GraphEntryNode->GetFunctionVariableCache();
			if( NodeFunctionVarCache.IsValid() )
			{
				if( const UFunction* NodeFunction = Cast<UFunction>(NodeFunctionVarCache->GetStruct()) )
				{
					const FName NodeFunctionName = NodeFunction->GetFName(); 
					const FName TargetFunctionName = FunctionToFind->GetFName();
					if(NodeFunctionName == TargetFunctionName)
					{
						return GraphEntryNode;
					}							
				}
			}
		}
	}
	
	return nullptr;
}

void FHardReferenceFinderSearchData::FindPackagesInSCSNode(TSet<UPackage*>& OutReferencedPackages, const USCS_Node* SCSNode) const
{
	if(SCSNode==nullptr || SCSNode->ComponentClass == nullptr)
	{
		return;
	}

	if( UPackage* NodePackage = SCSNode->ComponentClass->GetPackage() )
	{
		if(NodePackage)
		{
			OutReferencedPackages.Add(NodePackage);
		}
	}
	
	for( const FProperty* Property : TFieldRange<FProperty>(SCSNode->ComponentClass, EFieldIteratorFlags::IncludeSuper))
	{
		FSlateIcon VariableTypeIcon;
		TArray<UPackage*> ReferencedPackages = FindPackagesForProperty(VariableTypeIcon, SCSNode->ComponentTemplate, Property );
		for(UPackage* Package : ReferencedPackages)
		{
			OutReferencedPackages.Add(Package);
		}
	}
}

TArray<UPackage*> FHardReferenceFinderSearchData::FindPackagesForProperty(FSlateIcon& OutResultIcon, const UObject* ContainerPtr, const FProperty* TargetProperty) const
{
	TArray<UPackage*> FoundPackages;

	if(TargetProperty == nullptr)
	{
		return FoundPackages;
	}
	
	if(ContainerPtr != nullptr)
	{
		TArray<const FStructProperty*> EncounteredStructProps;
		const EPropertyObjectReferenceType ReferenceType = EPropertyObjectReferenceType::Strong;
		const bool bHasStrongReferences = TargetProperty->ContainsObjectReference(EncounteredStructProps, ReferenceType);
		if(bHasStrongReferences)
		{
			const void* TargetPropertyAddress = TargetProperty->ContainerPtrToValuePtr<void>(ContainerPtr);

			struct PropertyAndAddressTuple
			{
				const FProperty* Property = nullptr;
				const void* ValueAddress = nullptr;
			};
			TArray<PropertyAndAddressTuple> PropertiesToExamine;

			if(const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(TargetProperty))
			{
				OutResultIcon = FSlateIcon("EditorStyle", "Kismet.VariableList.ArrayTypeIcon");
				FScriptArrayHelper ArrayHelper(ArrayProperty, TargetPropertyAddress);
				for(int i=0; i<ArrayHelper.Num(); ++i)
				{
					PropertiesToExamine.Add({ArrayProperty->Inner, ArrayHelper.GetRawPtr(i)});
				}
			}
			else if(const FSetProperty* SetProperty = CastField<FSetProperty>(TargetProperty))
			{
				OutResultIcon = FSlateIcon("EditorStyle", "Kismet.VariableList.SetTypeIcon");
				FScriptSetHelper SetHelper(SetProperty, TargetPropertyAddress);
				for(int i=0; i<SetHelper.Num(); ++i)
				{
					PropertiesToExamine.Add({SetHelper.GetElementProperty(), SetHelper.GetElementPtr(i)});
				}
			}
			else if(const FMapProperty* MapProperty = CastField<FMapProperty>(TargetProperty))
			{
				OutResultIcon = FSlateIcon("EditorStyle", "Kismet.VariableList.MapValueTypeIcon");
				FScriptMapHelper MapHelper(MapProperty, TargetPropertyAddress);
				for(int i=0; i<MapHelper.Num(); ++i)
				{
					PropertiesToExamine.Add({MapHelper.GetKeyProperty(), MapHelper.GetKeyPtr(i)});
					PropertiesToExamine.Add({MapHelper.GetValueProperty(), MapHelper.GetValuePtr(i)});
				}
			}
			else
			{
				OutResultIcon = FSlateIcon("EditorStyle", "Kismet.VariableList.TypeIcon");
				PropertiesToExamine.Add({TargetProperty, TargetPropertyAddress});
			}

			for (const PropertyAndAddressTuple& Tuple : PropertiesToExamine)
			{
				if( const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Tuple.Property) )
				{
					if(const UObject* Object = ObjectProperty->GetObjectPropertyValue(Tuple.ValueAddress))
					{
						if(UPackage* Package = Object->GetPackage())
						{
							FoundPackages.AddUnique(Package);
						}
					}
				}
			}
		}
	}
	
	const TArray<UObject*>* ScriptAndPropertyObjectReferences = nullptr;
	if(const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(TargetProperty))
	{
		ScriptAndPropertyObjectReferences = &ObjectProperty->PropertyClass->ScriptAndPropertyObjectReferences;
		if(ObjectProperty->PropertyClass)
		{
			if(UPackage* Package = ObjectProperty->PropertyClass->GetPackage())
			{
				FoundPackages.AddUnique(Package);
			}
		}
	}
	else if(const FStructProperty* StructProperty = CastField<FStructProperty>(TargetProperty))
	{
		ScriptAndPropertyObjectReferences = &StructProperty->Struct->ScriptAndPropertyObjectReferences;
	}

	if(ScriptAndPropertyObjectReferences)
	{
		for(const UObject* ObjectReference : *ScriptAndPropertyObjectReferences)
		{
			if(ObjectReference)
			{
				if(UPackage* Package = ObjectReference->GetPackage())
				{
					FoundPackages.AddUnique(Package);
				}
			}
		}
	}
	
	return FoundPackages;
}

void FHardReferenceFinderSearchData::SearchBlueprintClassProperties(TMap<FName, FHRFTreeViewItemPtr>& OutPackageMap,	const FAssetRegistryModule& AssetRegistryModule, UBlueprint* Blueprint) const
{
	for( FProperty* Property : TFieldRange<FProperty>(Blueprint->GeneratedClass, EFieldIteratorFlags::ExcludeSuper))
	{
		UBlueprint* FoundBlueprint = nullptr;
		const FName VarName = Property->GetFName();
		const int32 VarIndex = FBlueprintEditorUtils::FindNewVariableIndexAndBlueprint(Blueprint, VarName, FoundBlueprint);
		const bool bIsVar = VarIndex != INDEX_NONE;
		
		bool bSkipProperty = false;
		if(!bIsVar)
		{
			if(const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
			{
				const bool bIsActorComponentClass = ObjectProperty->PropertyClass->IsChildOf(UActorComponent::StaticClass());
				if(bIsActorComponentClass)
				{
					// Actor components aren't initialized in the CDO so parsing them here isn't exhaustive.
					// SearchSimpleConstructionScript() handles these, so skip these properties here. 
					bSkipProperty = true;
				}
			}
		}

		if(!bSkipProperty)
		{
			FSlateIcon ResultIcon;
			TArray<UPackage*> ReferencedPackages = FindPackagesForProperty(ResultIcon, Blueprint->GeneratedClass->GetDefaultObject(), Property); 

			for(const UPackage* Package : ReferencedPackages)
			{
				if(const FHRFTreeViewItemPtr Result = CheckAddPackageResult(OutPackageMap, AssetRegistryModule, Package))
				{
					if(bIsVar)
					{
						const FBPVariableDescription& Description = Blueprint->NewVariables[VarIndex];
						if( UEdGraphSchema_K2 const* Schema = GetDefault<UEdGraphSchema_K2>() )
						{
							Result->IconColor = Schema->GetPinTypeColor(Description.VarType);
						}
						Result->SlateIcon = ResultIcon;
						Result->Name = FText::Format(LOCTEXT("MemberVariable","{0} (Member Variable)"), FText::FromName(Description.VarName));
						Result->Tooltip = LOCTEXT("MemberVariableTooltip","Blueprint member variable");
					}
					else
					{
						Result->SlateIcon = ResultIcon;
						Result->Name = FText::Format(LOCTEXT("OtherPropertyName", "{0}"), FText::FromName(VarName));
					}
				}
			}
		}
	}
}

FHRFTreeViewItemPtr FHardReferenceFinderSearchData::CheckAddPackageResult(TMap<FName, FHRFTreeViewItemPtr>& OutPackageMap, const FAssetRegistryModule& AssetRegistryModule, const UPackage* Package) const
{
	if( Package )
	{
		const FName PackageName = Package->GetFName();
		if(const FHRFTreeViewItemPtr* FoundHeader = OutPackageMap.Find(PackageName))
		{
			const FHRFTreeViewItemPtr Header = *FoundHeader;
								
			FAssetPackageData AssetPackageData;
			const bool bExists = TryGetAssetPackageData(PackageName, AssetPackageData, AssetRegistryModule);
			if(ensure(bExists))
			{
				FHRFTreeViewItemPtr Link = MakeShared<FHRFTreeViewItem>();
				Header->Children.Add(Link);
				return Link; 
			}
		}
	}

	return  nullptr;
}

void FHardReferenceFinderSearchData::GetAssetForPackages(const TArray<FName>& PackageNames, TMap<FName, FAssetData>& OutPackageToAssetData) const
{
#if ENGINE_MAJOR_VERSION < 5
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	FARFilter Filter;
	for ( auto PackageIt = PackageNames.CreateConstIterator(); PackageIt; ++PackageIt )
	{
		const FString& PackageName = (*PackageIt).ToString();
		Filter.PackageNames.Add(*PackageIt);
	}

	TArray<FAssetData> AssetDataList;
	AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);
	for ( auto AssetIt = AssetDataList.CreateConstIterator(); AssetIt; ++AssetIt )
	{
		OutPackageToAssetData.Add((*AssetIt).PackageName, *AssetIt);
	}
#else
	UE::AssetRegistry::GetAssetForPackages(PackageNames, OutPackageToAssetData);
#endif
}

bool FHardReferenceFinderSearchData::TryGetAssetPackageData(FName PathName, FAssetPackageData& OutPackageData, const FAssetRegistryModule& AssetRegistryModule) const
{
#if UE_VERSION_OLDER_THAN(5, 0, 0)
	if(const FAssetPackageData* pAssetPackageData = AssetRegistryModule.Get().GetAssetPackageData(PathName))
	{
		OutPackageData = *pAssetPackageData;
		return true;
	}
#elif UE_VERSION_OLDER_THAN(5, 1, 0)
	const TOptional<FAssetPackageData> pAssetPackageData = AssetRegistryModule.Get().GetAssetPackageDataCopy(PathName);
	if (pAssetPackageData.IsSet())
	{
		OutPackageData = pAssetPackageData.GetValue();
		return true;
	}
#else
	const UE::AssetRegistry::EExists Result = AssetRegistryModule.TryGetAssetPackageData(PathName, OutPackageData);
	if(Result == UE::AssetRegistry::EExists::Exists)
	{
		return true;
	}
#endif
	return false;
}

FString FHardReferenceFinderSearchData::GetAssetTypeName(const FAssetData& AssetData) const
{
#if UE_VERSION_OLDER_THAN(5, 1, 0)
	return AssetData.AssetClass.ToString();
#else
	return AssetData.AssetClassPath.GetAssetName().ToString();
#endif
}

FAssetData FHardReferenceFinderSearchData::GetAssetDataForObject(const UObject* Object) const
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	FString ObjectPath = Object->GetPathName();

#if UE_VERSION_OLDER_THAN(5, 1, 0)
	return AssetRegistryModule.Get().GetAssetByObjectPath(*ObjectPath);
#else
	return AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(ObjectPath));
#endif
}

int64 FHardReferenceFinderSearchData::GatherAssetSizeByName(const FName& AssetName, FAssetRegistryModule& AssetRegistryModule) const
{
	TSet<FName> Visited;
	const int64 Size = GatherAssetSizeRecursive(AssetName, Visited, AssetRegistryModule);
	return Size;
}

int64 FHardReferenceFinderSearchData::GatherAssetSizeRecursive(const FName& AssetName, TSet<FName>& OutVisited, FAssetRegistryModule& AssetRegistryModule) const
{
	const bool bAlreadyVisited = OutVisited.Contains(AssetName);
	if(bAlreadyVisited)
	{
		return 0;
	}
	OutVisited.Add(AssetName);

	int64 AssetSize = 0;
	FAssetPackageData AssetPackageData;
	if( TryGetAssetPackageData(AssetName, AssetPackageData, AssetRegistryModule) )
	{
		AssetSize += AssetPackageData.DiskSize;
	}
	
	TArray<FName> Dependencies;
	const UE::AssetRegistry::FDependencyQuery Flags(UE::AssetRegistry::EDependencyQuery::Hard);
	AssetRegistryModule.GetDependencies(AssetName, Dependencies, UE::AssetRegistry::EDependencyCategory::Package, Flags);
	int64 DependencySize = 0;	
	for(const FName& DependencyName : Dependencies)
	{
		DependencySize += GatherAssetSizeRecursive(DependencyName, OutVisited, AssetRegistryModule);  
	}

	const int64 TotalSize = AssetSize + DependencySize; 
	return TotalSize;
}

#undef LOCTEXT_NAMESPACE
