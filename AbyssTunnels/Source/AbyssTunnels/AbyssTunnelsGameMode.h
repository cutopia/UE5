// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BoardGenerator.h"
#include "AbyssTunnelsGameMode.generated.h"

UCLASS(minimalapi)
class AAbyssTunnelsGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AAbyssTunnelsGameMode();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<TSubclassOf<AAbyssTunnelsCharacter>> CharacterClasses;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ABoardGenerator> BoardGenClass;

	void SpawnNewGameBoard(const int Level, const bool bParamIsAscending);
	int GetCurrentBoardLevel() const;
	virtual void StartMatch() override;
	virtual void HandleMatchHasStarted() override;

	UPROPERTY(Transient)
	TObjectPtr<ABoardGenerator> CurrentBoard;
};
