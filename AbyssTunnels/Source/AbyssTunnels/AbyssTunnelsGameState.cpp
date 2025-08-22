// Fill out your copyright notice in the Description page of Project Settings.


#include "AbyssTunnelsGameState.h"
#include "AbyssTunnelsGameMode.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"


void AAbyssTunnelsGameState::BeginPlay()
{
	Super::BeginPlay();
}

void AAbyssTunnelsGameState::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AAbyssTunnelsGameState, bIsAscending);
}

void AAbyssTunnelsGameState::ServerChangeLevelDueToExitReached_Implementation()
{
	switch (GetNetMode())
	{
	case ENetMode::NM_DedicatedServer:
	case ENetMode::NM_ListenServer:
	case ENetMode::NM_Standalone:
		if (UWorld* World = GetWorld())
		{
			if (AAbyssTunnelsGameMode* GameMode = Cast<AAbyssTunnelsGameMode>(World->GetAuthGameMode()))
			{
				int SignMult = bIsAscending ? -1 : 1;
				const int CurrentLevel = GameMode->GetCurrentBoardLevel();
				int NextLevel = CurrentLevel + (1 * SignMult);
				GameMode->SpawnNewGameBoard(NextLevel, bIsAscending);
				
			}
		}
		break;
	default:
		break;
	}
}

AAbyssTunnelsGameState* AAbyssTunnelsGameState::Get(const UObject* WorldContextObject)
{
	UWorld* const World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	return World ? Cast<AAbyssTunnelsGameState>(World->GetGameState()) : nullptr;
}
