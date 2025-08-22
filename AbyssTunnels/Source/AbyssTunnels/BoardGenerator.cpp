
#include "BoardGenerator.h"
#include "AbyssTunnelsGameState.h"
#include "AbyssTunnelsCharacter.h"
#include "AbyssTunnelsPlayerController.h"
#include "MapLevelData.h"
#include "Net/UnrealNetwork.h"
#include "SpawnDataAsset.h"
#include <Runtime/AIModule/Classes/AIController.h>

#include "DetailLayoutBuilder.h"
#include "SpawnTableEntry.h"
#include "Components/InstancedStaticMeshComponent.h"

ABoardGenerator::ABoardGenerator()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetNetDormancy(ENetDormancy::DORM_DormantAll);
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(Root);
	// four inner maze wall variants and then the outerwalls
	MazeWallVariant1 = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("MazeWallVariant1"));
	MazeWallVariant1->SetupAttachment(Root);
	MazeWallVariants.Add(MazeWallVariant1);
	MazeWallVariant2 = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("MazeWallVariant2"));
	MazeWallVariant2->SetupAttachment(Root);
	MazeWallVariants.Add(MazeWallVariant2);
	MazeWallVariant3 = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("MazeWallVariant3"));
	MazeWallVariant3->SetupAttachment(Root);
	MazeWallVariants.Add(MazeWallVariant3);
	MazeWallVariant4 = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("MazeWallVariant4"));
	MazeWallVariant4->SetupAttachment(Root);
	MazeWallVariants.Add(MazeWallVariant4);
	OuterMazeWalls = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("OuterMazeWalls"));
	OuterMazeWalls->SetupAttachment(Root);
}

void ABoardGenerator::BeginPlay()
{
	SetActorTickEnabled(false);
	Super::BeginPlay();
}

void ABoardGenerator::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority())
	{
		return;
	}

	const int ConnectedPlayers = GetWorld()->GetNumPlayerControllers();
	// this is responsible for spawning monsters gradually into the maze.
	if (ActiveMonsters.Num() < (ConnectedPlayers + CurrentMapLevelData.MaximumMonsters()))
	{
		SpawnRandomMonster();
	}
}

void ABoardGenerator::EndPlay(const EEndPlayReason::Type Reason)
{
	CleanupMazeContents();
	Super::EndPlay(Reason);
}

void ABoardGenerator::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABoardGenerator, CurrentMapLevelData);
}

void ABoardGenerator::OnRep_CurrentMapLevelData(FMapLevelData& OldMapLevelData)
{
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PlayerController = World->GetFirstPlayerController())
		{
			UE_LOG(LogTemp, Warning, TEXT("Replicated to %s"), *PlayerController->GetName());
		}
	}
	HandleLevelChange();
}

void ABoardGenerator::HandleLevelChange()
{
	if (AAbyssTunnelsGameState* GameState = AAbyssTunnelsGameState::Get(this))
	{
		if (GameState->bIsAscending && CurrentMapLevelData.Level == 0)
		{
			InvokeWinSequence();
			return;
		}
	}
	
	BuildNewLevel();
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &ThisClass::RespawnLocalCharacter);
	}
}

void ABoardGenerator::BuildNewLevel()
{
	// cleanup previous level
	CleanupMazeContents();
	const int Dim = CurrentMapLevelData.GetMazeDim();
	check(Dim > 3);
	Maze.Empty();
	Maze.Init(FMapUnit(), Dim * Dim);
	// these things need to stay in sync with the random stream so they are deterministic.
	RandStream = FRandomStream(CurrentMapLevelData.Seed);
	PlaceRandomRooms();
	CarveSimpleMaze(0, 0);
	GenerateMap();
	// only server spawns stuff.
	if (HasAuthority())
	{
		ShuffleSpawnLocations();
		PopulateMazeWithObjects();
		ShuffleSpawnLocations();
		SetActorTickInterval(MonsterSpawnInterval);
		SetActorTickEnabled(true);
	}
}

void ABoardGenerator::RespawnLocalCharacter()
{
	if (const UWorld* World = GetWorld())
	{
		if (AAbyssTunnelsPlayerController* PlayerController = Cast<AAbyssTunnelsPlayerController>(World->GetFirstPlayerController()))
		{
			if (PlayerController->IsLocalController())
			{
				UE_LOG(LogTemp, Warning, TEXT("Respawning %s"), *PlayerController->GetName());
				PlayerController->ServerRespawnPawnForPlayerController();
			}
		}
	}
}

void ABoardGenerator::InitializeMap(const int Level, const bool bParamIsAscending)
{
	if (HasAuthority())
	{
		RandStream.GenerateNewSeed();
		int32 Seed = RandStream.GetCurrentSeed();
		FlushNetDormancy();
		CurrentMapLevelData.SetData(Level, Seed, bParamIsAscending); // this passes seed to all clients so they can build the maze.
		HandleLevelChange();
	}
}

void ABoardGenerator::GenerateMap()
{
	int MazeDim = CurrentMapLevelData.GetMazeDim();
	// enclose the maze with some outer walls.
	PlaceWall(FVector(OuterWallMin, OuterWallMin, 0), FVector(MazeDim, OuterWallMin, 0));
	PlaceWall(FVector(OuterWallMin, MazeDim, 0), FVector(MazeDim, MazeDim, 0));
	PlaceWall(FVector(OuterWallMin, OuterWallMin, 0), FVector(OuterWallMin, MazeDim, 0));
	PlaceWall(FVector(MazeDim, OuterWallMin, 0), FVector(MazeDim, MazeDim, 0));
	const float TerrainUnitSizeMultiplier = CurrentMapLevelData.GetTerrainUnitSizeMultiplier();
	if (MazeWallVariants.Num() > 0)
	{
		// generate walls according to the maze map
		for (int i = 0; i < MazeDim; ++i)
		{
			for (int j = 0; j < MazeDim; ++j)
			{
				int Index = GetIndex(j,i);
				if (Maze[Index].bIsWall)
				{
					int WallIndex = RandStream.RandRange(0, MazeWallVariants.Num() - 1);
					const FVector Location = GetBaseOffset() + FVector(j * TerrainUnitSizeMultiplier * TerrainUnitSize, i * TerrainUnitSizeMultiplier * TerrainUnitSize, 0);
					FTransform Transform = FTransform(Location);
					const FVector TargetScale = FVector(TerrainUnitSizeMultiplier, TerrainUnitSizeMultiplier, OuterWallScaleFactor);
					Transform.SetScale3D(TargetScale);
					MazeWallVariants[WallIndex]->AddInstance(Transform);
					UE_LOG(LogTemp, Warning, TEXT("%d now has %d instances"), WallIndex, MazeWallVariants[WallIndex]->GetInstanceCount());
				}
				if (HasAuthority() && Maze[Index].bIsDoor)
				{
					const FVector Location = GetBaseOffset() + FVector(j * TerrainUnitSizeMultiplier * TerrainUnitSize, i * TerrainUnitSizeMultiplier * TerrainUnitSize, 0);
					FTransform Transform = FTransform(Location);
					Transform.SetRotation(FRotator(0,Maze[Index].DoorRotation, 0).Quaternion());
					const FVector TargetScale = FVector(DoorScale, DoorScale, DoorScale);
					if (AActor* Door = GetWorld()->SpawnActor<AActor>(DoorClass, Transform))
					{
						Door->SetActorScale3D(TargetScale);
						ActiveObjects.Add(Door);
					}
					if (AActor* DoorFrame = GetWorld()->SpawnActor<AActor>(DoorFrameClass, Transform))
					{
						DoorFrame->SetActorScale3D(TargetScale);
						ActiveObjects.Add(DoorFrame);
					}
				}
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("No maze wall instanced static meshes available!"));
	}
}

// just for the outer walls to make it so they are contiguous instead of being made if units.
void ABoardGenerator::PlaceWall(const FVector& StartPos, const FVector& EndPos)
{
	const float HorizLength = FMath::Abs(StartPos.X - EndPos.X);
	const float VertLength = FMath::Abs(StartPos.Y - EndPos.Y);
	const bool bHorizWall = HorizLength > VertLength;
	const float TerrainUnitSizeMultiplier = CurrentMapLevelData.GetTerrainUnitSizeMultiplier();
	const float WallLength = bHorizWall ? HorizLength : VertLength;
	const FVector WallDirection = (EndPos - StartPos).GetSafeNormal();
	const FVector WallCenter = StartPos + (WallDirection * WallLength * 0.5f);
	FVector TargetScale;
	if (bHorizWall)
	{
		TargetScale = FVector(FMath::Max((HorizLength + 1) * TerrainUnitSizeMultiplier, TerrainUnitSizeMultiplier), TerrainUnitSizeMultiplier, OuterWallScaleFactor);
	}
	else
	{
		TargetScale = FVector(TerrainUnitSizeMultiplier, FMath::Max((VertLength + 1) * TerrainUnitSizeMultiplier, TerrainUnitSizeMultiplier), OuterWallScaleFactor);
	}
	const FVector Location = GetBaseOffset() + (WallCenter * TerrainUnitSizeMultiplier * TerrainUnitSize) + (bHorizWall?FVector(1,1,0):FVector::ZeroVector) + FVector(0,0,(TerrainUnitSize * OuterWallScaleFactor * 0.5f));
	FTransform Transform = FTransform(Location);
	Transform.SetScale3D(TargetScale);
	OuterMazeWalls->AddInstance(Transform);
}

void ABoardGenerator::PlaceRandomRooms()
{
	const int MazeDim = CurrentMapLevelData.GetMazeDim();
	int MinX = 2;
	int MaxX = ModAdjust(MazeDim * 0.25f);
	int MinY = 2;
	int MaxY = ModAdjust(MazeDim * 0.25f);
	int LargestYAttained = 0;
	for (int i = 0; i < CurrentMapLevelData.MaxRooms; i++)
	{
		const int RoomDimIndexX = RandStream.RandRange(0, CurrentMapLevelData.RoomDims.Num() -1);
		const int RoomDimIndexY = RandStream.RandRange(0, CurrentMapLevelData.RoomDims.Num() -1);
		const int RoomDimX = CurrentMapLevelData.RoomDims[RoomDimIndexX];
		const int RoomDimY = CurrentMapLevelData.RoomDims[RoomDimIndexY];
		FVector2d TopLeftCorner = FVector2d(ModAdjust(RandStream.RandRange(MinX, MaxX)), ModAdjust(RandStream.RandRange(MinY, MaxY)));
		FVector2d BottomRightCorner = FVector2d(TopLeftCorner.X + RoomDimX, TopLeftCorner.Y + RoomDimY);
		if (BottomRightCorner.Y > LargestYAttained)
		{
			LargestYAttained = BottomRightCorner.Y;
		}
		MinX = BottomRightCorner.X + 2;
		MaxX = MinX + CurrentMapLevelData.RoomDims[CurrentMapLevelData.RoomDims.Num()-1];
		if (TopLeftCorner.X + RoomDimX > MazeDim - 1)
		{
			// time to wrap x around to next y part of map
			MinY = LargestYAttained + 2;
			MaxY = ModAdjust(MinY + CurrentMapLevelData.RoomDims[CurrentMapLevelData.RoomDims.Num()-1]);
			// reset x
			MinX = 2;
			MaxX = ModAdjust(MazeDim * 0.25f);
		}
		
		BottomRightCorner.X = FMath::Min(BottomRightCorner.X, MazeDim -1);
		BottomRightCorner.Y = FMath::Min(BottomRightCorner.Y, MazeDim -1);
		if (BottomRightCorner.X - TopLeftCorner.X < CurrentMapLevelData.RoomDims[0] || BottomRightCorner.Y - TopLeftCorner.Y < CurrentMapLevelData.RoomDims[0])
		{
			// room too smol to be valid. discard.
			continue;
		}
		
		for (int j = TopLeftCorner.X; j <= BottomRightCorner.X; j++)
		{
			for (int k = TopLeftCorner.Y; k <= BottomRightCorner.Y; k++)
			{
				int Index = GetIndex(j, k);
				Maze[Index].bIsWall = false;
				Maze[Index].bIsRoom = true;
				Maze[Index].bIsSpawn = false;
			}
		}
		
		const int RoomCenterX = TopLeftCorner.X + (BottomRightCorner.X - TopLeftCorner.X) / 2;
		const int RoomCenterY = TopLeftCorner.Y + (BottomRightCorner.Y - TopLeftCorner.Y) / 2;
		Maze[GetIndex(RoomCenterX, RoomCenterY)].bIsSpawn = true;
		// put a door in each wall.
		if (BottomRightCorner.Y < MazeDim - 1)
		{
			if (Maze[GetIndex(RoomCenterX - 1, BottomRightCorner.Y + 1)].bIsWall &&
				Maze[GetIndex(RoomCenterX + 1, BottomRightCorner.Y + 1)].bIsWall)
			{
				int Index = GetIndex(RoomCenterX, BottomRightCorner.Y + 1);
				Maze[Index].bIsDoor = true;
				Maze[Index].bIsWall = false;
				Maze[Index].DoorRotation = 90.0f;
			}
		}
		if (BottomRightCorner.X < MazeDim - 1)
		{
			if (Maze[GetIndex(BottomRightCorner.X + 1, RoomCenterY - 1)].bIsWall &&
				Maze[GetIndex(BottomRightCorner.X + 1, RoomCenterY + 1)].bIsWall)
			{
				Maze[GetIndex(BottomRightCorner.X + 1, RoomCenterY)].bIsDoor = true;
				Maze[GetIndex(BottomRightCorner.X + 1, RoomCenterY)].bIsWall = false;
			}
		}
		if (Maze[GetIndex(RoomCenterX - 1, TopLeftCorner.Y - 1)].bIsWall &&
			Maze[GetIndex(RoomCenterX + 1, TopLeftCorner.Y - 1)].bIsWall)
		{
			int Index = GetIndex(RoomCenterX, TopLeftCorner.Y - 1);
			Maze[Index].bIsDoor = true;
			Maze[Index].bIsWall = false;
			Maze[Index].DoorRotation = 90.0f;
		}
		if (Maze[GetIndex(TopLeftCorner.X - 1, RoomCenterY - 1)].bIsWall &&
			Maze[GetIndex(TopLeftCorner.X - 1, RoomCenterY + 1)].bIsWall)
		{
			Maze[GetIndex(TopLeftCorner.X - 1, RoomCenterY)].bIsDoor = true;
			Maze[GetIndex(TopLeftCorner.X - 1, RoomCenterY)].bIsWall = false;
		}
	}
}

void ABoardGenerator::CarveSimpleMaze(const int X, const int Y)
{
	Maze[GetIndex(X,Y)].bIsWall = false;
	int Direction = RandStream.RandRange(0,3);
	bool WayFound = false;
	for (int i = 0; i < 4; ++i)
	{
		switch (Direction)
		{
			case 0: // north
				if (Y-2 >= 0 && Maze[GetIndex(X,Y-2)].bIsWall)
				{
					WayFound = true;
					Maze[GetIndex(X, Y-1)].bIsWall = false;
					CarveSimpleMaze(X,Y-2);
				}
				break;
			case 1: // south
				if (Y+2 < CurrentMapLevelData.GetMazeDim() && Maze[GetIndex(X,Y+2)].bIsWall)
				{
					WayFound = true;
					Maze[GetIndex(X, Y+1)].bIsWall = false;
					CarveSimpleMaze(X,Y+2);
				}
				break;
			case 2: // east
				if (X+2 < CurrentMapLevelData.GetMazeDim() && Maze[GetIndex(X+2,Y)].bIsWall)
				{
					WayFound = true;
					Maze[GetIndex(X+1, Y)].bIsWall = false;
					CarveSimpleMaze(X+2,Y);
				}
				break;
			case 3: // west
				if (X - 2 >= 0 && Maze[GetIndex(X - 2, Y)].bIsWall)
				{
					WayFound = true;
					Maze[GetIndex(X-1, Y)].bIsWall = false;
					CarveSimpleMaze(X-2,Y);
				}
				break;
			default:
				UE_LOG(LogTemp, Error, TEXT("math is hard"));
		}
		Direction++;
		if (Direction > 3)
		{
			Direction = 0;
		}
	}
	if (!WayFound)
	{
		Maze[GetIndex(X, Y)].bIsSpawn = true;
	}
}

void ABoardGenerator::ClearMazeCenter()
{
	const int Dim = CurrentMapLevelData.GetMazeDim();
	int CenterX = Dim / 2;
	int CenterY = Dim / 2;
	Maze[GetIndex(CenterX, CenterY)].bIsWall = false;
}

bool ABoardGenerator::IsNearPlayerCharacter(const FVector& Location) const
{
	if (UWorld* World = GetWorld())
	{
		const float TerrainUnitSizeMultiplier = CurrentMapLevelData.GetTerrainUnitSizeMultiplier();
		for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			APlayerController* PlayerController = Iterator->Get();
			if (PlayerController == nullptr || PlayerController->Player == nullptr)
			{
				continue;
			}

			if (APawn* Pawn = PlayerController->GetPawn())
			{
				if (FMath::Abs(Location.X - Pawn->GetActorLocation().X) < (MinSpawnUnitsFromPlayerCharacter * TerrainUnitSize * TerrainUnitSizeMultiplier))
				{
					return true;
				}
			}
		}
	}
	return false;
}

AAbyssTunnelsCharacter* ABoardGenerator::SpawnAIFromClass(TSubclassOf<AAbyssTunnelsCharacter> PawnClass, UBehaviorTree* BehaviorTree, FVector Location, FRotator Rotation, bool bNoCollisionFail)
{
	AAbyssTunnelsCharacter* NewPawn = NULL;

	UWorld* World = GEngine->GetWorldFromContextObject(this, EGetWorldErrorMode::LogAndReturnNull);
	if (World && *PawnClass)
	{
		FActorSpawnParameters ActorSpawnParams;
		ActorSpawnParams.Owner = Owner;
		ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		NewPawn = World->SpawnActor<AAbyssTunnelsCharacter>(*PawnClass, Location, Rotation, ActorSpawnParams);

		if (NewPawn != NULL)
		{
			if (NewPawn->Controller == NULL)
			{	// NOTE: SpawnDefaultController ALSO calls Possess() to possess the pawn (if a controller is successfully spawned).
				NewPawn->SpawnDefaultController();
			}

			if (BehaviorTree != NULL)
			{
				if (AAIController* AIController = Cast<AAIController>(NewPawn->Controller))
				{
					AIController->RunBehaviorTree(BehaviorTree);
				}
			}
		}
	}

	return NewPawn;
}

int ABoardGenerator::ConvertPositionToMazeIndex(const FVector& Position)
{
	const float TerrainUnitSizeMultiplier = CurrentMapLevelData.GetTerrainUnitSizeMultiplier();
	FVector AdjustedPos = Position - GetBaseOffset();
	AdjustedPos.X /= (TerrainUnitSize * TerrainUnitSizeMultiplier);
	AdjustedPos.Y /= (TerrainUnitSize * TerrainUnitSizeMultiplier);
	return AdjustedPos.Y / CurrentMapLevelData.GetMazeDim() + AdjustedPos.X;
}

FVector ABoardGenerator::ConvertUnitsToLocation(const FVector2d& MapGridUnitsLocation) const
{
	FVector RetVector;
	const float TerrainUnitSizeMultiplier = CurrentMapLevelData.GetTerrainUnitSizeMultiplier();
	RetVector.X = MapGridUnitsLocation.X * TerrainUnitSize * TerrainUnitSizeMultiplier;
	RetVector.Y = MapGridUnitsLocation.Y * TerrainUnitSize * TerrainUnitSizeMultiplier;
	RetVector.Z = 0;
	RetVector = GetBaseOffset() + RetVector;
	return RetVector;
}

void ABoardGenerator::SpawnRandomMonster()
{
	float SpawnChance = 0.4f;
	TSubclassOf<AAbyssTunnelsCharacter> ChosenMonster = nullptr;
	// pick a monster to spawn.
	for (const auto& Entry : SpawnableMonsters)
	{
		if (Entry.Key >= CurrentMapLevelData.Level)
		{
			if (!ChosenMonster)
			{
				ChosenMonster = Entry.Value;
			}
			else if (FMath::FRand() < SpawnChance)
			{
				ChosenMonster = Entry.Value;
			}
		}
	}

	if (SpawnTopIndex >= 0)
	{
		FVector SpawnLoc = ConvertUnitsToLocation(ShuffledSpawnLocations[SpawnTopIndex]);
		SpawnTopIndex--;
		if (TObjectPtr<AAbyssTunnelsCharacter> NewMonster = SpawnAIFromClass(ChosenMonster, nullptr, SpawnLoc, FRotator::ZeroRotator))
		{
			ActiveMonsters.Add(NewMonster);
		}
	}
	else
	{
		ShuffleSpawnLocations();
	}
}

void ABoardGenerator::PopulateMazeWithObjects()
{
	if (ShuffledSpawnLocations.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("No spawn locations found!"));
		return;
	}
	TArray<FSpawnTableEntry> RoomSpawnTable;
	TArray<FSpawnTableEntry> DeadEndSpawnTable;
	for (const auto Entry : SpawnDataAsset->SpawnTable)
	{
		if (Entry.bCanSpawnInDeadEnds)
		{
			DeadEndSpawnTable.Add(Entry);
		}
		RoomSpawnTable.Add(Entry);
	}
	if (UWorld* World = GetWorld())
	{
		FVector2d SpawnLoc = ShuffledSpawnLocations[SpawnTopIndex];
		SpawnTopIndex--;
		if (ExitActorClass)
		{
			FTransform Transform = FTransform(ConvertUnitsToLocation(SpawnLoc));
			ActiveObjects.Add(World->SpawnActor<AActor>(ExitActorClass, Transform));
		}

		const int ItemsToSpawn = CurrentMapLevelData.NumberOfItems();
		int SpawnTableIndex = 0;
		while (SpawnTopIndex >= 0 && SpawnTableIndex < SpawnDataAsset->SpawnTable.Num())
		{
			FSpawnTableEntry Entry = SpawnDataAsset->SpawnTable[SpawnTableIndex];
			if (Entry.MinimumLevel <= CurrentMapLevelData.Level && Entry.MaximumLevel >= CurrentMapLevelData.Level && Entry.SpawnableObjectClass
				&& FMath::FRand() < Entry.SpawnChance)
			{
				SpawnLoc = ShuffledSpawnLocations[SpawnTopIndex];
				FTransform Transform = FTransform(ConvertUnitsToLocation(SpawnLoc));
				FRotator Rotation = FRotator(0, FMath::RandRange(0, 360), 0);
				Transform.SetRotation(Rotation.Quaternion());
				AActor* SpawnedItem = World->SpawnActor<AActor>(Entry.SpawnableObjectClass, Transform);
				ActiveObjects.Add(SpawnedItem);
			}
			else
			{
				SpawnTableIndex++;
				continue;
			}

			// we still have spaces for more items... so let's try again to place some.
			SpawnTableIndex++;
			if (SpawnTableIndex == SpawnDataAsset->SpawnTable.Num() && ActiveObjects.Num() < ItemsToSpawn)
			{
				SpawnTableIndex = 0;
			}
			
			SpawnTopIndex--;
		}
	}
}

void ABoardGenerator::CleanupMazeContents()
{
	OuterMazeWalls->ClearInstances();
	for (const auto Entry : MazeWallVariants)
	{
		Entry->ClearInstances();
	}
	if (HasAuthority())
	{
		while (ActiveMonsters.Num() > 0)
		{
			if (AAbyssTunnelsCharacter* Monster = ActiveMonsters.Pop())
			{
				Monster->Destroy();
			}
		}
		while (ActiveObjects.Num() > 0)
		{
			if (AActor* Obj = ActiveObjects.Pop())
			{
				Obj->Destroy();
			}
		}
	}
	if (AAbyssTunnelsPlayerController* PlayerController = Cast<AAbyssTunnelsPlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		PlayerController->ClearAllDecals();
	}
	ActiveMonsters.Empty();
	ActiveObjects.Empty();
}
