// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InstancedMeshActor.generated.h"

class UInstancedStaticMeshComponent;
class USceneComponent;

USTRUCT()
struct FInstanceEntryData
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Index;

	UPROPERTY()
	FTransform InstanceTransform;
};

// a quick generic actor with an instanced static mesh component on it.
// can use it from server to tell all clients to add a new instance locally.
UCLASS()
class ABYSSTUNNELS_API AInstancedMeshActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AInstancedMeshActor();
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UInstancedStaticMeshComponent> InstancedStaticMeshComponent;

	UPROPERTY(ReplicatedUsing=OnRep_Instances);
	TArray<FInstanceEntryData> Instances;
	
	TMap<int32, int32> ServerInstanceIndexToLocalIndexInstanceMap;

	UFUNCTION()
	void OnRep_Instances();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(BlueprintCallable, Category = "Abyss")
	void AddInstance(const FTransform& NewInstanceTransform);
};
