// Copyright Saeid Gholizade. All Rights Reserved. 2020
// ---------------------------------------------------------------------------
// 本文件是 Physical Layout Tool 的修改版本。原作品作者 Saeid Gholizade，
// 采用 CC BY 4.0 许可 (https://creativecommons.org/licenses/by/4.0/)。
// 修改者：星空 (https://space.bilibili.com/177308205)
// 修改内容与完整署名见插件根目录的 NOTICE.txt
// ---------------------------------------------------------------------------

#include "StarRain.h"
#include "StarRainMode.h"
#include "StarRainCommands.h"
#include "StarRainStyle.h"


#define LOCTEXT_NAMESPACE "FStarRainModule"

void FStarRainModule::StartupModule()
{
	FStarRainStyle::Initialize();
	FStarRainStyle::ReloadTextures();
	FStarRainCommands::Register();

	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FEditorModeRegistry::Get().RegisterMode<FStarRainMode>(
		FStarRainMode::EM_StarRainModeId, 
		LOCTEXT("StarRainEdModeName", "星雨"),
		FSlateIcon(FStarRainStyle::GetStyleSetName(), "StarRain", "StarRain.Icon"),
		true);
}

void FStarRainModule::ShutdownModule()
{
	FStarRainStyle::Shutdown();
	FStarRainCommands::Unregister();

	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FEditorModeRegistry::Get().UnregisterMode(FStarRainMode::EM_StarRainModeId);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FStarRainModule, StarRain)