// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TriggerSpace.generated.h"

UCLASS()
class ABYSSTUNNELS_API ATriggerSpace : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATriggerSpace();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bRequireAllCharactersPresent = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> StaticMeshComponent;

	UFUNCTION()
	void OnCharacterOverlapped(AAbyssTunnelsCharacter* Character);

	UFUNCTION(BlueprintImplementableEvent)
	void TriggerSpaceTriggered(AAbyssTunnelsCharacter* TriggeringCharacter);

	UFUNCTION(BlueprintPure)
	bool AreAllCharactersPresent() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float GroupTriggerDistance = 200.f;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
};
