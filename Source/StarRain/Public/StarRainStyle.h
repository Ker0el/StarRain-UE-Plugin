// Copyright Saeid Gholizade. All Rights Reserved. 2020
// ---------------------------------------------------------------------------
// 本文件是 Physical Layout Tool 的修改版本。原作品作者 Saeid Gholizade，
// 采用 CC BY 4.0 许可 (https://creativecommons.org/licenses/by/4.0/)。
// 修改者：星空 (https://space.bilibili.com/177308205)
// 修改内容与完整署名见插件根目录的 NOTICE.txt
// ---------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"

/**  */
class FStarRainStyle
{
public:
	/** Initialize toolkit style */
	static void Initialize();

	/** Destroys toolkit style */
	static void Shutdown();

	/** reloads textures used by slate renderer */
	static void ReloadTextures();

	/** Returns The Slate style set */
	static const ISlateStyle& Get();

	/** Returns The Slate style set name */
	static FName GetStyleSetName();

private:

	/** Creates all styles settings */
	static TSharedRef< class FSlateStyleSet > Create();

private:

	/** Slate Style Ptr */
	static TSharedPtr< class FSlateStyleSet > StyleInstance;
};