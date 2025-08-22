// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AbyssTunnelsCharacter.h"
#include "AbyssTunnelsGameplayStatics.generated.h"

UCLASS()
class ABYSSTUNNELS_API UAbyssTunnelsGameplayStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure, Category = "AbyssTunnels")
	static bool IsCharacterFacing(const AAbyssTunnelsCharacter* Character, const AActor* Target)
	{
		const FVector CharacterForwardVector = Character->GetActorForwardVector();
		const FVector LookAtVector = Target->GetActorLocation() - Character->GetActorLocation();
		return FVector::DotProduct(LookAtVector.GetUnsafeNormal(), CharacterForwardVector) > 0.2;
	}
};
