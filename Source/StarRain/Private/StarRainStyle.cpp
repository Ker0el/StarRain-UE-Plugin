// Copyright Saeid Gholizade. All Rights Reserved. 2020
// ---------------------------------------------------------------------------
// 本文件是 Physical Layout Tool 的修改版本。原作品作者 Saeid Gholizade，
// 采用 CC BY 4.0 许可 (https://creativecommons.org/licenses/by/4.0/)。
// 修改者：星空 (https://space.bilibili.com/177308205)
// 修改内容与完整署名见插件根目录的 NOTICE.txt
// ---------------------------------------------------------------------------


#include "StarRainStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"

TSharedPtr< FSlateStyleSet > FStarRainStyle::StyleInstance = NULL;

void FStarRainStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FStarRainStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FStarRainStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("StarRainStyle"));
	return StyleSetName;
}

#define PLUGIN_IMAGE_BRUSH( Path, ... ) FSlateImageBrush( IPluginManager::Get().FindPlugin("StarRain")->GetBaseDir()  + "/" + Path + ".png", __VA_ARGS__ )
#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);
const FVector2D Icon40x40(40.0f, 40.0f);

TSharedRef< FSlateStyleSet > FStarRainStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("StarRainStyle"));

	Style->SetContentRoot(FPaths::EngineContentDir() / TEXT("Editor/Slate"));

	Style->Set("StarRain.SelectCommand", new PLUGIN_IMAGE_BRUSH(TEXT("/Resources/select"), Icon40x40));
	Style->Set("StarRain.PaintSelectCommand", new PLUGIN_IMAGE_BRUSH(TEXT("/Resources/paintselect"), Icon40x40));
	Style->Set("StarRain.TransformationCommand", new PLUGIN_IMAGE_BRUSH(TEXT("/Resources/icon128"), Icon40x40));
	Style->Set("StarRain.PaintCommand", new PLUGIN_IMAGE_BRUSH(TEXT("/Resources/paint"), Icon40x40));
	Style->Set("StarRain.Delete", new IMAGE_BRUSH(TEXT("icons/icon_Delete_40x"), Icon40x40));

	Style->Set("Save", FButtonStyle()
			.SetNormal(PLUGIN_IMAGE_BRUSH("/Resources/icon_file_save_40x", Icon20x20))
			.SetPressed(PLUGIN_IMAGE_BRUSH("/Resources/icon_file_save_clicked_40x", Icon20x20))
			.SetHovered(PLUGIN_IMAGE_BRUSH("/Resources/icon_file_save_hover_40x", Icon20x20)));
	
	Style->Set("Delete", FButtonStyle()
		.SetNormal(IMAGE_BRUSH("icons/icon_Delete_40x", Icon20x20))
		.SetPressed(IMAGE_BRUSH("icons/icon_Delete_40x", Icon20x20))
		.SetHovered(IMAGE_BRUSH("icons/icon_Delete_40x", Icon20x20)));
	
	Style->Set("Select", FButtonStyle()
		.SetNormal(PLUGIN_IMAGE_BRUSH("/Resources/selectActors", Icon20x20))
		.SetPressed(PLUGIN_IMAGE_BRUSH("/Resources/selectActors", Icon20x20))
		.SetHovered(PLUGIN_IMAGE_BRUSH("/Resources/selectActors", Icon20x20)));
	
	// 模式图标：星雨（流星）
	// 注意 FSlateIcon(样式集, 小图标名, 大图标名) —— 模式下拉框取的是「小图标」。
	// 原版 FSlateIcon 里小图标名写的是 "StarRain"，但这里只注册了 "StarRain.Icon"，
	// 名字对不上，所以模式前面一直是空白没有图标。现在把三个尺寸都注册齐。
	Style->Set("StarRain",       new PLUGIN_IMAGE_BRUSH(TEXT("/Resources/icon_starrain_16x"), Icon16x16));
	Style->Set("StarRain.Small", new PLUGIN_IMAGE_BRUSH(TEXT("/Resources/icon_starrain_20x"), Icon20x20));
	Style->Set("StarRain.Icon",  new PLUGIN_IMAGE_BRUSH(TEXT("/Resources/icon_starrain_40x"), Icon40x40));
	
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("StarRain")->GetBaseDir() / TEXT("Resources"));

	Style->Set("ClassIcon.StarRainPreset",  new PLUGIN_IMAGE_BRUSH(TEXT("/Resources/paint"), Icon40x40));
	Style->Set("ClassThumbnail.StarRainPreset",  new PLUGIN_IMAGE_BRUSH(TEXT("/Resources/paint"), Icon40x40));
	
	return Style;
}

void FStarRainStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FStarRainStyle::Get()
{
	return *StyleInstance;
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT
