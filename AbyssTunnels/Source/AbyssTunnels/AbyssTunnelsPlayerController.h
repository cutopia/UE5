// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AbyssTunnelsPlayerController.generated.h"

class AAbyssTunnelsGameMode;

UCLASS()
class ABYSSTUNNELS_API AAbyssTunnelsPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	virtual void BeginPlay() override;
	APawn* SpawnChosenPawn(AAbyssTunnelsGameMode* GameMode, const FTransform& SpawnTransform);
	
	UFUNCTION(BlueprintPure, Category = "Abyss")
	ASpawnableObject* GetSpawnableObjectAtScreenCenter() const;

	UFUNCTION(BlueprintPure, Category = "Abyss")
	AAbyssTunnelsCharacter* GetCharacter();

	UFUNCTION(BlueprintCallable, Category = "Abyss")
	void PickupItem(AEquipment* Equipment);

	void AttachEquipmentToPlayer(AEquipment* Equipment, const bool bIsActiveItem);
	
	UFUNCTION(Server, Reliable, Category = "Abyss")
	void ServerPickupItem(AEquipment* Equipment, const bool bIsActiveItem);

	UFUNCTION(NetMulticast, Reliable, Category = "Abyss")
	void MulticastAttachEquipmentToPlayer(AEquipment* Equipment, const bool bIsActiveItem);

	UFUNCTION(BlueprintCallable, Category = "Abyss")
	void DropActiveItem();

	UFUNCTION(Server, Reliable, Category = "Abyss")
	void ServerDropItem(AEquipment* Equipment);

	UFUNCTION(BlueprintCallable, Category = "Abyss")
	void UseActiveItem();
	
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Abyss")
	void ServerRespawnPawnForPlayerController();

	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Abyss")
	void ServerSpawnDecalForAllClients(UMaterialInterface* DecalMaterial, const FVector& DecalSize, const FVector& InstanceLocation, const FRotator& InstanceRotation);

	UFUNCTION(NetMulticast, Reliable, Category = "Abyss")
	void MulticastSpawnDecalOnAllClients(UMaterialInterface* DecalMaterial, const FVector& DecalSize, const FVector& InstanceLocation, const FRotator& InstanceRotation);

	// spawnable object's interaction method is called when user tries to interact with one of these items.
	// they are NOT liftable/equippable, but are larger objects sitting around in the world.
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Abyss")
	void ServerInteractWithSpawnableObject(ASpawnableObject* SpawnableObject);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss")
	TArray<AEquipment*> Equipments;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss")
	int ActiveEquipmentIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss")
	int MaxEquipmentSlots = 2;
	
	void ClearAllDecals();
	
	UPROPERTY(Transient)
	TArray<UDecalComponent*> DecalComponents;

	UPROPERTY(Transient)
	TArray<AActor*> Characters;
};
