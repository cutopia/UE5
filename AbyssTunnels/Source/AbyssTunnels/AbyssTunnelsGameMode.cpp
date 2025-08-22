// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbyssTunnelsGameMode.h"
#include "AbyssTunnelsGameState.h"
#include "AbyssTunnelsPlayerController.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameSession.h"
#include "UObject/ConstructorHelpers.h"

AAbyssTunnelsGameMode::AAbyssTunnelsGameMode()
{
	PlayerControllerClass = AAbyssTunnelsPlayerController::StaticClass();
	GameStateClass = AAbyssTunnelsGameState::StaticClass();
	static ConstructorHelpers::FClassFinder<ABoardGenerator> BoardClass(TEXT("/Game/ThirdPerson/Blueprints/BP_Board"));
	if (BoardClass.Class != NULL)
	{
		BoardGenClass = BoardClass.Class;
	}
	else
	{
		BoardGenClass = ABoardGenerator::StaticClass();
	}
}

void AAbyssTunnelsGameMode::SpawnNewGameBoard(const int Level, const bool bParamIsAscending)
{
	if (CurrentBoard)
	{
		CurrentBoard->Destroy();
		CurrentBoard = nullptr;
	}

	if (UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			APlayerController* PlayerController = Iterator->Get();
			if (PlayerController == nullptr || PlayerController->Player == nullptr)
			{
				continue;
			}

			if (APawn* Pawn = PlayerController->GetPawn())
			{
				Pawn->Destroy();
			}
		}
		UE_LOG(LogTemp, Warning, TEXT("Spawning game board from %s"), *BoardGenClass->GetName());
		if (ABoardGenerator* NewBoard = Cast<ABoardGenerator>(World->SpawnActor(BoardGenClass)))
		{
			NewBoard->InitializeMap(Level, bParamIsAscending);
			CurrentBoard = NewBoard;
		}
	}
}

int AAbyssTunnelsGameMode::GetCurrentBoardLevel() const
{
	if (CurrentBoard)
	{
		return CurrentBoard->CurrentMapLevelData.Level;
	}
	UE_LOG(LogTemp, Error, TEXT("Falling back to default board level."));
	return 1;
}

void AAbyssTunnelsGameMode::StartMatch()
{
	Super::StartMatch();
	SpawnNewGameBoard(1, false);
}

void AAbyssTunnelsGameMode::HandleMatchHasStarted()
{
	GameSession->HandleMatchHasStarted();
	// Make sure level streaming is up to date before triggering NotifyMatchStarted
	GEngine->BlockTillLevelStreamingCompleted(GetWorld());

	// First fire BeginPlay, if we haven't already in waiting to start match
	GetWorldSettings()->NotifyBeginPlay();

	// Then fire off match started
	GetWorldSettings()->NotifyMatchStarted();
	if (IsHandlingReplays() && GetGameInstance() != nullptr)
	{
		GetGameInstance()->StartRecordingReplay(GetWorld()->GetMapName(), GetWorld()->GetMapName());
	}
}
