// Copyright Saeid Gholizade. All Rights Reserved. 2020
// ---------------------------------------------------------------------------
// 本文件是 Physical Layout Tool 的修改版本。原作品作者 Saeid Gholizade，
// 采用 CC BY 4.0 许可 (https://creativecommons.org/licenses/by/4.0/)。
// 修改者：星空 (https://space.bilibili.com/177308205)
// 修改内容与完整署名见插件根目录的 NOTICE.txt
// ---------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "StarRainStyle.h"

/**
 * StarRain 模式的工具条命令
 * 重构说明：原来的 4 个模式（选择/笔刷选择/物理变换/喷撒）已简化为 3 个，
 * 「笔刷选择」合并进了「选择」——单击选择和拖拽刷选本来就是同一个动作的两种手势。
 */
class FStarRainCommands : public TCommands< FStarRainCommands >
{

public:
	/** Constructor */
	FStarRainCommands()
		: TCommands<FStarRainCommands>(
			TEXT("StarRain"), // Context name for fast lookup
			NSLOCTEXT("Contexts", "StarRain", "星雨模式"), // Localized context name for displaying
			NAME_None, // Parent
			FStarRainStyle::GetStyleSetName()
			)
	{
	}

	/** Command interface */
	virtual void RegisterCommands() override;

	/** 选择命令 */
	TSharedPtr<FUICommandInfo> SelectCommand;

	/** 调整命令（Gizmo 物理拖拽） */
	TSharedPtr<FUICommandInfo> TransformationCommand;

	/** 摆放命令（物理喷撒） */
	TSharedPtr<FUICommandInfo> PaintCommand;

	/** Command list */
	TArray<TSharedPtr<FUICommandInfo>> Commands = {
		SelectCommand,
		TransformationCommand,
		PaintCommand
	};

};
