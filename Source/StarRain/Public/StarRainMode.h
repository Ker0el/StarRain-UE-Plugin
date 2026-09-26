// Copyright Saeid Gholizade. All Rights Reserved. 2020
// ---------------------------------------------------------------------------
// 本文件是 Physical Layout Tool 的修改版本。原作品作者 Saeid Gholizade，
// 采用 CC BY 4.0 许可 (https://creativecommons.org/licenses/by/4.0/)。
// 修改者：星空 (https://space.bilibili.com/177308205)
// 修改内容与完整署名见插件根目录的 NOTICE.txt
// ---------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "EdMode.h"
#include "StarRainToolkit.h"

class FViewportClient;

/** Layout mode names Enum
 *  重构说明：原来的「选择」和「笔刷选择」两个模式已合并成一个 ——
 *  单击选择、拖拽刷选本来就是同一个动作的两种手势，没必要让用户先选模式。
 *  Q 键仍然可以临时切到选择模式，从「摆放」里快速切过来擦除物体。 */
namespace ELayoutMode
{
	static const FString Select = TEXT("选择");
	static const FString Transform = TEXT("调整");
	static const FString Paint = TEXT("摆放");
}

/** Layout mode colors Enum */
namespace ELayoutModeColor
{
	static const FLinearColor Add = FLinearColor::Green;
	static const FLinearColor Remove = FLinearColor::Red;
	static const FLinearColor Select = FLinearColor::Blue;
	static const FLinearColor Deselect = FLinearColor(1.0f, 1.0f, 0.0f);
}

/** Physical Layout Edit Mode */
class FStarRainMode : public FEdMode
{
public:
	const static FEditorModeID EM_StarRainModeId;
public:
	
	/** Constructor */
	FStarRainMode();

	/** FEdMode interface */
	virtual void Enter() override;
	virtual void Exit() override;
	virtual void ActorSelectionChangeNotify() override;
	virtual void Tick(FEditorViewportClient* ViewportClient, float DeltaTime) override;
	virtual bool InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale) override;
	virtual bool MouseMove(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 x, int32 y) override;
	virtual bool StartTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport);
	virtual bool EndTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport);
	virtual bool InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event) override;
	virtual bool MouseEnter(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 x, int32 y) override;
	virtual bool ProcessCapturedMouseMoves(FEditorViewportClient* InViewportClient, FViewport* InViewport, const TArrayView<FIntPoint>& CapturedMouseMoves) override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual bool ShowModeWidgets() const;
	virtual bool UsesTransformWidget() const;
	virtual FVector GetWidgetLocation() const override;
	bool UsesToolkits() const override;

	/** Resets the selected actor's transform */
	void ResetTransform();
	
	/** Returns the spawned primitive components */
	TArray<UPrimitiveComponent*> GetSpawnedComponents();
	
	/** Returns the selected actors */
	TArray<AActor*> GetSelectedActors();
	
	
	/** Returns the spawned actors */
	TArray<AActor*> GetSpawnedActors() { 
		return SpawnedActors;
	}
	
	/** Destroys the all or selected actors */
	void DestroyActors(bool InSelected);
	
	/** Returns the selected primitive components */
	TArray<UPrimitiveComponent*> GetSelectedPrimitives();
	
	/** Unregisters the brush actor */
	void UnregisterBrush();
	
	/** Registers the brush actor */
	void RegisterBrush();
	
	/** Layout mode change event */
	void OnLayoutModeChange(FString InMode);
	
	/** Updates an actor's physics */
	void UpdatePhysics(AActor* InActor, bool bInEnableGravity);
	
	/** Makes the selected actors static */
	void MakeSelectedStatic();

	void SelectPlacedActors(UStaticMesh* InStaticMesh);

	void AddSelectedActor(AActor *InActor);
	void CachePhysics();

private:

	
	/** Brush actor's material */
	class UMaterialInstanceDynamic* BrushMI = nullptr;
	
	/** Brush actor */
	TObjectPtr<class UStaticMeshComponent> Brush = nullptr;
	
	/** Is painting enable */
	bool bIsPainting = false;
	
	/** Returns the brush actor position */
	FVector GetPosition();
	
	/** Brush actor uniform size */
	float fBrushSize = 20;
	
	/** Cursor move world direction for brush actor */
	FVector BrushDirection;
	
	/** Brush actor position */
	FVector BrushPosition;
	
	/** Brush actor rotation */
	FVector BrushNormal;
	
	/** Cursor Last world position */
	FVector BrushLastPosition;
	
	/** Brush actor color */
	FLinearColor BrushColor = FLinearColor::Blue;
	
	/** Last place actor world position */
	FVector LastSpawnedPosition;
	
	/** Cursor view position */
	FVector2D CursorPosition;
	
	/** Is Shift key down */
	bool bIsShiftDown = false;
	
	/** Is Ctrl key down */
	bool bIsCtrlDown = false;
	
	/** Is Q key down */
	bool bIsQDown = false;
	
	/** Last selected actor list */
	TArray<AActor*> LastSelectedActors;
	
	/** Last placed acotr list */
	TArray<AActor*> LastSpawnedActors;
	
	/** Placed actor list */
	TArray<AActor*> SpawnedActors;
	
	/** All map's actors */
	TArray<AActor*> LevelActors;
	
	// ★ 这五张表必须用 TWeakObjectPtr 做 key，不能用裸指针 ★
	// 它们要跨帧保存「被我改过状态的组件」，而这些组件随时可能被销毁
	// （撤销回滚、关卡切换、Actor 被删）。裸指针在对象销毁后会变成悬空指针，
	// 而 IsValid() 对悬空指针会误判为有效 —— 接着解引用就是
	// EXCEPTION_ACCESS_VIOLATION。TWeakObjectPtr 由 GC 维护，对象死了自动变 null。

	/** Physics dictionary for all map's actors */
	TMap<TWeakObjectPtr<UPrimitiveComponent>, bool> Physics;

	/** Gravity dictionary for all map's actors */
	TMap<TWeakObjectPtr<UPrimitiveComponent>, bool> Gravities;

	/** Position dictionary for all map's actors */
	TMap<TWeakObjectPtr<UPrimitiveComponent>, FVector> Positions;

	/** Rotation dictionary for all map's actors */
	TMap<TWeakObjectPtr<UPrimitiveComponent>, FRotator> Rotations;

	/** Mobility dictionary for all map's actors */
	TMap<TWeakObjectPtr<UPrimitiveComponent>, EComponentMobility::Type> Mobilities;
	
	/** Returns primitive components of an actor */
	TArray<UPrimitiveComponent*> GetPrimitives(const AActor* InActor);

	/** 剔除追踪数组里已被销毁/撤销掉的失效 Actor 指针。
	 *  UE 的撤销系统回滚时不会广播 GEngine->OnLevelActorDeleted()，
	 *  所以 SpawnedActors 等数组会残留失效指针，必须在每次使用前清理。 */
	void PruneInvalidActors();

	/** Physical layout mode toolkit Ptr */
	TSharedPtr<class FStarRainToolkit> ToolkitPtr;

	/** Updates the selected actor's physics */
	void UpdateSelectionPhysics();
	
	/** Resets the physics and transform for a primitive component
	 *  参数用弱指针 —— 传进来的组件可能已被销毁，必须在函数内部 .Get() 后再判空 */
	void ResetPrimitivePhysics(TWeakObjectPtr<UPrimitiveComponent> InPrim, bool bResetTransform, bool bForceStatic=false);
	
	/** Traces the actor under cursor */
	bool Trace(FHitResult& outHits, FEditorViewportClient *InViewportClient);
	
	/** Resets all actors physics */
	void ResetPhysics();
		
	/** On Level actors added event */
	void OnLevelActorsAdded(AActor* InActor);
	
	/** On level actors deleted event */
	void OnLevelActorsDeleted(AActor* InActor);

	/** On Pre Begin Pie event */
	void OnPreBeginPie(bool InStarted);


	bool bSimulatePhysic = true;
};
