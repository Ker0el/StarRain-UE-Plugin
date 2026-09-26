// Copyright Saeid Gholizade. All Rights Reserved. 2020
// ---------------------------------------------------------------------------
// 本文件是 Physical Layout Tool 的修改版本。原作品作者 Saeid Gholizade，
// 采用 CC BY 4.0 许可 (https://creativecommons.org/licenses/by/4.0/)。
// 修改者：星空 (https://space.bilibili.com/177308205)
// 修改内容与完整署名见插件根目录的 NOTICE.txt
// ---------------------------------------------------------------------------

#include "StarRainToolkit.h"
#include "Engine/Selection.h"

#include "Widgets/Views/SListView.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Text/STextBlock.h"
#include "EditorModeManager.h"
#include "FileHelpers.h"
#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "PropertyCustomizationHelpers.h"
#include "Modules/ModuleManager.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/MultiBox/MultiBox.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Misc/Attribute.h"
#include "Textures/SlateIcon.h"
#include "StarRainCommands.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Input/SHyperlink.h"
#include "HAL/PlatformProcess.h"
#include "Widgets/Input/SVectorInputBox.h"
#include "Widgets/Input/SRotatorInputBox.h"
#include "Widgets/Input/SSpinBox.h"

#include "SlateUtil.h"
#include "StarRainMode.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "EngineUtils.h"
#include "MeshMergeModule.h"
#include "StarRainPreset.h"
#include "SourceControlOperations.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"

// ── FMeshInstancingSettings 的位置在版本之间搬过家 ────────────────────────
//   5.4  : 在 Engine/MeshMerging.h 里
//   5.5  : 拆到 MeshMerge/MeshInstancingSettings.h（此时 MeshMerging.h 仍在）
//   5.8  : MeshMerging.h 本身也被拆走并移除了
//
// 本插件真正需要的只有 FMeshInstancingSettings 一个类型（MergeComponentsToInstances
// 的参数）。原版 include 的 Engine/MeshMerging.h 其实一个符号都没用到 —— 5.8 上它
// 已不存在，正好暴露了这一点。所以这里按版本只包含真正需要的那一个。
//
// 注意：不要用 UE_VERSION_NEWER_THAN(5,4,0) 判断 —— 它在 5.4.4 上同样为真，
// 会导致 5.4 去包含 5.5 才有的头文件，直接 C1083。必须比对主次版本号。
#include "Runtime/Launch/Resources/Version.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 4
	#include "Engine/MeshMerging.h"
#else
	#include "MeshMerge/MeshInstancingSettings.h"
#endif

#define LOCTEXT_NAMESPACE "FStarRainToolkit"

FStarRainToolkit::FStarRainToolkit()
{
	ThumbnailPool = MakeShareable(new FAssetThumbnailPool(24));
	bIsPercentRelative = false;
	bUseSelected = false;
	bEnableGravity = false;
	CurrentLayoutMode = ELayoutMode::Select;
}

void FStarRainToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost)
{
	BindCommands();

	TSharedPtr<FReferenceMesh> mesh = MakeShareable<FReferenceMesh>(new FReferenceMesh());
	mesh.Get()->Chance = 100;
	ReferenceMeshes.Add(mesh);

	// Tab 键循环的三个模式，顺序与工具条一致
	LayoutModes.Add(MakeShared<FString>(ELayoutMode::Paint));
	LayoutModes.Add(MakeShared<FString>(ELayoutMode::Select));
	LayoutModes.Add(MakeShared<FString>(ELayoutMode::Transform));

	// 默认进入「摆放」——绝大多数人打开这个工具就是想撒东西
	CurrentLayoutMode = ELayoutMode::Paint;

	const FStarRainCommands& Commands = FStarRainCommands::Get();
	
	TSharedPtr<FUICommandList> CommandList = GetToolkitCommands();

	FToolBarBuilder LayoutModeButtons(CommandList, FMultiBoxCustomization::None);

	// 三个动作按钮。「摆放」放第一个 —— 新手一眼就知道从哪开始
	LayoutModeButtons.AddToolBarButton(Commands.PaintCommand);
	LayoutModeButtons.AddToolBarButton(Commands.SelectCommand);
	LayoutModeButtons.AddToolBarButton(Commands.TransformationCommand);

	
	SAssignNew(ToolkitWidget, SBorder)
	.HAlign(HAlign_Fill)
	.Padding(10)
	[
		SNew(SVerticalBox)

		// 模式工具条（3 个按钮，替代原来的 4 个）
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 0, 0, 8)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.HAlign(HAlign_Center)
			[
				LayoutModeButtons.MakeWidget()
			]
		]

		// 摆放面板（默认模式，最完整）
		+ SVerticalBox::Slot()
		[
			CreatePaintModeWidget().ToSharedRef()
		]

		// 选择面板
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			CreateSelectModeWidget().ToSharedRef()
		]

		// 调整面板
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			CreateTransformModeWidget().ToSharedRef()
		]

		// 底部：作者署名 + B 站超链接（三种模式下都常驻可见）
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.Padding(0, 12, 0, 0)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("StarRainByline", "作者："))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				// 蓝色超链接样式直接借用引擎自带的「跳转到蓝图」超链接样式，
				// 是蓝色的带点线下划线，和编辑器其它超链接观感一致。
				SNew(SHyperlink)
				.Style(FAppStyle::Get(), "Common.GotoBlueprintHyperlink")
				.Text(LOCTEXT("StarRainBilibiliLink", "星空"))
				.ToolTipText(LOCTEXT("StarRainBilibiliLinkTip", "在 B 站打开「星空插件」主页\nhttps://space.bilibili.com/177308205"))
				.OnNavigate_Lambda([]()
				{
					FPlatformProcess::LaunchURL(TEXT("https://space.bilibili.com/177308205"), nullptr, nullptr);
				})
			]
		]
	];
	FModeToolkit::Init(InitToolkitHost);
}

//void FStarRainToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& TabManager)
//{
//	SetEdMode(nullptr);
//	
//	TSharedPtr<FUICommandList> CommandList = GetToolkitCommands();
//
//	auto Commands = FStarRainCommands::Get();
//
//	for (auto& Command : Commands.Commands)
//	{
//		CommandList->UnmapAction(Command);
//	}
//
//	CommandList.Reset();
//	
//	FModeToolkit::UnregisterTabSpawners(TabManager);
//}

TArray<TSharedPtr<FReferenceMesh>> FStarRainToolkit::GetReferenceMeshes() const
{
	return ReferenceMeshes;
}

TSharedRef<ITableRow> FStarRainToolkit::GetReferenceMeshWidget(TSharedPtr<FReferenceMesh> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	TSharedPtr<SSlider> Slider;
	TSharedPtr<STableRow<TSharedPtr<FReferenceMesh>>> row;

	// 重构说明：原版这一行是「左：资产框+权重滑块 / 右：两个 20px 纯图标按钮（垃圾桶、方块堆）」，
	// 图标没文字没提示，新手完全看不出是干嘛的。现在改成竖向三段式：
	//   ① 选网格  ② 调权重  ③ 两个带文字的按钮
	SAssignNew(row, STableRow<TSharedPtr<FReferenceMesh>>, OwnerTable)
	[
		SNew(SVerticalBox)

		// ① 选网格
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SObjectPropertyEntryBox)
			.ObjectPath_Lambda([this, InItem]() -> FString
			{
				if (!InItem.IsValid() || !InItem.Get()->StaticMesh)
				{
					return FString();
				}
				return InItem.Get()->StaticMesh->GetPathName();
			})
			.OnObjectChanged_Lambda([this, InItem](const FAssetData& InAsset)
				{
					if (InAsset.IsValid())
					{
						InItem.Get()->StaticMesh = Cast<UStaticMesh>(InAsset.GetAsset());
						SetRandomMesh();
						if (EdMode)
						{
							EdMode->RegisterBrush();
						}
					}
				}
			)
			.AllowedClass(UStaticMesh::StaticClass())
			.DisplayThumbnail(true)
			.DisplayUseSelected(false)
			.DisplayBrowse(true)
			.DisplayUseSelected(true)
			.ThumbnailPool(ThumbnailPool)
		]

		// ② 权重：这种物体被抽中的概率
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SAssignNew(Slider, SSlider)
			.MinValue(0)
			.MaxValue(100)
			.OnValueChanged_Lambda([this, InItem](float InValue)
				{
					float delta = (InItem.Get()->Chance - InValue);
					float sign = delta / delta;
					InItem.Get()->Chance = InValue;
					if (bIsPercentRelative)
					{
						if (InItem.Get()->bIsAdjusting)
						{
							return;
						}
						if (ReferenceMeshes.Num() > 1)
						{
							float sum = 0;
							int c = 0;
							for (auto& RefMesh : ReferenceMeshes)
							{
								if (RefMesh != InItem)
								{
									RefMesh.Get()->bIsAdjusting = true;
									RefMesh.Get()->Chance += delta;
									delta = RefMesh.Get()->Chance < 0 ? RefMesh.Get()->Chance : RefMesh.Get()->Chance >= 100 ? RefMesh.Get()->Chance - 100 : 0;
									RefMesh.Get()->Chance = FMath::Clamp(RefMesh.Get()->Chance, 0.0f, 100.0f);

									if (auto RefSlider = Sliders.Find(RefMesh))
									{
										RefSlider->Get()->SetValue(RefMesh.Get()->Chance);
									}

									RefMesh.Get()->bIsAdjusting = false;
								}
								sum += RefMesh.Get()->Chance;
								c++;
							}
						}
					}
					SetRandomMesh();
				}
			)
			.Value(InItem.Get()->Chance)
			.ForceVolatile(true)
			.ToolTipText(LOCTEXT("MeshChanceTip", "这种物体被抽中的概率。加多种物体时，按这个比例随机混着撒"))
		]

		// ③ 两个操作按钮 —— 原来是没文字的图标，现在写清楚
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 4, 0, 0)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(0.5f)
			.Padding(0, 0, 2, 0)
			[
				SNew(SButton)
				.Text(LOCTEXT("SelectMeshBtn", "选中这类"))
				.ToolTipText(LOCTEXT("SelectByMeshTip",
					"选中场景里所有用这个网格撒出去的物体\n"
					"选中后可以用「调整」拖动，或点「设为静态」定稿"))
				.HAlign(EHorizontalAlignment::HAlign_Center)
				.VAlign(EVerticalAlignment::VAlign_Center)
				.OnClicked_Lambda([this, InItem]()
				{
					if (EdMode)
					{
						EdMode->SelectPlacedActors(InItem->StaticMesh);
					}
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.5f)
			.Padding(2, 0, 0, 0)
			[
				SNew(SButton)
				.Text(LOCTEXT("RemoveMeshBtn", "移除此类"))
				.ToolTipText(LOCTEXT("DeleteMeshTip",
					"从列表里移除这一种物体\n"
					"（只是不再撒它，场景里已经撒出去的不会被删）"))
				.HAlign(EHorizontalAlignment::HAlign_Center)
				.VAlign(EVerticalAlignment::VAlign_Center)
				.IsEnabled_Lambda([this]() { return ReferenceMeshes.Num() > 1; })
				.OnClicked_Lambda([this, InItem]()
					{
						if (ReferenceMeshes.Num() > 1)
						{
							int32 RefMeshIndex = ReferenceMeshes.Find(InItem);
							if (RefMeshIndex >= 0)
							{
								TSharedPtr<FReferenceMesh> RefMesh = ReferenceMeshes[RefMeshIndex];
								ReferenceMeshes.RemoveAt(RefMeshIndex);
								RefMesh.Reset();
							}
						}
						return FReply::Handled();
					})
			]
		]
	];

	if (!Sliders.Find(InItem))
	{
		Sliders.Add(InItem, Slider);
	}
	return row.ToSharedRef();
}

// ═══════════════════════════════════════════════════════════════════════════
//  面板区块
//
//  重构说明：原版是一个 632 行的大函数把所有控件平铺出来，新手打开就是 20 个
//  数字框糊脸。现在拆成 5 个区块，按「用户实际的操作顺序」排列：
//
//    ① 要放置的物体  —— 第一件事，先告诉工具你要撒什么
//    ② 基础设置      —— 只有 3 项，其中「随机程度」一个滑块顶原来 6 个参数
//    ③ 一键预设      —— 不想调参就点这里
//    ④ 高级设置      —— 折叠，原来那 14 个参数全在这，功能零损失
//    ⑤ 物体操作      —— 合并 / 设为静态 / 加物理 等
// ═══════════════════════════════════════════════════════════════════════════

namespace StarRainPanelLayout
{
	/** 「标签 + 控件」两列布局的宽度比例 */
	static const float LabelWidth   = 0.40f;
	static const float ControlWidth = 0.60f;
}

/** 把 Randomness(0..1) 按比例映射到六个 min/max 随机参数上
 *  这是本次界面简化最关键的一环 —— 用户只面对一个滑块。 */
void FStarRainToolkit::ApplyRandomness()
{
	const float t = FMath::Clamp(Randomness, 0.0f, 1.0f);

	// 位置：0 → 完全重合，1 → 水平面内 ±30cm 散布
	MinPositionRandom = FVector(-30.0f * t, -30.0f * t, 0.0f);
	MaxPositionRandom = FVector( 30.0f * t,  30.0f * t, 0.0f);

	// 旋转：绕 Z 轴全周随机（最出效果的一项），俯仰/翻滚只给一点点
	// 注意 FRotator 构造顺序是 (Pitch, Yaw, Roll)
	MinRotateRandom = FRotator(-6.0f * t, -180.0f * t, -10.0f * t);
	MaxRotateRandom = FRotator( 6.0f * t,  180.0f * t,  10.0f * t);

	// 缩放：在 1.0 附近抖动
	MinScaleRandom = FVector(1.0f - 0.20f * t);
	MaxScaleRandom = FVector(1.0f + 0.25f * t);

	SetRandomMesh();
}

void FStarRainToolkit::SetRandomness(float InValue)
{
	Randomness = FMath::Clamp(InValue, 0.0f, 1.0f);
	ApplyRandomness();
}

void FStarRainToolkit::ApplyPreset(EStarRainPreset InPreset)
{
	switch (InPreset)
	{
	case EStarRainPreset::NaturalScatter:
		// 像落叶：朝向完全随机，物体之间留点间隔
		Randomness = 0.45f;
		fMinDistance = 1.0f;
		break;

	case EStarRainPreset::NeatStack:
		// 像码砖：完全不随机，只靠重力自然堆叠
		Randomness = 0.0f;
		fMinDistance = 1.0f;
		break;

	case EStarRainPreset::DenseCover:
		// 草丛/碎石：挨得近，随机大
		Randomness = 0.70f;
		fMinDistance = 0.5f;
		break;

	case EStarRainPreset::SparseAccent:
		// 点缀：偶尔放几个
		Randomness = 0.60f;
		fMinDistance = 6.0f;
		break;
	}

	// 所有预设都让物体受重力，这是这个工具的灵魂
	fNormalDistance = 0.0f;
	NormalRotation = FRotator::ZeroRotator;
	bEnableGravity = true;

	ApplyRandomness();

	// ── 预设专属微调 ──────────────────────────────────────────
	// 注意：必须放在 ApplyRandomness() 之后。ApplyRandomness 会按 Randomness
	// 重写全部六个 min/max 参数，写在前面的任何值都会被它覆盖掉。
	if (InPreset == EStarRainPreset::NaturalScatter)
	{
		// 位置：X/Y 归零，Z 固定抬高 200 —— 让物体从高处落下来，自然散开堆叠
		MinPositionRandom = FVector(0.0f, 0.0f, 200.0f);
		MaxPositionRandom = FVector(0.0f, 0.0f, 200.0f);

		// 旋转：三轴都是 0~360，也就是完全随机朝向
		MinRotateRandom = FRotator(0.0f, 0.0f, 0.0f);
		MaxRotateRandom = FRotator(360.0f, 360.0f, 360.0f);

		// 缩放：0.8 ~ 1.2
		MinScaleRandom = FVector(0.8f);
		MaxScaleRandom = FVector(1.2f);
	}

	SetRandomMesh();
}

// ─── 模式面板 ─────────────────────────────────────────────────────────────

TSharedPtr<SWidget> FStarRainToolkit::CreateSelectModeWidget()
{
	return SNew(SBox)
	.Visibility_Lambda([this]() { return GetCurrentLayoutMode() == ELayoutMode::Select ? EVisibility::Visible : EVisibility::Collapsed; })
	.WidthOverride(320)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 0, 0, 8)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(8)
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.ColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.80f, 1.0f)))
				.Text(LOCTEXT("SelectModeHelp", "单击选择物体 · 拖拽可刷选一片\n按住 Ctrl 单击 = 取消选择"))
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			CreateActionWidget().ToSharedRef()
		]
	];
}

TSharedPtr<SWidget> FStarRainToolkit::CreateTransformModeWidget()
{
	return SNew(SBox)
	.Visibility_Lambda([this]() { return GetCurrentLayoutMode() == ELayoutMode::Transform ? EVisibility::Visible : EVisibility::Collapsed; })
	.WidthOverride(320)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 0, 0, 8)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(8)
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.ColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.80f, 1.0f)))
				.Text(LOCTEXT("TransformModeHelp",
					"先切到「选择」选中物体，再回到这里\n"
					"拖动红绿蓝箭头 = 移动，按 E 换成旋转圆环\n"
					"物体带物理：会撞到别的东西，会掉下去\n"
					"按 R 拖缩放方块 = 让物体互相推开或靠拢"))
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			CreateActionWidget().ToSharedRef()
		]
	];
}

/** ⑤ 已放置物体的操作按钮组 */
TSharedPtr<SWidget> FStarRainToolkit::CreateActionWidget()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 0, 0, 4)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("ActionTitle", "对已放置的物体"))
			.Font(FAppStyle::GetFontStyle("BoldFont"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SCheckBox)
			.IsChecked_Lambda([this]() { return IsSelectingPlacedActors() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([this](ECheckBoxState InCheck)
			{
				bSelectPlacedActors = InCheck == ECheckBoxState::Checked;
			})
			.Content()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("SelectPlaceActors", "只操作本工具放出来的物体"))
				.ToolTipText(LOCTEXT("SelectPlaceActorsTip", "勾上以后，选择/刷选只会命中这个工具撒出来的物体，不会误伤场景原有物件"))
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SCheckBox)
			.IsChecked_Lambda([this]() { return IsDamplingVelocity() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([this](ECheckBoxState InCheck)
			{
				SetDampVelocity(InCheck == ECheckBoxState::Checked);
			})
			.Content()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("DampVelocity", "拖拽时锁住速度"))
				.ToolTipText(LOCTEXT("DampVelocityTip", "勾上后物体不会在拖动过程中乱滚，松手才恢复物理"))
			]
		]
		// 选择 + 静态
		+ SVerticalBox::Slot()
		.Padding(0, 4, 0, 0)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(0.5f)
			.Padding(0, 0, 2, 0)
			[
				SNew(SButton)
				.HAlign(EHorizontalAlignment::HAlign_Center)
				.Text(LOCTEXT("PhysicLayoutModeSelectAll", "全选已放置"))
				.OnClicked_Lambda([this]()
				{
					if (EdMode) { EdMode->SelectPlacedActors(nullptr); }
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.5f)
			.Padding(2, 0, 0, 0)
			[
				SNew(SButton)
				.HAlign(EHorizontalAlignment::HAlign_Center)
				.Text(LOCTEXT("PhysicLayoutModeMakeStatic", "设为静态"))
				.ToolTipText(LOCTEXT("MakeStaticTip", "定稿：把选中的物体彻底固定住，不再受物理影响（省性能）"))
				.OnClicked_Lambda([this]()
				{
					if (EdMode) { EdMode->MakeSelectedStatic(); }
					return FReply::Handled();
				})
			]
		]
		// 重力开关（只对选中的物体）
		+ SVerticalBox::Slot()
		.Padding(0, 2, 0, 0)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(0.5f)
			.Padding(0, 0, 2, 0)
			[
				SNew(SButton)
				.HAlign(EHorizontalAlignment::HAlign_Center)
				.Text(LOCTEXT("PhysicLayoutModeToggleGravityEnable", "开启重力"))
				.ToolTipText(LOCTEXT("GravityOnTip", "让选中的物体重新掉下去"))
				.OnClicked_Lambda([this]()
				{
					if (EdMode)
					{
						for (auto& actor : EdMode->GetSelectedActors()) { EdMode->UpdatePhysics(actor, true); }
					}
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.5f)
			.Padding(2, 0, 0, 0)
			[
				SNew(SButton)
				.HAlign(EHorizontalAlignment::HAlign_Center)
				.Text(LOCTEXT("PhysicLayoutModeToggleGravityDisable", "关闭重力"))
				.ToolTipText(LOCTEXT("GravityOffTip", "让选中的物体悬停在原地"))
				.OnClicked_Lambda([this]()
				{
					if (EdMode)
					{
						for (auto& actor : EdMode->GetSelectedActors()) { EdMode->UpdatePhysics(actor, false); }
					}
					return FReply::Handled();
				})
			]
		]
		// 加入物理模拟
		+ SVerticalBox::Slot()
		.Padding(0, 2, 0, 0)
		.AutoHeight()
		[
			SNew(SButton)
			.HAlign(EHorizontalAlignment::HAlign_Center)
			.Text(LOCTEXT("PhysicLayoutModeAddSelectedActors", "把选中的场景物体也纳入物理"))
			.ToolTipText(LOCTEXT("AddSelectedTip", "让本来不参与物理的场景物件也能被落下的物体撞动"))
			.OnClicked_Lambda([this]()
			{
				USelection* SelectedActors = GEditor->GetSelectedActors();
				for (FSelectionIterator Iter(*SelectedActors); Iter; ++Iter)
				{
					if (AActor* LevelActor = Cast<AActor>(*Iter))
					{
						if (EdMode) { EdMode->AddSelectedActor(LevelActor); }
					}
				}
				return FReply::Handled();
			})
		]
		// 烘焙
		+ SVerticalBox::Slot()
		.Padding(0, 6, 0, 0)
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("BakeTitle", "定稿（省性能）"))
			.Font(FAppStyle::GetFontStyle("BoldFont"))
		]
		+ SVerticalBox::Slot()
		.Padding(0, 2, 0, 0)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(0.5f)
			.Padding(0, 0, 2, 0)
			[
				SNew(SButton)
				.HAlign(EHorizontalAlignment::HAlign_Center)
				.Text(LOCTEXT("PhysicLayoutModeBakeSelectedMesh", "合并选中的"))
				.ToolTipText(LOCTEXT("BakeSelectedTip", "把选中的物体合并成一个整体，帧数会明显变好。定稿前做这一步"))
				.OnClicked_Lambda([this]()
				{
					BakeToInstanceMesh(true);
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.5f)
			.Padding(2, 0, 0, 0)
			[
				SNew(SButton)
				.HAlign(EHorizontalAlignment::HAlign_Center)
				.Text(LOCTEXT("PhysicLayoutModeBakeAllMesh", "合并全部"))
				.ToolTipText(LOCTEXT("BakeAllTip", "把本工具撒出来的所有物体合并成一个整体，帧数会明显变好"))
				.OnClicked_Lambda([this]()
				{
					BakeToInstanceMesh(false);
					return FReply::Handled();
				})
			]
		]
		// 重置
		+ SVerticalBox::Slot()
		.Padding(0, 2, 0, 0)
		.AutoHeight()
		[
			SNew(SButton)
			.HAlign(EHorizontalAlignment::HAlign_Center)
			.Text(LOCTEXT("PhysicLayoutModeResetMesh", "把选中的物体放回原位"))
			.ToolTipText(LOCTEXT("ResetTip", "撤销物理位移，回到它们被放下去之前的位置和朝向"))
			.OnClicked_Lambda([this]()
			{
				if (EdMode) { EdMode->ResetTransform(); }
				return FReply::Handled();
			})
		];
}

FReply FStarRainToolkit::SavePLPreset()
{
	// ★ 原版这里必崩：上面 if (PLPreset) 判了空，出了块之后又直接 PLPreset->GetOutermost()，
	//   没选预设资产时点「保存」就是空指针解引用。
	UStarRainPreset* Preset = PLPreset.Get();
	if (!Preset)
	{
		FNotificationInfo Info(LOCTEXT("NoPresetToSave", "还没有选择预设资产\n请先用上面的资产框选一张 StarRain 预设（没有就新建一张）"));
		Info.ExpireDuration = 5.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
		return FReply::Handled();
	}

	Preset->Modify();
	Preset->PaintObjects.Reset();
	for (auto RefMesh : ReferenceMeshes)
	{
		if (RefMesh.IsValid())
		{
			Preset->PaintObjects.Add(FPLPaintObject(RefMesh->StaticMesh, RefMesh->Chance));
		}
	}

	// Save the package
	TArray<UPackage*> PackagesToSave;
	PackagesToSave.Add(Preset->GetOutermost());
	constexpr bool bCheckDirty = false;
	constexpr bool bPromptToSave = false;
	FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, bCheckDirty, bPromptToSave);
	return FReply::Handled();
}

/** ① 要放置的物体 */
TSharedPtr<SWidget> FStarRainToolkit::CreateMeshListWidget()
{
	auto VerticalScrollbar = SNew(SScrollBar)
		.Orientation(Orient_Vertical)
		.Thickness(FVector2D(14.0f, 14.0f));

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 0, 0, 4)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("MeshListTitle", "要放置的物体"))
			.Font(FAppStyle::GetFontStyle("BoldFont"))
		]
		// 预设资产（存/读这套配置）
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1)
			[
				SNew(SObjectPropertyEntryBox)
				.AllowedClass(UStarRainPreset::StaticClass())
				.DisplayBrowse(true)
				.AllowClear(true)
				.AllowCreate(true)
				.DisplayUseSelected(true)
				.DisplayCompactSize(true)
				.DisplayThumbnail(true)
				.ThumbnailPool(ThumbnailPool)
				.ObjectPath_Lambda([this]()
				{
					return PLPreset.IsValid() ? PLPreset->GetPathName() : FString();
				})
				.OnObjectChanged(this, &FStarRainToolkit::LoadPLPreset)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ButtonStyle(FStarRainStyle::Get(), "Save")
				.ToolTipText(LOCTEXT("SavePresetTip", "把当前的物体列表和参数存成一个预设资产"))
				.OnClicked(this, &FStarRainToolkit::SavePLPreset)
			]
		]
		// 添加
		+ SVerticalBox::Slot()
		.Padding(0, 4)
		.AutoHeight()
		[
			SNew(SButton)
			.HAlign(EHorizontalAlignment::HAlign_Center)
			.Text(LOCTEXT("PhysicLayoutModeAddMesh", "+ 添加一种物体"))
			.ToolTipText(LOCTEXT("AddMeshTip", "往列表里再加一种要撒的网格体。加多种会按权重随机混着撒"))
			.OnClicked_Lambda([this]()
			{
				TSharedPtr<FReferenceMesh> RefMesh = MakeShareable<FReferenceMesh>(new FReferenceMesh());
				ReferenceMeshes.Add(RefMesh);
				ReferenceMeshesListView->RequestListRefresh();
				return FReply::Handled();
			})
		]
		// 物体列表
		+ SVerticalBox::Slot()
		.MaxHeight(270.0f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SAssignNew(ReferenceMeshesListView, SListView<TSharedPtr<FReferenceMesh>>)
				.ItemHeight(132)
				.ListItemsSource(&ReferenceMeshes)
				.OnGenerateRow(this, &FStarRainToolkit::GetReferenceMeshWidget)
				.OnSelectionChanged_Lambda([this](TSharedPtr<FReferenceMesh> InMesh, ESelectInfo::Type SelectionType)
					{
						SelectedMeshIndex = ReferenceMeshes.IndexOfByKey(InMesh);
						SetRandomMesh();
					}
				)
				.ForceVolatile(true)
				.ExternalScrollbar(VerticalScrollbar)
			]
		];
}

/** ② 基础设置 —— 只有 3 项 */
TSharedPtr<SWidget> FStarRainToolkit::CreateBasicWidget()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 0, 0, 4)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("BasicTitle", "基础设置"))
			.Font(FAppStyle::GetFontStyle("BoldFont"))
		]
		// 间距
		+ SVerticalBox::Slot()
		.Padding(0, 2)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.VAlign(EVerticalAlignment::VAlign_Center)
			.FillWidth(StarRainPanelLayout::LabelWidth)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("BrushSpacingText", "物体间距"))
				.ToolTipText(LOCTEXT("BrushSpacingTip", "两个物体之间至少隔多远。数值是物体半径的倍数，越小撒得越密"))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(StarRainPanelLayout::ControlWidth)
			[
				SNew(SSpinBox<float>)
				.MinValue(0.1f)
				.MaxValue(10.0f)
				.Delta(0.1f)
				.Value(fMinDistance)
				.OnValueChanged_Lambda([this](float InValue)
				{
					fMinDistance = InValue;
					SetRandomMesh();
				})
			]
		]
		// 随机程度 —— 本次简化最关键的一个控件
		+ SVerticalBox::Slot()
		.Padding(0, 2)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.VAlign(EVerticalAlignment::VAlign_Center)
			.FillWidth(StarRainPanelLayout::LabelWidth)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("RandomnessText", "随机程度"))
				.ToolTipText(LOCTEXT("RandomnessTip",
					"往左 = 整整齐齐，往右 = 东倒西歪。\n"
					"这一个滑块同时控制位置、旋转、缩放三项随机范围。\n"
					"想单独精调，展开下面的「高级设置」。"))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(StarRainPanelLayout::ControlWidth)
			[
				SNew(SSlider)
				.MinValue(0.0f)
				.MaxValue(1.0f)
				.Value_Lambda([this]() { return GetRandomness(); })
				.OnValueChanged_Lambda([this](float InValue) { SetRandomness(InValue); })
			]
		]
		// 重力
		+ SVerticalBox::Slot()
		.Padding(0, 2, 0, 0)
		.AutoHeight()
		[
			SNew(SCheckBox)
			.IsChecked_Lambda([this]() { return IsEnableGravity() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([this](ECheckBoxState InCheck)
			{
				bEnableGravity = InCheck == ECheckBoxState::Checked;
			})
			.Content()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("EnableGravity", "让物体受重力下落"))
				.ToolTipText(LOCTEXT("EnableGravityTip", "关掉的话物体就悬在你点的地方，不会掉下来堆起来"))
			]
		];
}

/** ③ 一键预设 */
TSharedPtr<SWidget> FStarRainToolkit::CreatePresetsWidget()
{
	// 每个预设：一个按钮，点完自动把间距、随机程度、重力全设好
	auto MakePresetButton = [this](EStarRainPreset InPreset, FText InLabel, FText InTip) -> TSharedRef<SWidget>
	{
		return SNew(SButton)
			.HAlign(EHorizontalAlignment::HAlign_Center)
			.Text(InLabel)
			.ToolTipText(InTip)
			.OnClicked_Lambda([this, InPreset]()
			{
				ApplyPreset(InPreset);
				return FReply::Handled();
			});
	};

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 0, 0, 4)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("PresetsTitle", "一键预设"))
			.Font(FAppStyle::GetFontStyle("BoldFont"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(0.5f)
			.Padding(0, 0, 2, 0)
			[
				MakePresetButton(EStarRainPreset::NaturalScatter,
					LOCTEXT("PresetNatural", "自然散落"),
					LOCTEXT("PresetNaturalTip", "像落叶：朝向随机，物体之间留点间隔"))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.5f)
			.Padding(2, 0, 0, 0)
			[
				MakePresetButton(EStarRainPreset::NeatStack,
					LOCTEXT("PresetStack", "整齐堆叠"),
					LOCTEXT("PresetStackTip", "像码砖：完全不随机，只靠重力自然堆"))
			]
		]
		+ SVerticalBox::Slot()
		.Padding(0, 2, 0, 0)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(0.5f)
			.Padding(0, 0, 2, 0)
			[
				MakePresetButton(EStarRainPreset::DenseCover,
					LOCTEXT("PresetDense", "密集铺满"),
					LOCTEXT("PresetDenseTip", "草丛/碎石：挨得很近，随机大，适合铺地面"))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.5f)
			.Padding(2, 0, 0, 0)
			[
				MakePresetButton(EStarRainPreset::SparseAccent,
					LOCTEXT("PresetSparse", "稀疏点缀"),
					LOCTEXT("PresetSparseTip", "偶尔放几个，适合在场景里加少量细节"))
			]
		];
}

/** ④ 高级设置（默认折叠）—— 原面板那 14 个参数全在这 */
TSharedPtr<SWidget> FStarRainToolkit::CreateAdvancedWidget()
{
	return SNew(SVerticalBox)
		// 顺序说明
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.AutoWrapText(true)
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			.Text(LOCTEXT("AdvancedHint", "手动修改下面任意一项，会覆盖上面「随机程度」滑块的设定。"))
		]

		// 法线偏移距离
		+ SVerticalBox::Slot()
		.Padding(0, 6, 0, 2)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.VAlign(EVerticalAlignment::VAlign_Center)
			.FillWidth(StarRainPanelLayout::LabelWidth)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("NormalDistanceText", "离地高度"))
				.ToolTipText(LOCTEXT("NormalDistanceTip", "从命中点沿表面法线再抬高多少。调大一点可以让物体从高处落下，堆得更自然"))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(StarRainPanelLayout::ControlWidth)
			[
				SNew(SSpinBox<float>)
				.MinValue(0.0f)
				.MaxValue(100.0f)
				.Value(GetNormalDistance())
				.OnValueChanged_Lambda([this](float InValue)
				{
					fNormalDistance = InValue;
					SetRandomMesh();
				})
			]
		]

		// 位置随机 min
		+ SVerticalBox::Slot()
		.Padding(0, 2, 0, 0)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.VAlign(EVerticalAlignment::VAlign_Center)
			.FillWidth(StarRainPanelLayout::LabelWidth)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("MinPositionRandomText", "位置随机 最小"))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(StarRainPanelLayout::ControlWidth)
			[
				SNew(SVectorInputBox)
				.bColorAxisLabels(true)
				.AllowSpin(true)
				.X_Lambda([this]()->TOptional<float> { return MinPositionRandom.X; })
				.Y_Lambda([this]()->TOptional<float> { return MinPositionRandom.Y; })
				.Z_Lambda([this]()->TOptional<float> { return MinPositionRandom.Z; })
				.OnXChanged_Lambda([this](float InValue) { MinPositionRandom.X = InValue; SetRandomMesh(); })
				.OnYChanged_Lambda([this](float InValue) { MinPositionRandom.Y = InValue; SetRandomMesh(); })
				.OnZChanged_Lambda([this](float InValue) { MinPositionRandom.Z = InValue; SetRandomMesh(); })
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SNew(SButton)
				.OnClicked_Lambda([this]() { MinPositionRandom = FVector::ZeroVector; return FReply::Handled(); })
				.Visibility_Lambda([this]() { return MinPositionRandom.IsZero() ? EVisibility::Hidden : EVisibility::Visible; })
				.ButtonStyle(FAppStyle::Get(), "NoBorder")
				.ToolTipText(LOCTEXT("ResetToDefaultTip", "重置为默认值"))
				.Content()
				[
					SNew(SImage).Image(FAppStyle::GetBrush("PropertyWindow.DiffersFromDefault"))
				]
			]
		]

		// 位置随机 max
		+ SVerticalBox::Slot()
		.Padding(0, 0, 0, 2)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.VAlign(EVerticalAlignment::VAlign_Center)
			.FillWidth(StarRainPanelLayout::LabelWidth)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("MaxPositionRandomText", "位置随机 最大"))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(StarRainPanelLayout::ControlWidth)
			[
				SNew(SVectorInputBox)
				.bColorAxisLabels(true)
				.AllowSpin(true)
				.X_Lambda([this]()->TOptional<float> { return MaxPositionRandom.X; })
				.Y_Lambda([this]()->TOptional<float> { return MaxPositionRandom.Y; })
				.Z_Lambda([this]()->TOptional<float> { return MaxPositionRandom.Z; })
				.OnXChanged_Lambda([this](float InValue) { MaxPositionRandom.X = InValue; SetRandomMesh(); })
				.OnYChanged_Lambda([this](float InValue) { MaxPositionRandom.Y = InValue; SetRandomMesh(); })
				.OnZChanged_Lambda([this](float InValue) { MaxPositionRandom.Z = InValue; SetRandomMesh(); })
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SNew(SButton)
				.OnClicked_Lambda([this]() { MaxPositionRandom = FVector::ZeroVector; return FReply::Handled(); })
				.Visibility_Lambda([this]() { return MaxPositionRandom.IsZero() ? EVisibility::Hidden : EVisibility::Visible; })
				.ButtonStyle(FAppStyle::Get(), "NoBorder")
				.ToolTipText(LOCTEXT("ResetToDefaultTip2", "重置为默认值"))
				.Content()
				[
					SNew(SImage).Image(FAppStyle::GetBrush("PropertyWindow.DiffersFromDefault"))
				]
			]
		]

		// 旋转随机 min
		+ SVerticalBox::Slot()
		.Padding(0, 2, 0, 0)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.VAlign(EVerticalAlignment::VAlign_Center)
			.FillWidth(StarRainPanelLayout::LabelWidth)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("MinRotateRandomText", "旋转随机 最小"))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(StarRainPanelLayout::ControlWidth)
			[
				SNew(SRotatorInputBox)
				.bColorAxisLabels(true)
				.AllowSpin(true)
				.Yaw_Lambda([this]()->TOptional<float> { return MinRotateRandom.Yaw; })
				.Pitch_Lambda([this]()->TOptional<float> { return MinRotateRandom.Pitch; })
				.Roll_Lambda([this]()->TOptional<float> { return MinRotateRandom.Roll; })
				.OnYawChanged_Lambda([this](float InValue) { MinRotateRandom.Yaw = InValue; SetRandomMesh(); })
				.OnPitchChanged_Lambda([this](float InValue) { MinRotateRandom.Pitch = InValue; SetRandomMesh(); })
				.OnRollChanged_Lambda([this](float InValue) { MinRotateRandom.Roll = InValue; SetRandomMesh(); })
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SNew(SButton)
				.OnClicked_Lambda([this]() { MinRotateRandom = FRotator::ZeroRotator; return FReply::Handled(); })
				.Visibility_Lambda([this]() { return MinRotateRandom.IsZero() ? EVisibility::Hidden : EVisibility::Visible; })
				.ButtonStyle(FAppStyle::Get(), "NoBorder")
				.ToolTipText(LOCTEXT("ResetToDefaultTip3", "重置为默认值"))
				.Content()
				[
					SNew(SImage).Image(FAppStyle::GetBrush("PropertyWindow.DiffersFromDefault"))
				]
			]
		]

		// 旋转随机 max
		+ SVerticalBox::Slot()
		.Padding(0, 0, 0, 2)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.VAlign(EVerticalAlignment::VAlign_Center)
			.FillWidth(StarRainPanelLayout::LabelWidth)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("MaxRotateRandomText", "旋转随机 最大"))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(StarRainPanelLayout::ControlWidth)
			[
				SNew(SRotatorInputBox)
				.bColorAxisLabels(true)
				.AllowSpin(true)
				.Yaw_Lambda([this]()->TOptional<float> { return MaxRotateRandom.Yaw; })
				.Pitch_Lambda([this]()->TOptional<float> { return MaxRotateRandom.Pitch; })
				.Roll_Lambda([this]()->TOptional<float> { return MaxRotateRandom.Roll; })
				.OnYawChanged_Lambda([this](float InValue) { MaxRotateRandom.Yaw = InValue; SetRandomMesh(); })
				.OnPitchChanged_Lambda([this](float InValue) { MaxRotateRandom.Pitch = InValue; SetRandomMesh(); })
				.OnRollChanged_Lambda([this](float InValue) { MaxRotateRandom.Roll = InValue; SetRandomMesh(); })
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SNew(SButton)
				.OnClicked_Lambda([this]() { MaxRotateRandom = FRotator::ZeroRotator; return FReply::Handled(); })
				.Visibility_Lambda([this]() { return MaxRotateRandom.IsZero() ? EVisibility::Hidden : EVisibility::Visible; })
				.ButtonStyle(FAppStyle::Get(), "NoBorder")
				.ToolTipText(LOCTEXT("ResetToDefaultTip4", "重置为默认值"))
				.Content()
				[
					SNew(SImage).Image(FAppStyle::GetBrush("PropertyWindow.DiffersFromDefault"))
				]
			]
		]

		// 缩放随机 min
		+ SVerticalBox::Slot()
		.Padding(0, 2, 0, 0)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.VAlign(EVerticalAlignment::VAlign_Center)
			.FillWidth(StarRainPanelLayout::LabelWidth)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("MinScaleRandomText", "缩放随机 最小"))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.MaxWidth(18.0f)
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([this]() { return IsMinScaleLock() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged_Lambda([this](ECheckBoxState InState) { bMinScaleLock = InState == ECheckBoxState::Checked; })
				.ToolTipText(LOCTEXT("ScaleLockTip", "锁定：XYZ 等比缩放"))
				.Style(FAppStyle::Get(), "TransparentCheckBox")
				[
					SNew(SImage)
					.Image_Lambda([this]() { return IsMinScaleLock() ? FAppStyle::GetBrush(TEXT("Icons.Lock")) : FAppStyle::GetBrush(TEXT("Icons.Unlock")); })
					.ColorAndOpacity(FSlateColor::UseForeground())
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(StarRainPanelLayout::ControlWidth)
			[
				SNew(SVectorInputBox)
				.bColorAxisLabels(true)
				.AllowSpin(true)
				.X_Lambda([this]()->TOptional<float> { return MinScaleRandom.X; })
				.Y_Lambda([this]()->TOptional<float> { return MinScaleRandom.Y; })
				.Z_Lambda([this]()->TOptional<float> { return MinScaleRandom.Z; })
				.OnXChanged_Lambda([this](float InValue) { MinScaleRandom.X = InValue; if (IsMinScaleLock()) { MinScaleRandom.Y = InValue; MinScaleRandom.Z = InValue; } SetRandomMesh(); })
				.OnYChanged_Lambda([this](float InValue) { MinScaleRandom.Y = InValue; if (IsMinScaleLock()) { MinScaleRandom.X = InValue; MinScaleRandom.Z = InValue; } SetRandomMesh(); })
				.OnZChanged_Lambda([this](float InValue) { MinScaleRandom.Z = InValue; if (IsMinScaleLock()) { MinScaleRandom.X = InValue; MinScaleRandom.Y = InValue; } SetRandomMesh(); })
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SNew(SButton)
				.OnClicked_Lambda([this]() { MinScaleRandom = FVector::OneVector; return FReply::Handled(); })
				.Visibility_Lambda([this]() { return MinScaleRandom.X == 1 && MinScaleRandom.Y == 1 && MinScaleRandom.Z == 1 ? EVisibility::Hidden : EVisibility::Visible; })
				.ButtonStyle(FAppStyle::Get(), "NoBorder")
				.ToolTipText(LOCTEXT("ResetToDefaultTip5", "重置为默认值"))
				.Content()
				[
					SNew(SImage).Image(FAppStyle::GetBrush("PropertyWindow.DiffersFromDefault"))
				]
			]
		]

		// 缩放随机 max
		+ SVerticalBox::Slot()
		.Padding(0, 0, 0, 2)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.VAlign(EVerticalAlignment::VAlign_Center)
			.FillWidth(StarRainPanelLayout::LabelWidth)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("MaxScaleRandomText", "缩放随机 最大"))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.MaxWidth(18.0f)
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([this]() { return IsMaxScaleLock() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged_Lambda([this](ECheckBoxState InState) { bMaxScaleLock = InState == ECheckBoxState::Checked; })
				.ToolTipText(LOCTEXT("ScaleLockTip2", "锁定：XYZ 等比缩放"))
				.Style(FAppStyle::Get(), "TransparentCheckBox")
				[
					SNew(SImage)
					.Image_Lambda([this]() { return IsMaxScaleLock() ? FAppStyle::GetBrush(TEXT("Icons.Lock")) : FAppStyle::GetBrush(TEXT("Icons.Unlock")); })
					.ColorAndOpacity(FSlateColor::UseForeground())
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(StarRainPanelLayout::ControlWidth)
			[
				SNew(SVectorInputBox)
				.bColorAxisLabels(true)
				.AllowSpin(true)
				.X_Lambda([this]()->TOptional<float> { return MaxScaleRandom.X; })
				.Y_Lambda([this]()->TOptional<float> { return MaxScaleRandom.Y; })
				.Z_Lambda([this]()->TOptional<float> { return MaxScaleRandom.Z; })
				.OnXChanged_Lambda([this](float InValue) { MaxScaleRandom.X = InValue; if (IsMaxScaleLock()) { MaxScaleRandom.Y = InValue; MaxScaleRandom.Z = InValue; } SetRandomMesh(); })
				.OnYChanged_Lambda([this](float InValue) { MaxScaleRandom.Y = InValue; if (IsMaxScaleLock()) { MaxScaleRandom.X = InValue; MaxScaleRandom.Z = InValue; } SetRandomMesh(); })
				.OnZChanged_Lambda([this](float InValue) { MaxScaleRandom.Z = InValue; if (IsMaxScaleLock()) { MaxScaleRandom.X = InValue; MaxScaleRandom.Y = InValue; } SetRandomMesh(); })
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SNew(SButton)
				.OnClicked_Lambda([this]() { MaxScaleRandom = FVector::OneVector; return FReply::Handled(); })
				.Visibility_Lambda([this]() { return MaxScaleRandom.X == 1 && MaxScaleRandom.Y == 1 && MaxScaleRandom.Z == 1 ? EVisibility::Hidden : EVisibility::Visible; })
				.ButtonStyle(FAppStyle::Get(), "NoBorder")
				.ToolTipText(LOCTEXT("ResetToDefaultTip6", "重置为默认值"))
				.Content()
				[
					SNew(SImage).Image(FAppStyle::GetBrush("PropertyWindow.DiffersFromDefault"))
				]
			]
		]

		// 法线朝向额外旋转
		+ SVerticalBox::Slot()
		.Padding(0, 2, 0, 2)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.VAlign(EVerticalAlignment::VAlign_Center)
			.FillWidth(StarRainPanelLayout::LabelWidth)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ExtraNormalRotation", "贴地朝向补偿"))
				.ToolTipText(LOCTEXT("ExtraNormalRotationTip", "按住 Shift 对齐表面时，再额外偏转这个角度。比如让草歪一点"))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(StarRainPanelLayout::ControlWidth)
			[
				SNew(SRotatorInputBox)
				.bColorAxisLabels(true)
				.AllowSpin(true)
				.Yaw_Lambda([this]()->TOptional<float> { return NormalRotation.Yaw; })
				.Pitch_Lambda([this]()->TOptional<float> { return NormalRotation.Pitch; })
				.Roll_Lambda([this]()->TOptional<float> { return NormalRotation.Roll; })
				.OnYawChanged_Lambda([this](float InValue) { NormalRotation.Yaw = InValue; SetRandomMesh(); })
				.OnPitchChanged_Lambda([this](float InValue) { NormalRotation.Pitch = InValue; SetRandomMesh(); })
				.OnRollChanged_Lambda([this](float InValue) { NormalRotation.Roll = InValue; SetRandomMesh(); })
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SNew(SButton)
				.OnClicked_Lambda([this]() { NormalRotation = FRotator::ZeroRotator; return FReply::Handled(); })
				.Visibility_Lambda([this]() { return NormalRotation.IsZero() ? EVisibility::Hidden : EVisibility::Visible; })
				.ButtonStyle(FAppStyle::Get(), "NoBorder")
				.ToolTipText(LOCTEXT("ResetToDefaultTip7", "重置为默认值"))
				.Content()
				[
					SNew(SImage).Image(FAppStyle::GetBrush("PropertyWindow.DiffersFromDefault"))
				]
			]
		]

		// 权重相关
		+ SVerticalBox::Slot()
		.Padding(0, 4, 0, 0)
		.AutoHeight()
		[
			SNew(SCheckBox)
			.IsChecked_Lambda([this]() { return bIsPercentRelative ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([this](ECheckBoxState InCheck)
			{
				bIsPercentRelative = InCheck == ECheckBoxState::Checked;
				if (bIsPercentRelative && ReferenceMeshes.Num() > 1)
				{
					float Percent = 100.0f / ReferenceMeshes.Num();
					for (auto& RefMesh : ReferenceMeshes)
					{
						RefMesh.Get()->bIsAdjusting = true;
						RefMesh.Get()->Chance = Percent;
						if (auto RefSlider = Sliders.Find(RefMesh))
						{
							RefSlider->Get()->SetValue(RefMesh.Get()->Chance);
						}
						RefMesh.Get()->bIsAdjusting = false;
					}
				}
			})
			.Content()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("RelativePercents", "权重按比例联动"))
				.ToolTipText(LOCTEXT("RelativePercentsTip", "勾上后调某一种物体的权重，其他种会自动反向补偿，总和始终 100%"))
			]
		]

		// 使用选中的网格
		+ SVerticalBox::Slot()
		.Padding(0, 2, 0, 0)
		.AutoHeight()
		[
			SNew(SCheckBox)
			.IsChecked_Lambda([this]() { return IsUseSelected() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([this](ECheckBoxState InCheck)
			{
				bUseSelected = InCheck == ECheckBoxState::Checked;
			})
			.Content()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("UseSelected", "只用列表里选中的那一种"))
				.ToolTipText(LOCTEXT("UseSelectedTip", "勾上后不再按权重随机混撒，只撒你在上面列表中高亮的那一种"))
			]
		];
}

/** 摆放模式主面板 */
TSharedPtr<SWidget> FStarRainToolkit::CreatePaintModeWidget()
{
	return SNew(SBox)
	.Visibility_Lambda([this]() { return GetCurrentLayoutMode() == ELayoutMode::Paint ? EVisibility::Visible : EVisibility::Collapsed; })
	.WidthOverride(320)
	[
		SNew(SVerticalBox)

		// 顶部提示条（原来的三大行黄色警告，改成一条柔和的说明）
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 0, 0, 8)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(8)
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.ColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.80f, 1.0f)))
				.Text(LOCTEXT("PaintModeHelp",
					"按住左键在场景里拖动即可撒物体\n"
					"· 按住 Q 临时切到选择，可擦掉\n"
					"· 按住 Shift 让物体贴着地面朝向\n"
					"· 按住 Ctrl 反向（擦除）"))
			]
		]

		// ① 要放置的物体
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			CreateMeshListWidget().ToSharedRef()
		]

		// ② 基础设置
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 10, 0, 0)
		[
			CreateBasicWidget().ToSharedRef()
		]

		// ③ 一键预设
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 10, 0, 0)
		[
			CreatePresetsWidget().ToSharedRef()
		]

		// ④ 高级设置（默认折叠，功能零损失）
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 10, 0, 0)
		[
			SNew(SExpandableArea)
			.AreaTitle(LOCTEXT("AdvancedArea", "高级设置"))
			.InitiallyCollapsed(true)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.BodyContent()
			[
				CreateAdvancedWidget().ToSharedRef()
			]
		]

		// ⑤ 对已放置物体的操作
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 10, 0, 0)
		[
			CreateActionWidget().ToSharedRef()
		]
	];
}

void FStarRainToolkit::BindCommands()
{
	const TSharedRef<FUICommandList>& CommandList = GetToolkitCommands();
	const FStarRainCommands& Commands = FStarRainCommands::Get();

	// Select command action
	CommandList->MapAction(
		Commands.SelectCommand, 
		FExecuteAction::CreateLambda([this]()
		{
			ChangeMode(ELayoutMode::Select);
			LayoutModeIndex = 1;
			if (EdMode)
			{
				EdMode->UnregisterBrush();
			}
		}),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([this]()
		{
			return GetCurrentLayoutMode() == ELayoutMode::Select;
		})
	);

	// Transform command action
	CommandList->MapAction(
		Commands.TransformationCommand, 
		FExecuteAction::CreateLambda([this]()
		{
			ChangeMode(ELayoutMode::Transform);
			LayoutModeIndex = 2;
			if (EdMode)
			{
				EdMode->UnregisterBrush();
			}
		}),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([this]()
		{
			return GetCurrentLayoutMode() == ELayoutMode::Transform;
		})
	);

	// Paint command action
	CommandList->MapAction(
		Commands.PaintCommand, 
		FExecuteAction::CreateLambda([this]()
		{
			ChangeMode(ELayoutMode::Paint);
			LayoutModeIndex = 0;
			if (EdMode)
			{
				EdMode->RegisterBrush();
			}
		}),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([this]()
		{
			return GetCurrentLayoutMode() == ELayoutMode::Paint;
		})
	);
}

float FStarRainToolkit::GetMinDistance()
{
	if (GetRandomMesh())
	{
		return fMinDistance * ScaleRandom.Size() * GetRandomMesh()->GetBounds().GetSphere().W;
	}
	return fMinDistance * ScaleRandom.Size();
}

void FStarRainToolkit::SetRandomMesh()
{
	if (IsUseSelected())
	{
		if (SelectedMeshIndex >= 0)
		{
			PickedMesh = ReferenceMeshes[SelectedMeshIndex].Get()->StaticMesh;
		}
	}
	else
	{
		auto Meshes = ReferenceMeshes;
		int Rand = FMath::FRandRange(0.0f, 100.0f);

		auto PickedMeshes = Meshes.FilterByPredicate([Rand](const TSharedPtr<FReferenceMesh>& InMesh)
			{
				return (InMesh.Get()->StaticMesh && InMesh.Get()->Chance >= Rand);
			}
		);
		if (PickedMeshes.Num() == 0)
		{
			return;
		}
		PickedMesh = PickedMeshes[FMath::RandRange(0, PickedMeshes.Num() - 1)].Get()->StaticMesh;
	}

	PositionRandom = FMath::RandPointInBox(FBox(MinPositionRandom, MaxPositionRandom));
	// 原版写的是 FRotator(RandRot.X, RandRot.Y, RandRot.Z)，但
	// FRotator 的构造顺序是 (Pitch, Yaw, Roll)，而 FRotator::Euler() 返回的是 (Roll, Pitch, Yaw)，
	// 顺序不一致导致「旋转随机」里设的 Yaw 范围实际作用到了 Roll 上。
	// 这里按正确顺序还原：Euler 的 (X=Roll, Y=Pitch, Z=Yaw) -> FRotator(Pitch, Yaw, Roll)
	FVector RandRot = FMath::RandPointInBox(FBox(MinRotateRandom.Euler(), MaxRotateRandom.Euler()));
	RotateRandom = FRotator(RandRot.Y, RandRot.Z, RandRot.X);
	
	if (IsMinScaleLock() && IsMaxScaleLock())
	{
		ScaleRandom = FVector::OneVector * FMath::RandRange(MinScaleRandom.X, MaxScaleRandom.X);
	}
	else
	{
		ScaleRandom = FMath::RandPointInBox(FBox(MinScaleRandom, MaxScaleRandom));
	}
}

void FStarRainToolkit::LoadPLPreset(const FAssetData& InAsset)
{
	PLPreset = Cast<UStarRainPreset>(InAsset.GetAsset());

	UStarRainPreset* Preset = PLPreset.Get();
	if (!Preset)
	{
		return;
	}

	ReferenceMeshes.Reset();

	// 原版这里对每一项直接 StaticNesh->GetPathName() 打日志再塞进列表，
	// 预设里只要有一项没填网格（新建的空预设就是这样）就空指针崩溃。
	bool bAddedAny = false;
	for (const FPLPaintObject& PaintObject : Preset->PaintObjects)
	{
		if (!IsValid(PaintObject.StaticNesh))
		{
			continue;
		}

		TSharedPtr<FReferenceMesh> RefMesh = MakeShareable(new FReferenceMesh());
		RefMesh->StaticMesh = PaintObject.StaticNesh;
		RefMesh->Chance = PaintObject.Chance;
		ReferenceMeshes.Add(RefMesh);
		bAddedAny = true;
	}

	// 预设为空（或全是无效项）时至少留一行，否则列表会整个空掉、没法再添加
	if (!bAddedAny)
	{
		TSharedPtr<FReferenceMesh> Mesh = MakeShareable(new FReferenceMesh());
		Mesh->Chance = 100;
		ReferenceMeshes.Add(Mesh);
	}

	// ListView 要等 Init() 建完面板才存在
	if (ReferenceMeshesListView.IsValid())
	{
		ReferenceMeshesListView->RequestListRefresh();
	}
}

UStaticMesh* FStarRainToolkit::GetRandomMesh()
{
	return PickedMesh;
}

void FStarRainToolkit::ChangeMode(FString InLayoutMode)
{
	LastLayoutMode = GetCurrentLayoutMode();
	CurrentLayoutMode = InLayoutMode;
	
	if (EdMode)
	{
		EdMode->OnLayoutModeChange(InLayoutMode);
	}
}

void FStarRainToolkit::ChangeMode(int InDirection)
{
	LayoutModeIndex += InDirection;
	LayoutModeIndex = (LayoutModes.Num() + (LayoutModeIndex % LayoutModes.Num())) % LayoutModes.Num() ;
	
	if (LayoutModeIndex >= 0 && LayoutModeIndex < LayoutModes.Num())
	{
		LastLayoutMode = GetCurrentLayoutMode();
		ChangeMode(*LayoutModes[LayoutModeIndex].Get());
	}
}

/** 把合并出来的 ISM Actor 的轴心搬回物体群中心
 *
 *  引擎的 MergeComponentsToInstances 是在**世界原点** SpawnActor 的
 *  （MeshMergeUtilities.cpp:3608，SpawnActor 调用没传 Transform），
 *  再把每个实例的位置按绝对世界坐标塞进 ISM 组件。结果就是 Actor 轴心永远留在原点，
 *  离物体群十万八千里 —— 你想整体挪动/旋转这堆东西时，gizmo 出现在莫名其妙的地方。
 *
 *  这里把 Actor 挪到包围盒中心，同时把所有实例反向平移同样的量：
 *  视觉上纹丝不动，但轴心归位了，后续移动/旋转/缩放都符合直觉。
 */
static void StarRainRecenterInstancedPivot(AActor* InActor)
{
	if (!IsValid(InActor))
	{
		return;
	}

	FVector BoundsOrigin, BoundsExtent;
	InActor->GetActorBounds(/*bOnlyCollidingComponents=*/false, BoundsOrigin, BoundsExtent);

	const FVector OldLocation = InActor->GetActorLocation();
	const FVector Delta = BoundsOrigin - OldLocation;
	if (Delta.IsNearlyZero())
	{
		return;
	}

	USceneComponent* Root = InActor->GetRootComponent();

	// 合并出来的组件是 Static 的，不临时切成 Movable 就挪不动
	const EComponentMobility::Type SavedMobility =
		Root ? Root->Mobility.GetValue() : EComponentMobility::Static;
	if (Root)
	{
		Root->SetMobility(EComponentMobility::Movable);
	}

	InActor->Modify();
	InActor->SetActorLocation(BoundsOrigin, /*bSweep=*/false, nullptr, ETeleportType::TeleportPhysics);
	InActor->MarkPackageDirty();

	// 把实例反向平移，抵消掉 Actor 的位移（实例坐标是相对组件的）
	TArray<UInstancedStaticMeshComponent*> ISMComponents;
	InActor->GetComponents<UInstancedStaticMeshComponent>(ISMComponents);
	for (UInstancedStaticMeshComponent* ISM : ISMComponents)
	{
		if (!IsValid(ISM))
		{
			continue;
		}

		const int32 InstanceCount = ISM->GetInstanceCount();
		for (int32 Index = 0; Index < InstanceCount; ++Index)
		{
			FTransform InstanceTransform;
			if (ISM->GetInstanceTransform(Index, InstanceTransform, /*bWorldSpace=*/false))
			{
				InstanceTransform.SetTranslation(InstanceTransform.GetTranslation() - Delta);
				ISM->UpdateInstanceTransform(Index, InstanceTransform,
					/*bWorldSpace=*/false, /*bMarkRenderStateDirty=*/true, /*bTeleport=*/true);
			}
		}
		ISM->MarkRenderStateDirty();
	}

	if (Root)
	{
		Root->SetMobility(SavedMobility);
	}
}

void FStarRainToolkit::BakeToInstanceMesh(bool BakeSelected)
{
	if (EdMode)
	{
		TArray<UPrimitiveComponent*> SpawnedComponents;
		if (BakeSelected)
		{
			SpawnedComponents = EdMode->GetSelectedPrimitives();
		}
		else
		{
			SpawnedComponents = EdMode->GetSpawnedComponents();
		}

		if (SpawnedComponents.Num() > 0)
		{
			UWorld* World = EdMode->GetWorld();
			if (!World)
			{
				return;
			}

			// 记下合并前的所有 Actor，等会儿 diff 出引擎新建的那几个
			TSet<AActor*> ActorsBefore;
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				ActorsBefore.Add(*It);
			}

			GEditor->BeginTransaction(LOCTEXT("StarRainMode_Bake", "合并成实例化网格"));
			FMeshInstancingSettings Settings;
			const IMeshMergeUtilities& MeshUtilities = FModuleManager::Get().LoadModuleChecked<IMeshMergeModule>("MeshMergeUtilities").GetUtilities();
			MeshUtilities.MergeComponentsToInstances(SpawnedComponents, World, World->GetCurrentLevel(), Settings);
			GEditor->EndTransaction();

			// ★ 把新生成的 ISM Actor 的轴心搬回物体群中心
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				AActor* Actor = *It;
				if (!ActorsBefore.Contains(Actor))
				{
					StarRainRecenterInstancedPivot(Actor);
				}
			}
		}

		EdMode->DestroyActors(BakeSelected);
	}
}

FName FStarRainToolkit::GetToolkitFName() const
{
	return FName("StarRainEdMode");
}

FText FStarRainToolkit::GetBaseToolkitName() const
{
	return NSLOCTEXT("StarRainToolkit", "DisplayName", "星雨");
}

class FEdMode* FStarRainToolkit::GetEditorMode() const
{
	return GLevelEditorModeTools().GetActiveMode(FStarRainMode::EM_StarRainModeId);
}



#undef LOCTEXT_NAMESPACE
