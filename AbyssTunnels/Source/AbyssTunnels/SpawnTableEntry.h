// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SpawnableObject.h"
#include "UObject/Object.h"
#include "SpawnTableEntry.generated.h"

USTRUCT(BlueprintType)
struct ABYSSTUNNELS_API FSpawnTableEntry
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abyss")
	TSubclassOf<ASpawnableObject> SpawnableObjectClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abyss")
	float SpawnChance = 0.3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abyss")
	int MinimumLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abyss")
	int MaximumLevel = 100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abyss")
	bool bCanSpawnInDeadEnds = false;

	// only one spawns per level
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abyss")
	bool bLevelUnique = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abyss")
	bool bLevelRequired = false;
};
