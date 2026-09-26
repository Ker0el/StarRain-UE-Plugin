// Copyright Saeid Gholizade. All Rights Reserved. 2020
// ---------------------------------------------------------------------------
// 本文件是 Physical Layout Tool 的修改版本。原作品作者 Saeid Gholizade，
// 采用 CC BY 4.0 许可 (https://creativecommons.org/licenses/by/4.0/)。
// 修改者：星空 (https://space.bilibili.com/177308205)
// 修改内容与完整署名见插件根目录的 NOTICE.txt
// ---------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Toolkits/BaseToolkit.h"
#include "Engine/StaticMesh.h"
#include "AssetThumbnail.h"
#include "StarRainPreset.h"
#include "Widgets/Input/SSlider.h"

class FStarRainMode;

/** Reference Mesh Struct */
struct FReferenceMesh
{
	UPROPERTY()
	UStaticMesh* StaticMesh = nullptr;

	float Chance=100;
	bool bIsAdjusting;
	FVector MinRotate;
	FVector MaxRotate;
	FVector MinScale;
	FVector MaxScale;
};

/** 一键预设 */
enum class EStarRainPreset : uint8
{
	NaturalScatter,   // 自然散落：像落叶一样随机铺散
	NeatStack,        // 整齐堆叠：像砖块一样码放
	DenseCover,       // 密集铺满：草丛/碎石那种高密度
	SparseAccent,     // 稀疏点缀：偶尔放几个
};

/** StarRain Toolkit
 *
 *  界面重构说明（相对原版）：
 *   - 顶部工具条 4 个模式 → 3 个（「笔刷选择」合并进「选择」）
 *   - 「要放置的物体」列表提到最上面 —— 这是用户第一件要干的事
 *   - 新增「随机程度」单一滑块，替代原来要分别调的 6 个 min/max 随机参数
 *   - 新增 4 个一键预设，新手点一下就能出效果
 *   - 原面板里其余 14 个参数全部收进「高级设置」折叠区，功能一个没删
 *   - 操作类按钮（烘焙/设为静态/加物理…）统一挪到最下方独立区块
 */
class FStarRainToolkit : public FModeToolkit
{
public:

	/** Constrcutor */
	FStarRainToolkit();

	/** FModeToolkit interface */
	virtual void Init(const TSharedPtr<IToolkitHost>& InitToolkitHost) override;
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual class FEdMode* GetEditorMode() const override;
	virtual TSharedPtr<class SWidget> GetInlineContent() const override { return ToolkitWidget; }

	/** Current layout mode */
	FString CurrentLayoutMode;

	/** Returns Previous layout mode */
	FString GetLastLayoutMode() const { return LastLayoutMode; }

	/** Returns current layout mode */
	FString GetCurrentLayoutMode() const { return CurrentLayoutMode; }

	/** Returns current layout mode text */
	FText GetCurrentLayoutModeText() const { return FText::FromString(CurrentLayoutMode); }

	/** Binds the toolkit commands */
	void BindCommands();

	/** Returns minimum distance between each placed actor */
	float GetMinDistance();

	/** Returns the current actor to place */
	UStaticMesh* GetRandomMesh();

	/** Changes the layout mode by name */
	void ChangeMode(FString InLayoutMode);

	/** Changes the layout mode by index (Tab 键循环用) */
	void ChangeMode(int InDirection);

	/** Returns true if we are selecting the placed actor  */
	bool IsSelectingPlacedActors() { return bSelectPlacedActors; }

	/** Returns true if minimum scale random is lock */
	bool IsMinScaleLock () { return bMinScaleLock; }

	/** Returns true if maximum scale random is lock */
	bool IsMaxScaleLock () { return bMaxScaleLock; }

	/** Returns true if gravity is enable */
	bool IsEnableGravity () { return bEnableGravity; }

	/** Returns true if use selectd is enable */
	bool IsUseSelected () { return bUseSelected; }

	/** Returns normal distance for hited polygon */
	float GetNormalDistance () { return fNormalDistance; }

	/** Returns position randomness */
	FVector GetPositionRandom() { return PositionRandom; }

	/** Returns rotatin randomness */
	FRotator GetRotateRandom() { return RotateRandom; }

	/** Returns scale randomness */
	FVector GetScaleRandom() { return ScaleRandom; }

	/** Returns normal rotation offset */
	FRotator GetNormalRotation() { return NormalRotation; }

	/** Returns the unified randomness slider value (0 = tidy, 1 = chaotic) */
	float GetRandomness() const { return Randomness; }

	/** Sets the unified randomness slider value and applies it to the six min/max params */
	void SetRandomness(float InValue);

	/** Applies one of the one-click presets */
	void ApplyPreset(EStarRainPreset InPreset);

	/** Sets the next actor to place */
	void SetRandomMesh();

	/** Sets the layout mode */
	void SetEdMode(FStarRainMode* InEdMode) { EdMode = InEdMode; }

	/** Returns true if damping velocity */
	bool IsDamplingVelocity()
	{ return bDampVelocity; }

	/** Sets Damp Velocity */
	void SetDampVelocity(bool InDampVelocity)
	{ bDampVelocity = InDampVelocity; }

	void LoadPLPreset(const FAssetData& InAsset);

private:

	/** 统一随机程度：0 = 完全对齐，1 = 杂乱散布 */
	float Randomness = 0.35f;

	/** 把 Randomness 按比例映射到六个 min/max 随机参数上 */
	void ApplyRandomness();

	/** Is Velocity Getting Damp */
	bool bDampVelocity;

	/** Previous layout mode */
	FString LastLayoutMode;

	TSharedPtr<SListView<TSharedPtr<FReferenceMesh>>> ReferenceMeshesListView;
	/** List of Reference meshes */
	TArray<TSharedPtr<FReferenceMesh>> ReferenceMeshes;

	/** Minimum position random */
	FVector MinPositionRandom = FVector::ZeroVector;

	/** Maximum position random */
	FVector MaxPositionRandom = FVector::ZeroVector;

	/** Position random */
	FVector PositionRandom = FVector::ZeroVector;

	/** Minimum rotation random */
	FRotator MinRotateRandom = FRotator::ZeroRotator;

	/** Maximum rotation random */
	FRotator MaxRotateRandom = FRotator::ZeroRotator;

	/** Rotation random */
	FRotator RotateRandom = FRotator::ZeroRotator;

	/** Minimum scale random */
	FVector MinScaleRandom = FVector::OneVector;

	/** Maximum scale random */
	FVector MaxScaleRandom = FVector::OneVector;

	/** Scale random */
	FVector ScaleRandom = FVector::OneVector;

	/** Nornaml rotation offset */
	FRotator NormalRotation = FRotator::ZeroRotator;

	/** Layout mode */
	FStarRainMode* EdMode = nullptr;

	/** Normal distance for hited polygon */
	float fNormalDistance = 0;

	/** Selected reference mesh index */
	int SelectedMeshIndex = -1;

	/** Minimum distance to last placed actor */
	float fMinDistance = 2;

	/** Is select placed actor enable */
	bool bSelectPlacedActors = true;

	/** Is minimum scale lock */
	bool bMinScaleLock = false;

	/** Is maximum scale lock */
	bool bMaxScaleLock = false;

	/** Is gravity enable */
	bool bEnableGravity = false;

	/** Is use selected enable */
	bool bUseSelected = false;

	/** Layout mode name */
	FString LayoutMode;

	/** Selected reference mesh */
	UStaticMesh* PickedMesh = nullptr;

	/** Is percents are relative */
	bool bIsPercentRelative;

	/** SSlider references for reference mesh chance */
	TMap<TSharedPtr<FReferenceMesh>, TSharedPtr<SSlider>> Sliders;

	/** Layout mode names (Tab 键循环用，与工具条顺序一致) */
	TArray<TSharedPtr<FString>> LayoutModes;

	/** Toolkit's SWidget */
	TSharedPtr<SWidget> ToolkitWidget;

	/** Current layout mode index */
	int LayoutModeIndex = 0;

	/** Reference mesh thumbnail pool */
	TSharedPtr<FAssetThumbnailPool> ThumbnailPool;

	/** Bakes the places actors into instance mesh */
	void BakeToInstanceMesh(bool BakeSelected);

	/** Returns reference meshes */
	TArray<TSharedPtr<FReferenceMesh>> GetReferenceMeshes() const;

	/** Reference mesh widget delegate */
	TSharedRef<ITableRow> GetReferenceMeshWidget(TSharedPtr<FReferenceMesh> InItem, const TSharedRef<STableViewBase>& OwnerTable);

	// ── 面板区块 ────────────────────────────────────────────
	/** ① 要放置的物体列表 */
	TSharedPtr<SWidget> CreateMeshListWidget();
	/** ② 基础设置：间距 / 随机程度 / 重力 */
	TSharedPtr<SWidget> CreateBasicWidget();
	/** ③ 一键预设 */
	TSharedPtr<SWidget> CreatePresetsWidget();
	/** ④ 高级设置（折叠）：其余 14 项 */
	TSharedPtr<SWidget> CreateAdvancedWidget();
	/** ⑤ 已放置物体的操作按钮组 */
	TSharedPtr<SWidget> CreateActionWidget();

	/** 摆放模式主面板（把上面五个区块装配起来） */
	TSharedPtr<SWidget> CreatePaintModeWidget();

	/** Creates select mode widget */
	TSharedPtr<SWidget> CreateSelectModeWidget();

	/** Creates transfrom mode widget */
	TSharedPtr<SWidget> CreateTransformModeWidget();

	FReply SavePLPreset();

	// 弱指针：PLPreset 指向一张预设资产，资产可能被 GC 回收或用户在内容浏览器里删掉。
	// 原来这里是裸指针且不是 UPROPERTY（Toolkit 不是 UObject，没法标记），
	// 资产一被回收就成悬空指针 —— 点「保存预设」时解引用必崩。
	TWeakObjectPtr<UStarRainPreset> PLPreset;
};
