// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MapLevelData.h"
#include "BoardGenerator.generated.h"

class USpawnDataAsset;
class AInstancedMeshActor;
class AAbyssTunnelsCharacter;
class UBehaviorTree;
class ASpawnableObject;

struct FMapUnit
{
	bool bIsWall = true;
	bool bIsRoom = false;
	bool bIsSpawn = false;
	bool bIsDoor = false;
	float DoorRotation = 0.0f;
};

UCLASS()
class ABYSSTUNNELS_API ABoardGenerator : public AActor
{
	GENERATED_BODY()
public:	
	ABoardGenerator();
	void HandleLevelChange();
	void BuildNewLevel();
	void PlaceWall(const FVector& StartPos, const FVector& EndPos);
	int ConvertPositionToMazeIndex(const FVector& Position);
	FVector ConvertUnitsToLocation(const FVector2d& MapGridUnitsLocation) const;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abyss")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abyss")
	TObjectPtr<UInstancedStaticMeshComponent> MazeWallVariant1;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abyss")
	TObjectPtr<UInstancedStaticMeshComponent> MazeWallVariant2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abyss")
	TObjectPtr<UInstancedStaticMeshComponent> MazeWallVariant3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abyss")
	TObjectPtr<UInstancedStaticMeshComponent> MazeWallVariant4;
	
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> MazeWallVariants;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abyss")
	TObjectPtr<UInstancedStaticMeshComponent> OuterMazeWalls;
		
	UPROPERTY(ReplicatedUsing=OnRep_CurrentMapLevelData)
	FMapLevelData CurrentMapLevelData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abyss")
	int OuterWallMin = -1;

	UFUNCTION()
	void RespawnLocalCharacter();

	UFUNCTION()
	void OnRep_CurrentMapLevelData(FMapLevelData& OldMapLevelData);

	UPROPERTY(EditAnywhere, Category = "Abyss")
	float TerrainUnitSize = 100.f;

	UPROPERTY(EditAnywhere, Category = "Abyss")
	float OuterWallScaleFactor = 20.f;

	UPROPERTY(EditAnywhere, Category = "Abyss")
	TSubclassOf<AActor> ExitActorClass;

	// prevent monsters from spawning too close to player
	UPROPERTY(EditAnywhere, Category = "Abyss")
	int MinSpawnUnitsFromPlayerCharacter = 3;

	// monsters indexed by their minimum level in which they can spawn.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abyss")
	TMap<int, TSubclassOf<AAbyssTunnelsCharacter>> SpawnableMonsters;

	UFUNCTION(BlueprintCallable, Category = "Abyss")
	void SpawnRandomMonster();

	UPROPERTY(VisibleAnywhere, Transient, Category = "Abyss")
	TArray<TObjectPtr<AAbyssTunnelsCharacter>> ActiveMonsters;

	UPROPERTY(VisibleAnywhere, Transient, Category = "Abyss")
	TArray<TObjectPtr<AActor>> ActiveObjects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss")
	float MonsterSpawnInterval = 30; // in seconds.

	UFUNCTION(BlueprintImplementableEvent, Category = "Abyss")
	void InvokeWinSequence();

	UFUNCTION()
	void InitializeMap(const int Level, const bool bParamIsAscending);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abyss")
	TObjectPtr<USpawnDataAsset> SpawnDataAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abyss")
	TSubclassOf<AActor> DoorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abyss")
	TSubclassOf<AActor> DoorFrameClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abyss")
	float DoorScale = 10.f;
protected:
	UPROPERTY(Transient)
	FRandomStream RandStream;
	
	TArray<FMapUnit> Maze;
	TArray<FVector2d> ShuffledSpawnLocations;
	int SpawnTopIndex = 0;
	void ShuffleSpawnLocations()
	{
		ShuffledSpawnLocations.Empty();
		for (int i = 0; i < Maze.Num(); i++)
		{
			if (Maze[i].bIsSpawn)
			{
				ShuffledSpawnLocations.Add(ConvertIndexToCoords(i));
			}
		}
		int ShuffleIndex = ShuffledSpawnLocations.Num()-1;
		while (ShuffleIndex >= 0)
		{
			const FVector2d Temp = ShuffledSpawnLocations[ShuffleIndex];
			const int RandomPosition = FMath::RandRange(0,ShuffledSpawnLocations.Num()-1);
			ShuffledSpawnLocations[ShuffleIndex] = ShuffledSpawnLocations[RandomPosition];
			ShuffledSpawnLocations[RandomPosition] = Temp;
			ShuffleIndex--;
		}
		SpawnTopIndex = ShuffledSpawnLocations.Num()-1;
	}

	virtual void BeginPlay() override;
	void GenerateMap();
	void CarveSimpleMaze(const int X, const int Y);
	void PlaceRandomRooms();
	void ClearMazeCenter();
	bool IsNearPlayerCharacter(const FVector& Location) const;
	void PopulateMazeWithObjects();
	void CleanupMazeContents();

	static int ModAdjust(int Val)
	{
		return (Val % 2 == 0) ? Val : (Val - 1);
	}

	FORCEINLINE int GetIndex(const int X, const int Y) const
	{
		return Y* CurrentMapLevelData.GetMazeDim() + X;
	}

	FORCEINLINE FVector2d ConvertIndexToCoords(int Index) const
	{
		return FVector2d(Index % CurrentMapLevelData.GetMazeDim(), Index / CurrentMapLevelData.GetMazeDim());
	}

	FVector GetBaseOffset() const
	{
		const float CalcMapDim = CurrentMapLevelData.GetMazeDim() * CurrentMapLevelData.GetTerrainUnitSizeMultiplier();
		return FVector(TerrainUnitSize * CalcMapDim * -0.5, TerrainUnitSize * CalcMapDim * -0.5, 0);
	}

	AAbyssTunnelsCharacter* SpawnAIFromClass(TSubclassOf<AAbyssTunnelsCharacter> PawnClass, UBehaviorTree* BehaviorTree, FVector Location, FRotator Rotation = FRotator::ZeroRotator, bool bNoCollisionFail = false);
};
