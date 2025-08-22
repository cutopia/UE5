// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SpawnTableEntry.h"
#include "Engine/DataAsset.h"
#include "SpawnDataAsset.generated.h"

UCLASS()
class ABYSSTUNNELS_API USpawnDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abyss")
	TArray<FSpawnTableEntry> SpawnTable;
};
