// Copyright Epic Games, Inc. All Rights Reserved.

#include "HardReferenceFinderStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FHardReferenceFinderStyle::StyleInstance = nullptr;

void FHardReferenceFinderStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FHardReferenceFinderStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FHardReferenceFinderStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("HardReferenceFinderStyle"));
	return StyleSetName;
}

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef< FSlateStyleSet > FHardReferenceFinderStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("HardReferenceFinderStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("HardReferenceFinder")->GetBaseDir() / TEXT("Resources"));


	return Style;
}

void FHardReferenceFinderStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FHardReferenceFinderStyle::Get()
{
	return *StyleInstance;
}
