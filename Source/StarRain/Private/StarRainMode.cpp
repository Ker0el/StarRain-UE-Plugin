// Copyright Saeid Gholizade. All Rights Reserved. 2020
// ---------------------------------------------------------------------------
// 本文件是 Physical Layout Tool 的修改版本。原作品作者 Saeid Gholizade，
// 采用 CC BY 4.0 许可 (https://creativecommons.org/licenses/by/4.0/)。
// 修改者：星空 (https://space.bilibili.com/177308205)
// 修改内容与完整署名见插件根目录的 NOTICE.txt
// ---------------------------------------------------------------------------

#include "StarRainMode.h"
#include "Engine/World.h"
#include "StarRainToolkit.h"
#include "Toolkits/ToolkitManager.h"
#include "EditorModeManager.h"
#include "ToolContextInterfaces.h"
#include "Engine/Selection.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#include "Kismet/GameplayStatics.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "SceneView.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Selection.h"
#include "Chaos/PBDRigidsEvolutionFwd.h"
#include "PBDRigidsSolver.h"
#include "GeometryCollection/GeometryCollectionActor.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "Physics/Experimental/PhysScene_Chaos.h"


#define LOCTEXT_NAMESPACE "FStarRainMode"

const FEditorModeID FStarRainMode::EM_StarRainModeId = TEXT("EM_StarRainMode");

FStarRainMode::FStarRainMode()
{
	UMaterial* BrushMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EditorLandscapeResources/FoliageBrushSphereMaterial.FoliageBrushSphereMaterial"), nullptr, LOAD_None, nullptr);
	BrushMI = UMaterialInstanceDynamic::Create(BrushMaterial, GetTransientPackage());
	BrushMI->SetVectorParameterValue(TEXT("HighlightColor"), FLinearColor::Blue);
	check(BrushMI != nullptr);

	Brush = NewObject<UStaticMeshComponent>(GetTransientPackage(), TEXT("SphereBrushComponent"));
	Brush->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Brush->SetCollisionObjectType(ECC_WorldDynamic);
	Brush->SetMaterial(0, BrushMI);
	Brush->SetAbsolute(true, true, true);
	Brush->CastShadow = false;
}

void FStarRainMode::OnLevelActorsAdded(AActor* InActor)
{
	if (InActor && InActor->IsA<AStaticMeshActor>() && !InActor->IsActorBeingDestroyed())
	{
		if (InActor->GetLevel() == GetWorld()->GetCurrentLevel())
		{
			if (!SpawnedActors.Contains(InActor))
			{
				UpdatePhysics(InActor, ToolkitPtr->IsEnableGravity());
				SpawnedActors.Add(InActor);

			}
		}
	}
}

void FStarRainMode::OnLevelActorsDeleted(AActor* InActor)
{
	if (SpawnedActors.Contains(InActor))
	{
		SpawnedActors.Remove(InActor);
	}

	if (LastSpawnedActors.Contains(InActor))
	{
		LastSpawnedActors.Remove(InActor);
	}

	if (LastSelectedActors.Contains(InActor))
	{
		LastSelectedActors.Remove(InActor);
	}
	if (LevelActors.Contains(InActor))
	{
		LevelActors.Remove(InActor);
	}

	auto Prims = GetPrimitives(InActor);
	for (auto& Prim : Prims)
	{
		if (Mobilities.Contains(Prim))
		{
			Mobilities.Remove(Prim);
		}
		if (Physics.Contains(Prim))
		{
			Physics.Remove(Prim);
		}
		if (Gravities.Contains(Prim))
		{
			Gravities.Remove(Prim);
		}
		if (Positions.Contains(Prim))
		{
			Positions.Remove(Prim);
		}
		if (Rotations.Contains(Prim))
		{
			Rotations.Remove(Prim);
		}
	}
	// Physics
	// Gravities
	// Positions
	// Rotations
}

void FStarRainMode::OnPreBeginPie(bool InStarted)
{
	GetModeManager()->ActivateMode(EM_StarRainModeId, true);
}

void FStarRainMode::Enter()
{
	FEdMode::Enter();
	GEngine->OnLevelActorAdded().AddSP(this, &FStarRainMode::OnLevelActorsAdded);
	GEngine->OnLevelActorDeleted().AddSP(this, &FStarRainMode::OnLevelActorsDeleted);
	FEditorDelegates::PreBeginPIE.AddSP(this, &FStarRainMode::OnPreBeginPie);

	if (IsValid(Brush))
	{
		Brush->SetVisibility(true);
		if (!Brush->IsRegistered())
		{
			Brush->RegisterComponentWithWorld(GetWorld());
		}
	}

	if (!Toolkit.IsValid() && UsesToolkits())
	{
		ToolkitPtr = MakeShareable(new FStarRainToolkit);
		Toolkit = ToolkitPtr;
		ToolkitPtr.Get()->SetEdMode(this);
		Toolkit->Init(Owner->GetToolkitHost());
	}

	CachePhysics();
}

void FStarRainMode::Exit()
{
	GEngine->OnLevelActorAdded().RemoveAll(this);
	GEngine->OnLevelActorDeleted().RemoveAll(this);
	FEditorDelegates::PreBeginPIE.RemoveAll(this);
	GetWorld()->FinishPhysicsSim();

	ResetPhysics();
	Mobilities.Reset();
	Gravities.Reset();
	Physics.Reset();
	Positions.Reset();      // 原版漏了这两张表，退出模式后残留，跨关卡会越积越多
	Rotations.Reset();
	LevelActors.Reset();
	LastSpawnedActors.Reset();
	LastSelectedActors.Reset();
	SpawnedActors.Reset();
	Brush->UnregisterComponent();
	Brush->SetStaticMesh(nullptr);

	if (Toolkit.IsValid())
	{
		FToolkitManager::Get().CloseToolkit(Toolkit.ToSharedRef());
		Toolkit.Reset();
	}

	// Call base Exit method to ensure proper cleanup
	FEdMode::Exit();
}

void FStarRainMode::ActorSelectionChangeNotify()
{
	FEdMode::ActorSelectionChangeNotify();
	
	if (ToolkitPtr.IsValid())
	{
		if (ToolkitPtr.Get()->IsSelectingPlacedActors())
		{
			// ★ 原版这里 GetAllActorsOfClass(AActor::StaticClass()) 拿全关卡 Actor，
			//   然后对每一个调 SelectActor(false)。在普通地图上是几百次调用，
			//   在 World Partition 地图上是几千次 —— 而本函数挂在「选择变化」事件上，
			//   等于每次点选都全图扫一遍，必然卡死甚至崩。
			//   其实只需要处理「当前真正被选中」的那几个。
			TArray<AActor*> SelectedActors;
			GEditor->GetSelectedActors()->GetSelectedObjects<AActor>(SelectedActors);

			for (AActor* Actor : SelectedActors)
			{
				if (IsValid(Actor) && !SpawnedActors.Contains(Actor))
				{
					GEditor->SelectActor(Actor, false, false);
				}
			}
		}
	}

	TArray<AActor*> Actors = GetSelectedActors();
	bool SelectionChanged = false;
	for (auto& Actor : Actors)
	{
		if (!LastSelectedActors.Contains(Actor))
		{
			SelectionChanged = true;
			break;
		}
	}
	
	if (SelectionChanged)
	{
		LastSelectedActors = GetSelectedActors();
		UpdateSelectionPhysics();
	}
}

void FStarRainMode::Tick(FEditorViewportClient* ViewportClient, float DeltaTime)
{
	// 每帧剔除失效的 Actor 指针，让追踪数组自愈。
	// 撤销/重做会销毁并重建 Actor 且不触发 OnLevelActorDeleted，不清理的话
	// LevelActors / SpawnedActors 会越积越多失效指针，后面任何一次遍历都可能崩。
	PruneInvalidActors();

	if (GEditor->IsSimulateInEditorInProgress() || GEditor->IsPlaySessionInProgress()
		|| GEditor->IsPlaySessionRequestQueued())
	{
		GetModeManager()->ActivateMode(EM_StarRainModeId, true);
		return;
	}

	if (ToolkitPtr.IsValid())
	{
		FHitResult Hit;
		bool Hited = Trace(Hit, ViewportClient);

		// 选择模式同时负责「单击选择」和「拖拽刷选」——合并了原来的 PaintSelect 模式
		if (ToolkitPtr.Get()->GetCurrentLayoutMode() == ELayoutMode::Select)
		{
			if (Hited && bIsPainting)
			{
				if (ToolkitPtr.Get()->IsSelectingPlacedActors())
				{
					if (SpawnedActors.Contains(Hit.GetActor()))
					{
						GEditor->SelectActor(Hit.GetActor(), !bIsCtrlDown, true);
					}
				}
				else
				{
					GEditor->SelectActor(Hit.GetActor(), !bIsCtrlDown, true);
				}
			}
		}
		else if (ToolkitPtr.Get()->GetCurrentLayoutMode() == ELayoutMode::Paint)
		{
			if (Hited)
			{
				auto Rotation = ToolkitPtr.Get()->GetRotateRandom();
				auto PickedMesh = ToolkitPtr.Get()->GetRandomMesh();

				if (IsValid(PickedMesh))
				{
					if (IsValid(Brush))
					{
						if (!Brush->IsRegistered())
						{
							Brush->RegisterComponentWithWorld(GetWorld());
						}

						Brush->SetStaticMesh(PickedMesh);
						Brush->SetWorldLocation(GetPosition());
						Brush->SetWorldScale3D(ToolkitPtr.Get()->GetScaleRandom());

						if (bIsShiftDown && bIsCtrlDown)
						{
							Rotation = UKismetMathLibrary::FindLookAtRotation(
								BrushPosition,
								BrushPosition + BrushDirection) + ToolkitPtr.Get()->GetNormalRotation();
						}
						else if (bIsShiftDown)
						{
							Rotation = UKismetMathLibrary::FindLookAtRotation(
								BrushPosition,
								BrushPosition + BrushNormal) + ToolkitPtr.Get()->GetNormalRotation();
						}

						Brush->SetWorldRotation(Rotation);
					}
					else
					{
						Brush->SetVisibility(false);
					}

					if (bIsPainting)
					{
						if (bIsCtrlDown && !bIsShiftDown && !bIsQDown)
						{
							if (SpawnedActors.Contains(Hit.GetActor()))
							{
								GetWorld()->DestroyActor(Hit.GetActor());
							}
						}
						else
						{
							auto ReferenceMesh = ToolkitPtr.Get()->GetRandomMesh();
							if (IsValid(ReferenceMesh))
							{
								float dist = FVector::Dist(GetPosition(), LastSpawnedPosition);
								if (dist > ToolkitPtr.Get()->GetMinDistance())
								{
									LastSpawnedPosition = GetPosition();

									FActorSpawnParameters Params = FActorSpawnParameters();
									FString name = FString::Format(TEXT("Actor_{0}"), { ReferenceMesh->GetFName().ToString() });
									FName fname = MakeUniqueObjectName(nullptr, AStaticMeshActor::StaticClass(), FName(*name));
									Params.Name = fname;
									Params.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;

									AStaticMeshActor* actor = GetWorld()->SpawnActor<AStaticMeshActor>(GetPosition(), Rotation, Params);

									actor->SetActorLabel(fname.ToString());
									actor->SetActorScale3D(ToolkitPtr.Get()->GetScaleRandom());
									LastSpawnedActors.Add(actor);
									actor->GetStaticMeshComponent()->SetStaticMesh(ReferenceMesh);

									UpdatePhysics(actor, ToolkitPtr.Get()->IsEnableGravity());
									ToolkitPtr.Get()->SetRandomMesh();
								}
							}
						}

						if (IsValid(Brush))
						{
							if (!Brush->IsRegistered())
							{
								Brush->RegisterComponentWithWorld(GetWorld());
							}
							Brush->SetStaticMesh(ToolkitPtr.Get()->GetRandomMesh());
						}
					}
				}
			}
		}
		auto World = GetWorld();
		auto Solver = World->GetPhysicsScene()->GetSolver();

		if (ToolkitPtr.Get()->GetCurrentLayoutMode() == ELayoutMode::Transform ||
			ToolkitPtr.Get()->GetCurrentLayoutMode() == ELayoutMode::Paint)
		{
			TArray<AActor*> Actors = GetSelectedActors();
			// GetWorld()->GetPhysicsScene()->StartFrame();
			Solver->StartingSceneSimulation();
			if (Actors.Num() > 0)
			{
				for (int i = 0; i < Actors.Num(); ++i)
				{
					AActor* SelectedActor =  Actors[i];
					if (SelectedActor)
					{
						bool DampVelocity = ToolkitPtr->IsDamplingVelocity();
						if (!ToolkitPtr->IsDamplingVelocity())
						{
							DampVelocity = GetCurrentWidgetAxis() != EAxisList::None && GetWidgetLocation() == SelectedActor->GetActorLocation();
						}
						if (DampVelocity)
						{
							TArray<UPrimitiveComponent*> Prims = GetPrimitives(SelectedActor);
							for (auto& Prim : Prims)
							{
								Prim->SetPhysicsLinearVelocity(FVector::ZeroVector);
								Prim->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
							}
						}
					}
				}

				UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), LevelActors);
				if (LevelActors.Num() > 0)
				{
					for (int i = 0; i < LevelActors.Num(); ++i)
					{
						AActor* LevelActor = LevelActors[i];
						if (LevelActor)
						{
							if (AGeometryCollectionActor* CollectionActor = Cast<AGeometryCollectionActor>(LevelActor))
							{
								auto CollectionComponent = CollectionActor->GetGeometryCollectionComponent();
							}
							else
							{

								// World->GetPhysicsScene()->RemoveObject

								bool DampVelocity = ToolkitPtr->IsDamplingVelocity();
								if (!ToolkitPtr->IsDamplingVelocity())
								{
									DampVelocity = GetCurrentWidgetAxis() != EAxisList::None && GetWidgetLocation() == LevelActor->GetActorLocation();
								}

								if (DampVelocity)
								{
									auto Prims = GetPrimitives(LevelActor);
									for (auto& Prim : Prims)
									{
										Prim->SetPhysicsLinearVelocity(FVector::ZeroVector);
										Prim->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
									}
								}
							}
						}
					}
				}
			}
			Solver->AdvanceAndDispatch_External(DeltaTime);
			/*World->Tick(ELevelTick::LEVELTICK_All, DeltaTime);
			if (World->GetPhysicsScene()->IsCompletionEventComplete())
			{
				World->bShouldSimulatePhysics = false;
				World->FinishPhysicsSim();
			}*/
			// Solver->CompleteSceneSimulation();
			// GetWorld()->FinishPhysicsSim();

		}

		if (ToolkitPtr.Get()->GetCurrentLayoutMode() != ELayoutMode::Transform)
		{
			DrawDebugLine(GetWorld(), BrushPosition, (BrushPosition + BrushDirection), BrushColor.ToFColor(false), false, -1, 0, 5);
		}
	}
}

bool FStarRainMode::InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale)
{
	bool handled = FEdMode::InputDelta(InViewportClient, InViewport, InDrag, InRot, InScale);
	
	if (!InScale.IsZero())
	{
		auto Prims = GetSelectedPrimitives();
		for (auto& Prim : Prims)
		{
			FVector Pivot = GetWidgetLocation();
			FVector dir = (Prim->GetComponentLocation() - Pivot) * 100;
			float sign = InScale.X + InScale.Y + InScale.Z;
			Prim->AddForce(sign * dir, NAME_None, true);
		}

		handled = true;
	}

	return handled;
}

bool FStarRainMode::MouseMove(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 x, int32 y)
{
	CursorPosition.Set(x, y);
	return FEdMode::MouseMove(ViewportClient, Viewport, x, y);
}

bool FStarRainMode::StartTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport)
{
	GEditor->EndTransaction();
	GEditor->BeginTransaction(LOCTEXT("StarRainMode_Transformation", "变换"));
	
	return FEdMode::StartTracking(InViewportClient, InViewport);
}

bool FStarRainMode::EndTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport)
{
	GEditor->EndTransaction();
	GEditor->NoteSelectionChange();
	
	return 	FEdMode::EndTracking(InViewportClient, InViewport);
}

bool FStarRainMode::InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
{
	bIsShiftDown = IsShiftDown(Viewport);
	
	bool bHandled = false;
	
	if (bIsCtrlDown != IsCtrlDown(Viewport))
	{
		bIsCtrlDown = IsCtrlDown(Viewport);
	}

	if (ToolkitPtr.IsValid())
	{
		if (Key == EKeys::Tab && Event == EInputEvent::IE_Pressed)
		{
			ToolkitPtr.Get()->ChangeMode(bIsShiftDown ? -1 : 1);
			bHandled = true;
		}

		if (Key == EKeys::Q && Event == EInputEvent::IE_Pressed)
		{
			bHandled = true;
			bIsQDown = true;

			ToolkitPtr.Get()->ChangeMode(ELayoutMode::Select);
		}
		else if (Key == EKeys::Q && Event == EInputEvent::IE_Released)
		{
			bIsQDown = false;
			ToolkitPtr.Get()->ChangeMode(ToolkitPtr.Get()->GetLastLayoutMode());
		}

		if (ToolkitPtr.Get()->GetCurrentLayoutMode() == ELayoutMode::Paint)
		{
			if (!bIsCtrlDown || (bIsCtrlDown && bIsShiftDown))
			{
				BrushColor = ELayoutModeColor::Add;
				Brush->SetVisibility(true);
			}
			else
			{
				Brush->SetVisibility(false);
				BrushColor = ELayoutModeColor::Remove;

			}
		}
		else if (ToolkitPtr.Get()->GetCurrentLayoutMode() == ELayoutMode::Select)
		{
			Brush->SetVisibility(false);

			if (!bIsCtrlDown)
			{
				BrushColor = ELayoutModeColor::Select;
			}
			else
			{
				BrushColor = ELayoutModeColor::Deselect;
			}
		}
		else
		{
			Brush->SetVisibility(false);
		}

		BrushMI->SetVectorParameterValue(TEXT("HighlightColor"), BrushColor);

		if (Key == EKeys::LeftMouseButton && Event == IE_Pressed && !IsAltDown(Viewport))
		{

			if (ToolkitPtr && ToolkitPtr.Get()->GetCurrentLayoutMode() == ELayoutMode::Paint ||
				ToolkitPtr && ToolkitPtr.Get()->GetCurrentLayoutMode() == ELayoutMode::Select)
			{
				bHandled = true;
			}

			bIsPainting = true;

			if (bIsQDown)
			{
				GEditor->EndTransaction();
				GEditor->BeginTransaction(LOCTEXT("StarRainMode_Paint", "选择物体"));
			}
			else
			{
				GEditor->EndTransaction();
				GEditor->BeginTransaction(LOCTEXT("StarRainMode_Paint", "添加物体"));
			}
		}

		if (Key == EKeys::LeftMouseButton && Event == IE_Released && !IsAltDown(Viewport))
		{
			if (bIsPainting)
			{
				GEditor->EndTransaction();
			}

			bIsPainting = false;
			LastSpawnedActors.Reset();
		}
	}

	return bHandled? bHandled : FEdMode::InputKey(ViewportClient, Viewport, Key, Event);;
}

bool FStarRainMode::MouseEnter(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 x, int32 y)
{
	return FEdMode::MouseEnter(ViewportClient, Viewport, x, y);;
}

bool FStarRainMode::ProcessCapturedMouseMoves(FEditorViewportClient* InViewportClient, FViewport* InViewport, const TArrayView<FIntPoint>& CapturedMouseMoves)
{
	bool bHandled = FEdMode::ProcessCapturedMouseMoves(InViewportClient, InViewport, CapturedMouseMoves);

	for (auto& move : CapturedMouseMoves)
	{
		CursorPosition.Set(move.X, move.Y);
	}

	return bHandled;
}

bool FStarRainMode::Trace(FHitResult& outHits, FEditorViewportClient *InViewportClient)
{
	FSceneViewFamilyContext ViewContext(FSceneViewFamilyContext::ConstructionValues(
				InViewportClient->Viewport, 
				InViewportClient->GetScene(), 
				InViewportClient->EngineShowFlags)
				.SetRealtimeUpdate(InViewportClient->IsRealtime()));

	FSceneView* View = InViewportClient->CalcSceneView(&ViewContext);
	FViewportCursorLocation Cursor(View, InViewportClient, CursorPosition.X, CursorPosition.Y);

	FCollisionQueryParams Params = FCollisionQueryParams::DefaultQueryParam;
	Params.AddIgnoredActors(LastSpawnedActors);

	bool Hited = GetWorld()->LineTraceSingleByChannel(
		outHits, 
		Cursor.GetOrigin(), 
		Cursor.GetOrigin() + Cursor.GetDirection() * HALF_WORLD_MAX, 
		ECollisionChannel::ECC_Visibility, Params);

	if (Hited)
	{
		BrushLastPosition = BrushPosition;
		BrushPosition = outHits.ImpactPoint;
		BrushDirection = BrushLastPosition - BrushPosition;
		BrushNormal = outHits.ImpactNormal;
	}

	return Hited;
}

void FStarRainMode::AddReferencedObjects(FReferenceCollector& Collector)
{
	FEdMode::AddReferencedObjects(Collector);
	Collector.AddReferencedObject(Brush);
}

bool FStarRainMode::ShowModeWidgets() const
{
	if (ToolkitPtr.IsValid())
	{
		if (ToolkitPtr.Get()->GetCurrentLayoutMode() == ELayoutMode::Transform)
		{
			return true;
		}
	}

	return false;
}

bool FStarRainMode::UsesTransformWidget() const
{
	if (ToolkitPtr.IsValid())
	{
		if (ToolkitPtr.Get()->GetCurrentLayoutMode() == ELayoutMode::Transform)
		{
			return true;
		}
	}

	return false;
}

FVector FStarRainMode::GetWidgetLocation() const
{
	if (LastSelectedActors.Num() > 0 && LastSelectedActors.Last())
	{
		if (!LastSelectedActors.Last()->HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed | RF_MirroredGarbage))
		{
			return LastSelectedActors.Last()->GetActorLocation();
		}
	}
	return FEdMode::GetWidgetLocation();
}

bool FStarRainMode::UsesToolkits() const
{
	return true;
}

void FStarRainMode::RegisterBrush()
{
	if (ToolkitPtr.IsValid())
	{
		if (Brush && !Brush->IsRegistered())
		{
			Brush->RegisterComponentWithWorld(GetWorld());
			auto PickedMesh = ToolkitPtr.Get()->GetRandomMesh();

			if (IsValid(PickedMesh))
			{
				Brush->SetStaticMesh(PickedMesh);
				Brush->SetVisibility(true);
			}
		}
	}
}

void FStarRainMode::OnLayoutModeChange(FString InMode)
{
	if (InMode == ELayoutMode::Select)
	{
		bSimulatePhysic = false;
		// auto Solver = GetWorld()->GetPhysicsScene()->GetSolver();
		// Solver->CompleteSceneSimulation();

	}
	else
	{
		bSimulatePhysic = true;
	}
	if (InMode == ELayoutMode::Transform)
	{
		GEditor->NoteSelectionChange();
	}
}

FVector FStarRainMode::GetPosition()
{
	if (ToolkitPtr.IsValid())
	{
		return BrushPosition + (BrushNormal * ToolkitPtr.Get()->GetNormalDistance()) + ToolkitPtr.Get()->GetPositionRandom();
	}

	return BrushPosition;
}

TArray<UPrimitiveComponent*> FStarRainMode::GetPrimitives(const AActor* InActor)
{
	TArray<UPrimitiveComponent*> prims;

	// 原版无此检查：传入已销毁的 Actor 会在下一行直接崩
	if (!IsValid(InActor))
	{
		return prims;
	}

	TArray<UPrimitiveComponent*> Comps;
	InActor->GetComponents<UPrimitiveComponent>(Comps, false);

	for (auto Comp : Comps)
	{
		if (Comp->IsA<UPrimitiveComponent>())
		{
			UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Comp);
			if (Prim)
			{
				prims.Add(Prim);
			}
		}
	}

	return prims;
}

void FStarRainMode::PruneInvalidActors()
{
	SpawnedActors.RemoveAll([](AActor* InActor) { return !IsValid(InActor); });
	LastSpawnedActors.RemoveAll([](AActor* InActor) { return !IsValid(InActor); });
	LastSelectedActors.RemoveAll([](AActor* InActor) { return !IsValid(InActor); });
	LevelActors.RemoveAll([](AActor* InActor) { return !IsValid(InActor); });
}

void FStarRainMode::DestroyActors(bool InSelected)
{
	TArray<AActor*> Actors;

	if (InSelected)
	{
		Actors = GetSelectedActors();
	}
	else
	{
		Actors = GetSpawnedActors();
	}

	for (auto& Actor : Actors)
	{
		SpawnedActors.Remove(Actor);
		LastSpawnedActors.Remove(Actor);
		LevelActors.Remove(Actor);

		// 撤销回滚后这里的指针可能已经失效，直接 Destroy() 会崩
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}
}

TArray<UPrimitiveComponent*> FStarRainMode::GetSelectedPrimitives()
{
	TArray<UPrimitiveComponent*> Prims;
	USelection* SelectedActors = GEditor->GetSelectedActors();

	// For each selected actor
	for (FSelectionIterator Iter(*SelectedActors); Iter; ++Iter)
	{
		if (AActor* LevelActor = Cast<AActor>(*Iter))
		{

			auto comps = LevelActor->GetComponents();
			TArray<UPrimitiveComponent*> prims;

			for (auto& comp : comps)
			{
				if (!comp->IsA<UGeometryCollectionComponent>())
				{
					if (comp->IsA<UPrimitiveComponent>())
					{
						UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(comp);
						if (Prim)
						{
							Prims.Add(Prim);
						}
					}
				}
			}
		}
	}

	return Prims;
}

void FStarRainMode::UnregisterBrush()
{
	if (Brush && Brush->IsRegistered())
	{
		Brush->SetVisibility(false);
		Brush->UnregisterComponent();

	}
}

TArray<AActor*> FStarRainMode::GetSelectedActors()
{
	TArray<AActor*> Actors;
	USelection* SelectedActors = GEditor->GetSelectedActors();

	// For each selected actor
	for (FSelectionIterator Iter(*SelectedActors); Iter; ++Iter)
	{
		if (AActor* LevelActor = Cast<AActor>(*Iter))
		{
			Actors.Add(LevelActor);
		}
	}

	return Actors;
}

void FStarRainMode::UpdatePhysics(AActor* InActor, bool bInEnableGravity)
{
	auto Prims = GetPrimitives(InActor);
	for (auto& Prim : Prims)
	{
		if (!Mobilities.Contains(Prim))
		{
			Mobilities.Add(Prim, Prim->Mobility.GetValue());
		}
		if (!Physics.Contains(Prim))
		{
			Physics.Add(Prim, Prim->IsSimulatingPhysics());
		}
		if (!Gravities.Contains(Prim))
		{
			Gravities.Add(Prim, Prim->IsGravityEnabled());
		}
		if (!Positions.Contains(Prim))
		{
			Positions.Add(Prim, Prim->GetComponentLocation());
		}
		if (!Rotations.Contains(Prim))
		{
			Rotations.Add(Prim, Prim->GetComponentRotation());
		}

		Prim->SetMobility(EComponentMobility::Movable);
		Prim->SetEnableGravity(bInEnableGravity);
		Prim->SetSimulatePhysics(bSimulatePhysic);

	}
}

void FStarRainMode::MakeSelectedStatic()
{
	auto Actors = GetSelectedActors();
	for (auto& Actor : Actors)
	{
		GEditor->SelectActor(Actor, false, true);

		auto Prims = GetPrimitives(Actor);
		for (auto& Prim : Prims)
		{
			ResetPrimitivePhysics(Prim, false, true);
		}
		
		SpawnedActors.Remove(Actor);
		LastSpawnedActors.Remove(Actor);
	}
}

void FStarRainMode::SelectPlacedActors(UStaticMesh* InStaticMesh)
{
	// ★ 崩溃修复：撤销回滚不会触发 OnLevelActorsDeleted，
	//   SpawnedActors 里会残留已销毁的指针，必须先清理再遍历。
	PruneInvalidActors();

	GEditor->SelectNone(false, true, false);
	if (InStaticMesh)
	{
		auto Components = GetSpawnedComponents();
		for (auto& Component : Components)
		{
			UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(Component);
			if (IsValid(StaticMeshComponent) && StaticMeshComponent->GetStaticMesh() == InStaticMesh)
			{
				// 注意：变量不能叫 Owner —— FEdMode 从 FLegacyEdModeWidgetHelper
				// 继承了一个同名成员，会触发 C4458「声明隐藏了类成员」
				AActor* CompOwner = StaticMeshComponent->GetOwner();
				if (IsValid(CompOwner))
				{
					GEditor->SelectActor(CompOwner, true, false);
				}
			}
		}
	}
	else
	{
		for (auto& Actor : SpawnedActors)
		{
			if (IsValid(Actor))
			{
				GEditor->SelectActor(Actor, true, false);
			}
		}
	}
}

void FStarRainMode::AddSelectedActor(AActor* InActor)
{
	OnLevelActorsAdded(InActor);
}

void FStarRainMode::CachePhysics()
{
	// ★ 毁灭性缺陷修复 ★
	// 原版在这里对全关卡每一个 Actor 执行 Prim->SetMobility(EComponentMobility::Static)，
	// 指望 Exit() -> ResetPhysics() 恢复。只要编辑器崩溃/被强杀/插件抛异常导致 Exit() 没跑到，
	// 整个关卡所有 Actor 会永久卡在 Static，用户再也拖不动任何东西 —— 这就是作者自己说的
	// "sometimes it messed up your maps"，也是 Fab 上 97 条评价只有 4.0 分的根源。
	//
	// 修复思路：目的只是"别让落下的物体把关卡几何推走"，这个目的不需要动 Mobility ——
	// 非模拟状态的组件在 Chaos 里本来就是 kinematic，落体撞上去会被正常弹开。
	// 所以只关掉关卡几何自身的物理模拟，Mobility 一律不碰。
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), LevelActors);

	for (auto& actor : LevelActors)
	{
		if (!IsValid(actor) || actor->IsA<AGeometryCollectionActor>())
		{
			continue;
		}

		for (auto& Prim : GetPrimitives(actor))
		{
			if (IsValid(Prim) && Prim->IsSimulatingPhysics() && !Physics.Contains(Prim))
			{
				Physics.Add(Prim, true);
				Prim->SetSimulatePhysics(false);
			}
		}
	}
}
void FStarRainMode::UpdateSelectionPhysics()
{
	if (!bSimulatePhysic)
	{
		return;
	}

	// ★ 原版在这里调 CachePhysics()，而 CachePhysics 会 GetAllActorsOfClass 遍历全关卡。
	//   本函数挂在「选择变化」上，等于每次点选都全图扫一遍 —— World Partition 地图直接卡死。
	//   全图基线状态在 Enter() 时已经 CachePhysics() 过一遍了，这里只需处理选中的那几个。

	auto Prims = GetSelectedPrimitives();
	for (auto& Prim : Prims)
	{
		if (!Mobilities.Contains(Prim))
		{
			Mobilities.Add(Prim, Prim->Mobility.GetValue());
		}

		if (!Physics.Contains(Prim))
		{
			Physics.Add(Prim, Prim->IsSimulatingPhysics());
		}

		if (!Gravities.Contains(Prim))
		{
			Gravities.Add(Prim, Prim->IsGravityEnabled());
		}

		Prim->SetSimulatePhysics(bSimulatePhysic);
		Prim->SetEnableGravity(false);
		Prim->SetMobility(EComponentMobility::Movable);
	}
}

void FStarRainMode::ResetTransform()
{
	USelection* SelectedActors = GEditor->GetSelectedActors();

	auto Prims = GetSelectedPrimitives();
	for (auto& Prim : Prims)
	{
		ResetPrimitivePhysics(Prim, true);
	}

	SelectedActors->DeselectAll();
}

TArray<UPrimitiveComponent*> FStarRainMode::GetSpawnedComponents()
{ 
	TArray<UPrimitiveComponent*> Components;
	for (auto& Actor : SpawnedActors)
	{
		Components.Append(GetPrimitives(Actor));
	}
	return Components;
}

void FStarRainMode::ResetPhysics()
{
	// 原版只遍历 Mobilities 的键。修复 CachePhysics 后，关卡几何不再被塞进 Mobilities
	// （因为不再改它们的 Mobility），所以必须把三张表并起来，否则它们被关掉的物理永远恢复不了。
	TSet<TWeakObjectPtr<UPrimitiveComponent>> Prims;
	for (const auto& Pair : Mobilities) { Prims.Add(Pair.Key); }
	for (const auto& Pair : Physics)    { Prims.Add(Pair.Key); }
	for (const auto& Pair : Gravities)  { Prims.Add(Pair.Key); }

	for (const auto& Prim : Prims)
	{
		// 弱指针在这里救场：表里可能存着早就被销毁的组件，
		// 用裸指针遍历时 IsValid() 会误判、紧接着解引用就崩。
		if (!Prim.IsValid())
		{
			continue;
		}
		ResetPrimitivePhysics(Prim, false);
	}
}

void FStarRainMode::ResetPrimitivePhysics(TWeakObjectPtr<UPrimitiveComponent> InPrim, bool bResetTransform, bool bForceStatic)
{
	// 弱指针在这里救场：表里可能存着早就被销毁的组件
	UPrimitiveComponent* Prim = InPrim.Get();
	if (!Prim || Prim->IsBeingDestroyed()
		|| Prim->HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed | RF_MirroredGarbage)
		|| !Prim->GetFName().IsValid())
	{
		return;
	}

	if (Physics.Contains(InPrim))
	{
		Prim->SetSimulatePhysics(bForceStatic ? false : Physics[InPrim]);
	}

	if (Gravities.Contains(InPrim))
	{
		Prim->SetEnableGravity(bForceStatic ? false : Gravities[InPrim]);
	}

	if (Mobilities.Contains(InPrim))
	{
		Prim->SetMobility(bForceStatic ? EComponentMobility::Static : Mobilities[InPrim]);
	}

	if (bResetTransform)
	{
		if (Positions.Contains(InPrim))
		{
			Prim->SetWorldLocation(Positions[InPrim]);
		}

		if (Rotations.Contains(InPrim))
		{
			Prim->SetWorldRotation(Rotations[InPrim]);
		}
	}
}


#undef LOCTEXT_NAMESPACE
