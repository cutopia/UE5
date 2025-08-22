// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "BoardGenerator.h"
#include "AbyssTunnelsCharacter.h"
#include "AbyssTunnelsGameState.generated.h"

class APlayerController;

UCLASS()
class ABYSSTUNNELS_API AAbyssTunnelsGameState : public AGameState
{
	GENERATED_BODY()
public:
	virtual void BeginPlay() override;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite)
	bool bIsAscending = false;

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void ServerChangeLevelDueToExitReached();

	UFUNCTION(BlueprintPure, Category = "Game", meta = (WorldContext = "WorldContextObject"))
	static AAbyssTunnelsGameState* Get(const UObject* WorldContextObject);

};
