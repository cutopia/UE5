// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Equipment.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "AbyssTunnelsCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class AEquipment;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

class UMaterialInterface;

UCLASS(config=Game)
class AAbyssTunnelsCharacter : public ACharacter
{
	GENERATED_BODY()
public:
	// how close our target must be (dist squared!) for us to actually attack it.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Abyss")
	float AttackRange = 10000.f;

	UFUNCTION(BlueprintPure, Category="Abyss")
	float GetDistSquaredFromActiveTarget() const;

	UFUNCTION(BlueprintPure, Category="Abyss")
	bool IsInAttackRangeOfTarget() const;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss")
	TObjectPtr<AActor> AttentionTarget;
	
	UFUNCTION(BlueprintPure)
	bool GetIsAttackable();

	UFUNCTION(BlueprintPure)
	bool GetTargetableCharacters(TArray<AAbyssTunnelsCharacter*>& OutTargetableCharacters) const;

	FVector InitialLocation = FVector::ZeroVector;
	
	/**
	 * is this running thing functioning as the server/authority/solo client?
	 * (do we need to be heavy handed?)
	 * @return 
	 */
	UFUNCTION(BlueprintPure)
	bool IsOnBoss() const;

	UPROPERTY(ReplicatedUsing=OnRep_bIsSeenByPlayer, VisibleAnywhere, BlueprintReadOnly)
	bool bIsSeenByPlayer = false;

	UPROPERTY(VisibleAnywhere, Transient, BlueprintReadOnly)
	TArray<AActor*> SeenCharacters;

	UFUNCTION(BlueprintCallable, Category = "Abyss")
	void DoCameraFade(const bool bIsFadingIn);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Abyss")
	void DisplayItemPickupMessaging(const bool bDisplayMessage, AEquipment* Equipment);
protected:
	UFUNCTION()
	void OnRep_bIsSeenByPlayer();
	
	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

public:
	AAbyssTunnelsCharacter();
	
	UFUNCTION(BlueprintImplementableEvent)
	void OnMovementInputHappening();
protected:
	UFUNCTION()
	void HandleOverlap(AActor* OverlappedActor, AActor* OtherActor);

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);
	
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// To add mapping context
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	UFUNCTION(BlueprintCallable)
	void ConvertMaterialsToDynamic();
	
	UFUNCTION(BlueprintCallable)
	void UpdatePlayerSeeing();

	UFUNCTION(BlueprintCallable)
	void UpdateStealthEffectOnAllMaterials();
	
	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> DynamicMaterials;
};

