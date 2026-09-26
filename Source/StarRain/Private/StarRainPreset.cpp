// ---------------------------------------------------------------------------
// 本文件是 Physical Layout Tool 的修改版本。原作品作者 Saeid Gholizade，
// 采用 CC BY 4.0 许可 (https://creativecommons.org/licenses/by/4.0/)。
// 修改者：星空 (https://space.bilibili.com/177308205)
// 修改内容与完整署名见插件根目录的 NOTICE.txt
// ---------------------------------------------------------------------------
// Fill out your copyright notice in the Description page of Project Settings.
#include "StarRainPreset.h"


UStarRainPreset_Factory::UStarRainPreset_Factory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UStarRainPreset::StaticClass();

	bCreateNew = true;
}


