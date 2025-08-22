// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GrowingActor.generated.h"

class UInstancedStaticMeshComponent;
class USceneComponent;

UCLASS()
class PLANTGROWING_API AGrowingActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AGrowingActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UInstancedStaticMeshComponent* Stalk;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UInstancedStaticMeshComponent* Leaves;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parameters")
	float MinScaleFactor = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parameters")
	bool SpecifyParameters = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parameters", meta=(EditCondition="SpecifyParameters"))
	float ScaleStep = 0.1f;
	
	/**
	 * 1.0 -- it is completely apically dominant like a sunflower. stalk with only a leaf at each node. No branching unless pinched off by human.
	 * 0.0 -- it grows evenly out of every node.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parameters", meta=(EditCondition="SpecifyParameters"))
	float ApicalDominance = 0.6f;

	/**
	 * in botany, leaves on plants are typically arranged either opposite or alternating.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parameters", meta=(EditCondition="SpecifyParameters"))
	bool bNodesOpposite = true;

	/**
	 * how much the plant should grow before stopping.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parameters", meta=(EditCondition="SpecifyParameters"))
	int Iterations = 100;

	/**
	 * Simulates leaves falling off as the plant ages.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parameters", meta=(EditCondition="SpecifyParameters"))
	int LeafLifespan = 100;

	/**
	 * This allows the rotation of the leaves to be customized to suit the orientation of the provided leaf mesh.
	 * They will be further rotated based on where they are being placed.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parameters")
	FRotator LeafRotationOrientation = FRotator();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parameters", meta=(EditCondition="SpecifyParameters"))
	float GrowthTwist = 90.0f;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void Grow(const FVector& StartingPoint);
	void PlaceLeavesAtNode(const FTransform& StartingTransform, const FVector& Extents);
	TArray<FTransform> GrowFromNode(const FTransform& StartingTransform, const bool bPlaceLeaves, const bool bPlaceSideShoots);
	FTransform PlaceShoot(const FTransform& PlacementTransform, const bool bApplyApicalDominance, FVector& OutExtents) const;
private:
	TArray<FTransform> EndPoints;
};
