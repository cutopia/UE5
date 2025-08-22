// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpawnableObject.generated.h"

class AAbyssTunnelsPlayerController;

UCLASS()
class ABYSSTUNNELS_API ASpawnableObject : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASpawnableObject();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Abyss")
	void Interaction(AAbyssTunnelsPlayerController* PlayerController);
};
