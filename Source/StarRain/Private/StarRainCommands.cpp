// Copyright Saeid Gholizade. All Rights Reserved. 2020
// ---------------------------------------------------------------------------
// 本文件是 Physical Layout Tool 的修改版本。原作品作者 Saeid Gholizade，
// 采用 CC BY 4.0 许可 (https://creativecommons.org/licenses/by/4.0/)。
// 修改者：星空 (https://space.bilibili.com/177308205)
// 修改内容与完整署名见插件根目录的 NOTICE.txt
// ---------------------------------------------------------------------------


#include "StarRainCommands.h"

#define LOCTEXT_NAMESPACE "StarRainCommands"

void FStarRainCommands::RegisterCommands()
{
	// 按钮标签会显示在工具条上，鼠标悬停显示提示
	// 注意：原版这里是 "Select" / "Paint Select" / "Transform" / "Paint Place" 四个英文模式，
	// 已简化成三个并汉化。EUserInterfaceActionType::RadioButton 保证同时只有一个被按下。
	UI_COMMAND(SelectCommand,
		"选择",
		"单击选择物体，拖拽刷选，Ctrl+单击取消选择",
		EUserInterfaceActionType::RadioButton, FInputChord());

	UI_COMMAND(TransformationCommand,
		"调整",
		"选中物体后拖红绿蓝箭头移动，物体受物理影响会碰撞、坠落",
		EUserInterfaceActionType::RadioButton, FInputChord());

	UI_COMMAND(PaintCommand,
		"摆放",
		"在场景里喷撒物体，它们会物理坠落后自然堆叠",
		EUserInterfaceActionType::RadioButton, FInputChord());
}
#undef LOCTEXT_NAMESPACE
