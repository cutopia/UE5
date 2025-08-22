// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SpawnableObject.h"
#include "GameFramework/Actor.h"
#include "Equipment.generated.h"


class AAbyssTunnelsCharacter;

UCLASS(Blueprintable, BlueprintType)
class ABYSSTUNNELS_API AEquipment : public ASpawnableObject
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEquipment();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Abyss")
	void UseEquipment(AAbyssTunnelsCharacter* UsingCharacter, const UStaticMeshComponent* StaticMeshComponent);

	UFUNCTION(BlueprintPure, Category = "Abyss")
	FRotator GetPlaneRotation(const TArray<FVector>& PlaneSamples) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Abyss")
	FText EquipmentDisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss")
	TObjectPtr<UStaticMeshComponent> MeshComponent;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	bool GetHitAtScreenPos(const FVector2d& ScreenPosition, FVector& OutHit) const;
};
