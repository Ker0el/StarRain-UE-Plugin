// ---------------------------------------------------------------------------
// 本文件是 Physical Layout Tool 的修改版本。原作品作者 Saeid Gholizade，
// 采用 CC BY 4.0 许可 (https://creativecommons.org/licenses/by/4.0/)。
// 修改者：星空 (https://space.bilibili.com/177308205)
// 修改内容与完整署名见插件根目录的 NOTICE.txt
// ---------------------------------------------------------------------------
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeCategories.h"
#include "Engine/DataAsset.h"
#include "Engine/StaticMesh.h"
#include "Factories/Factory.h"
#include "UObject/Object.h"
#include "StarRainPreset.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct STARRAIN_API FPLPaintObject
{
	GENERATED_BODY()
public:
	// 原版写的是空的 FPLPaintObject(){}，StaticNesh 指针从未初始化。
	// UE 反射系统规则：一旦声明自定义默认构造函数，就必须初始化所有 UPROPERTY 成员，
	// 否则启动时报 "ObjectProperty FPLPaintObject::StaticNesh is not initialized properly"，
	// 加载预设资产时可能读到野指针。
	FPLPaintObject() : StaticNesh(nullptr) {}
	FPLPaintObject(UStaticMesh* InStaticMesh, float InChance)
		: StaticNesh(InStaticMesh)
		, Chance(InChance)
	{
	}

	// 注意：成员名 StaticNesh 是原作者的拼写错误（少个 m）。
	// 不要"修正"它 —— 这是序列化字段名，改名会导致已有的预设资产读不出来。
	UPROPERTY(EditAnywhere, Category="Preset")
	UStaticMesh* StaticNesh = nullptr;

	UPROPERTY(EditAnywhere, Category="Preset", meta=(UIMin="0", UIMax = "100"))
	float Chance=100;
};

UCLASS(BlueprintType)
class STARRAIN_API UStarRainPreset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category="Preset")
	TArray<FPLPaintObject> PaintObjects;
};

UCLASS()
class STARRAIN_API UStarRainPreset_Factory : public UFactory
{
	GENERATED_UCLASS_BODY()
public:
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override
	{
		auto Preset = NewObject<UStarRainPreset>(InParent, InClass, InName, Flags);
		Preset->Modify();
		Preset->MarkPackageDirty();
		Preset->PostEditChange();
		return Preset;
	}

	virtual FText GetDisplayName() const override
	{
		return FText::FromString("StarRain 预设");
	}

	virtual uint32 GetMenuCategories() const override
	{
		return EAssetTypeCategories::Misc;
	}

	virtual bool CanCreateNew() const override
	{
		return true;
	}
};
