#include "TriggerSpace.h"
#include "AbyssTunnelsCharacter.h"
#include "AbyssTunnelsGameState.h"


// Sets default values
ATriggerSpace::ATriggerSpace()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = false;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(Root);
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TriggeringStaticMesh"));
	StaticMeshComponent->SetupAttachment(Root);
}

void ATriggerSpace::OnCharacterOverlapped(AAbyssTunnelsCharacter* Character)
{
	if (bRequireAllCharactersPresent && !AreAllCharactersPresent())
	{
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("Character overlapped triggerspace--%s"), *Character->GetName());
	TriggerSpaceTriggered(Character);
}

bool ATriggerSpace::AreAllCharactersPresent() const
{
	if (UWorld* World = GetWorld())
	{	
		int PlayersCounted = 0;
		for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			APlayerController* PlayerController = Iterator->Get();
			if (PlayerController == nullptr || PlayerController->Player == nullptr)
			{
				continue;
			}

			if (APawn* Pawn = PlayerController->GetPawn())
			{
				PlayersCounted++;
				if (FVector::Distance(Pawn->GetActorLocation(), this->GetActorLocation()) > GroupTriggerDistance)
				{
					return false;
				}
			}
		}
		return PlayersCounted > 0;
	}
	return false;
}

// Called when the game starts or when spawned
void ATriggerSpace::BeginPlay()
{
	Super::BeginPlay();
	// adjust location of the static mesh representation of the space so it should be lying right on the floor and not below it.
	FVector Min, Max;
	StaticMeshComponent->GetLocalBounds(Min, Max);
	float ZLoc = Max.Z - Min.Z;
	FVector WorldLoc = GetActorLocation();
	WorldLoc.Z += ZLoc;
	StaticMeshComponent->SetWorldLocation(WorldLoc);
}

