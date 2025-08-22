// Fill out your copyright notice in the Description page of Project Settings.


#include "AbyssTunnelsPlayerController.h"
#include "AbyssTunnelsGameMode.h"
#include "AbyssTunnelsGameState.h"
#include "Components/DecalComponent.h"
#include "Kismet/GameplayStatics.h"

void AAbyssTunnelsPlayerController::BeginPlay()
{
	Super::BeginPlay();
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	SetShowMouseCursor(false);
}

APawn* AAbyssTunnelsPlayerController::SpawnChosenPawn(AAbyssTunnelsGameMode* GameMode, const FTransform& SpawnTransform)
{
	if (!GameMode)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnChosenPawn: GameMode is null"));
		return nullptr;
	}
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = GetInstigator();
	SpawnInfo.ObjectFlags |= RF_Transient;	// We never want to save default player pawns into a map
	int Index = 0;
	if (AAbyssTunnelsGameState* GameState = AAbyssTunnelsGameState::Get(this))
	{
		Index = GameState->PlayerArray.IndexOfByKey(PlayerState);
		if (Index > 0 && Index >= GameMode->CharacterClasses.Num())
		{
			Index = (Index % GameMode->CharacterClasses.Num()) - 1;
		}
		if (Index < 0)
		{
			Index = 0;
		}
	}
	UClass* PawnClass = GameMode->CharacterClasses[Index];
	APawn* ResultPawn = GetWorld()->SpawnActor<APawn>(PawnClass, SpawnTransform, SpawnInfo);
	if (!ResultPawn)
	{
		UE_LOG(LogGameMode, Warning, TEXT("SpawnDefaultPawnAtTransform: Couldn't spawn Pawn of type %s at %s"), *GetNameSafe(PawnClass), *SpawnTransform.ToHumanReadableString());
	}
	return ResultPawn;
}

AAbyssTunnelsCharacter* AAbyssTunnelsPlayerController::GetCharacter()
{
	return Cast<AAbyssTunnelsCharacter>(GetPawn());
}

void AAbyssTunnelsPlayerController::MulticastSpawnDecalOnAllClients_Implementation(UMaterialInterface* DecalMaterial, const FVector& DecalSize,
                                                                                   const FVector& InstanceLocation, const FRotator& InstanceRotation)
{
	if (UDecalComponent* Decal = UGameplayStatics::SpawnDecalAtLocation(this, DecalMaterial, DecalSize, InstanceLocation, InstanceRotation))
	{
		DecalComponents.Add(Decal);
	}
}

void AAbyssTunnelsPlayerController::ClearAllDecals()
{
	for (auto* Decal : DecalComponents)
	{
		Decal->DestroyComponent();
	}
	DecalComponents.Empty();
}

void AAbyssTunnelsPlayerController::PickupItem(AEquipment* Equipment)
{
	if (Equipment && Equipments.Num() < MaxEquipmentSlots)
	{
		AttachEquipmentToPlayer(Equipment, Equipments.Num() == 0);  // first item we pick up auto activates
		ServerPickupItem(Equipment, Equipments.Num() == 0); // first item we pick up auto activates
		Equipments.Add(Equipment);
	}
}

void AAbyssTunnelsPlayerController::AttachEquipmentToPlayer(AEquipment* Equipment, const bool bIsActiveItem)
{
	if (Equipment)
	{
		if (AAbyssTunnelsCharacter* Avatar = GetCharacter())
		{
			TArray<UActorComponent*> Components = Avatar->GetComponentsByTag(USkeletalMeshComponent::StaticClass(), FName(TEXT("body")));
			if (Components.Num() > 0)
			{
				if (Equipment->GetParentActor() && Equipment->GetParentActor() != Avatar)
				{
					UE_LOG(LogTemp, Error, TEXT("Tried to pickup something that was already picked up!"))
					return;
				}
				if (HasAuthority())
				{
					Equipment->FlushNetDormancy();
					Equipment->SetOwner(Avatar->GetOwner());
				}
				Equipment->MeshComponent->SetSimulatePhysics(false);
				Equipment->SetActorEnableCollision(false);
				if (TObjectPtr<USkeletalMeshComponent> SkellyMesh = Cast<USkeletalMeshComponent>(Components[0]))
				{
					const auto SocketNames = SkellyMesh->GetAllSocketNames();
					for (const FName& Name : SocketNames)
					{
						UE_LOG(LogTemp, Warning, TEXT("Socket: %s"), *Name.ToString());
					}
					if (Equipment->AttachToComponent(SkellyMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("equipmentsocket")))
					{
						UE_LOG(LogTemp, Warning, TEXT("Attachment successful"));
					}
					else
					{
						UE_LOG(LogTemp, Error, TEXT("Attachment failed"));
					}
				}
				Equipment->SetActorHiddenInGame(false);
			}
		}
	}
}

void AAbyssTunnelsPlayerController::ServerPickupItem_Implementation(AEquipment* Equipment, const bool bIsActiveItem)
{
	AttachEquipmentToPlayer(Equipment, bIsActiveItem);
	MulticastAttachEquipmentToPlayer(Equipment, bIsActiveItem);
}

void AAbyssTunnelsPlayerController::MulticastAttachEquipmentToPlayer_Implementation(AEquipment* Equipment, const bool bIsActiveItem)
{
	AttachEquipmentToPlayer(Equipment, bIsActiveItem);
}

void AAbyssTunnelsPlayerController::DropActiveItem()
{
	if (Equipments.Num() > 0)
	{
		if (ActiveEquipmentIndex >= Equipments.Num())
		{
			ActiveEquipmentIndex = 0;
		}
		if (auto* Equipment = Equipments[ActiveEquipmentIndex])
		{
			Equipments.RemoveAt(ActiveEquipmentIndex);
			ServerDropItem(Equipment);
		}
		if (ActiveEquipmentIndex >= Equipments.Num())
		{
			ActiveEquipmentIndex = 0;
		}
		if (Equipments.Num() > 0)
		{
			ServerPickupItem(Equipments[ActiveEquipmentIndex], true);
		}
	}
}

void AAbyssTunnelsPlayerController::UseActiveItem()
{
	if (Equipments.Num() > 0)
	{
		if (ActiveEquipmentIndex >= Equipments.Num())
		{
			ActiveEquipmentIndex = 0;
		}
		if (auto* Equipment = Equipments[ActiveEquipmentIndex])
		{
			Equipment->UseEquipment(GetCharacter(), Equipment->MeshComponent);
		}
	}
}

void AAbyssTunnelsPlayerController::ServerInteractWithSpawnableObject_Implementation(ASpawnableObject* SpawnableObject)
{
	if (SpawnableObject)
	{
		SpawnableObject->Interaction(this);
	}
}

void AAbyssTunnelsPlayerController::ServerDropItem_Implementation(AEquipment* Equipment)
{
	if (Equipment)
	{
		Equipment->FlushNetDormancy();
		Equipment->MeshComponent->SetSimulatePhysics(false);
		Equipment->SetActorEnableCollision(true);
		Equipment->DetachFromActor(FDetachmentTransformRules::KeepRelativeTransform);
		Equipment->SetActorHiddenInGame(false);
	}
}

void AAbyssTunnelsPlayerController::ServerSpawnDecalForAllClients_Implementation(UMaterialInterface* DecalMaterial, const FVector& DecalSize, const FVector& InstanceLocation, const FRotator& InstanceRotation)
{
	MulticastSpawnDecalOnAllClients(DecalMaterial, DecalSize, InstanceLocation, InstanceRotation);
}

void AAbyssTunnelsPlayerController::ServerRespawnPawnForPlayerController_Implementation()
{
	if (UWorld* World = GetWorld())
	{
		if (AAbyssTunnelsGameMode* GameMode = Cast<AAbyssTunnelsGameMode>(World->GetAuthGameMode()))
		{
			if (GameMode->CharacterClasses.Num() == 0)
			{
				UE_LOG(LogTemp, Error, TEXT("No character classes available!"));
				return;
			}
			FVector StartLoc = FVector(0, 0, 100.f);
			if (AActor* PlayerStart = GameMode->FindPlayerStart(this))
			{
				StartLoc = PlayerStart->GetActorLocation();
				StartLoc.Z = 1000.f;
			}
			if (APawn* NewPawn = SpawnChosenPawn(GameMode, FTransform(StartLoc)))
			{
				Possess(NewPawn);
				// reequip the player's stuff.
				if (AAbyssTunnelsCharacter* AbyssCharacter = Cast<AAbyssTunnelsCharacter>(NewPawn))
				{
					if (Equipments.Num() > 0)
					{
						ServerPickupItem_Implementation(Equipments[ActiveEquipmentIndex], true);
					}
				}
			}
		}
	}
}

ASpawnableObject* AAbyssTunnelsPlayerController::GetSpawnableObjectAtScreenCenter() const
{
	if (UWorld* World = GetWorld())
	{
		if ( UGameViewportClient* ViewportClient = World->GetGameViewport())
		{
			FVector2d ViewportSize;
			ViewportClient->GetViewportSize(ViewportSize);
			ViewportSize.X *= 0.5f;
			ViewportSize.Y *= 0.5f;
			FVector WorldLoc, WorldDir;
			if (DeprojectScreenPositionToWorld(ViewportSize.X, ViewportSize.Y, WorldLoc, WorldDir))
			{
				FHitResult Hit;
				FVector TraceEnd = WorldLoc + (WorldDir * 3000.f);
				// if we trace to center successfully after that we will take some additional samples to determine geometric plane.
				FCollisionObjectQueryParams Params(FCollisionObjectQueryParams::AllDynamicObjects);
				FCollisionQueryParams QueryParams;
				QueryParams.AddIgnoredActors(Characters);
				if (World->LineTraceSingleByObjectType(Hit, WorldLoc, TraceEnd, Params, QueryParams))
				{
					FString Nom = Hit.GetActor()->GetName();
					UE_LOG(LogTemp, Warning, TEXT("%s"), *Nom);
					return Cast<ASpawnableObject>(Hit.GetActor());
				}
			}
		}
	}
	return nullptr;
}
