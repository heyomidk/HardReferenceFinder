// Copyright Epic Games, Inc. All Rights Reserved.

#include "HardReferenceViewerStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FHardReferenceViewerStyle::StyleInstance = nullptr;

void FHardReferenceViewerStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FHardReferenceViewerStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FHardReferenceViewerStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("HardReferenceViewerStyle"));
	return StyleSetName;
}

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef< FSlateStyleSet > FHardReferenceViewerStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("HardReferenceViewerStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("HardReferenceViewer")->GetBaseDir() / TEXT("Resources"));

	Style->Set("HardReferenceViewer.TabIcon", new IMAGE_BRUSH_SVG(TEXT("PlaceholderButtonIcon"), Icon20x20));

	return Style;
}

void FHardReferenceViewerStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FHardReferenceViewerStyle::Get()
{
	return *StyleInstance;
}
