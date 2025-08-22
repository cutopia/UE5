// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MapLevelData.generated.h"

USTRUCT(BlueprintType)
struct ABYSSTUNNELS_API FMapLevelData
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int Level = -1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Seed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsAscending = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int MinItemsPerLevel = 5;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int MaxItemsPerLevel = 20;

	TArray<int> RoomDims = { 4, 6, 8 };

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int MaxRooms = 6;
	
	// most values are derived from the level via simple calculations.
	/**
	* Allowing all mazes to be square. Nothing to be gained by giving them non matching dims.
	*/
	FORCEINLINE int GetMazeDim() const
	{
		// maze dim adjusted so the outerwalls appear in expected place when generating starting at 0,0
		return 30 + Level + (FMath::Fmod(Level, 2.f) == 0 ? 1 : 0);
	}

	FORCEINLINE float GetTerrainUnitSizeMultiplier() const
	{
		return FMath::Max(1.f,FMath::InterpEaseIn(6,1,Level*.01,1.f));
	}

	FORCEINLINE int MaximumMonsters() const
	{
		return FMath::InterpEaseIn(1,20,Level*.01,1.f);
	}

	FORCEINLINE int NumberOfItems() const
	{
		return FMath::Lerp(MinItemsPerLevel,MaxItemsPerLevel,Level * 0.01f);
	}

	FORCEINLINE void SetData(int NewLevel, int32 NewSeed, const bool bParamIsAscending)
	{
		Level = NewLevel;
		Seed = NewSeed;
		bIsAscending = bParamIsAscending;
	}
};
