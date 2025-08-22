// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbyssTunnelsCharacter.h"

#include "AbyssTunnelsGameplayStatics.h"
#include "AbyssTunnelsPlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "TriggerSpace.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

float AAbyssTunnelsCharacter::GetDistSquaredFromActiveTarget() const
{
	if (AttentionTarget)
	{
		return FVector::DistSquared2D(AttentionTarget->GetActorLocation(), GetActorLocation());
	}

	return 1000000.f * 1000000.f; // some very large number. meh.
}

bool AAbyssTunnelsCharacter::IsInAttackRangeOfTarget() const
{
	 return GetDistSquaredFromActiveTarget() <= AttackRange;
}

bool AAbyssTunnelsCharacter::GetIsAttackable()
{
	if (!IsPlayerControlled())
	{
		return false;
	}
	FVector CurrentLoc = GetActorLocation();
	if (FMath::IsNearlyEqual(InitialLocation.X, CurrentLoc.X) && FMath::IsNearlyEqual(InitialLocation.Y, CurrentLoc.Y))
	{
		return false;
	}
	if (auto* Player = GetNetOwningPlayer())
	{
		if (APlayerController* PlayerController = Player->PlayerController)
		{
			return !PlayerController->IsPaused();
		}
	}
	return false;
}

bool AAbyssTunnelsCharacter::GetTargetableCharacters(TArray<AAbyssTunnelsCharacter*>& OutTargetableCharacters) const
{
	bool bTargetableCharacterFound = false;
	if (UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			APlayerController* PlayerController = Iterator->Get();
			if (PlayerController == nullptr || PlayerController->Player == nullptr)
			{
				continue;
			}

			if (AAbyssTunnelsCharacter* Pawn = Cast<AAbyssTunnelsCharacter>(PlayerController->GetPawn()))
			{
				if (Pawn->GetIsAttackable())
				{
					OutTargetableCharacters.Add(Pawn);
					bTargetableCharacterFound = true;
				}
			}
		}
	}
	return bTargetableCharacterFound;
}

bool AAbyssTunnelsCharacter::IsOnBoss() const
{
	const auto NetMode = GetNetMode();
	return NetMode != NM_Client;
}

AAbyssTunnelsCharacter::AAbyssTunnelsCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 0.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	if (Mesh)
	{
		Mesh->bOwnerNoSee = true;
	}
	bUseControllerRotationYaw = true;
}

void AAbyssTunnelsCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();
	if (HasAuthority())
	{
		FScriptDelegate Delegate;
		Delegate.BindUFunction(this, "HandleOverlap");
		OnActorBeginOverlap.AddUnique(Delegate);
		InitialLocation = GetActorLocation();
	}
	if (AAbyssTunnelsPlayerController* PlayerController = Cast<AAbyssTunnelsPlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		PlayerController->Characters.Add(this);
	}
}

void AAbyssTunnelsCharacter::HandleOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	if (ATriggerSpace* TriggerSpace = Cast<ATriggerSpace>(OtherActor))
	{
		if (AAbyssTunnelsCharacter* Character = Cast<AAbyssTunnelsCharacter>(OverlappedActor))
		{
			TriggerSpace->OnCharacterOverlapped(Character);
		}
	}
}

void AAbyssTunnelsCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AAbyssTunnelsPlayerController* PlayerController = Cast<AAbyssTunnelsPlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		PlayerController->Characters.Remove(this);
	}
	Super::EndPlay(EndPlayReason);
}

//////////////////////////////////////////////////////////////////////////
// Input

void AAbyssTunnelsCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AAbyssTunnelsCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AAbyssTunnelsCharacter::Look);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AAbyssTunnelsCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
		OnMovementInputHappening();
	}
}

void AAbyssTunnelsCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}


void AAbyssTunnelsCharacter::UpdatePlayerSeeing()
{
	bIsSeenByPlayer = false;
	SeenCharacters.Empty();
	if (UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			APlayerController* PlayerController = Iterator->Get();
			if (PlayerController == nullptr || PlayerController->Player == nullptr)
			{
				continue;
			}
			if (AAbyssTunnelsCharacter* Character = Cast<AAbyssTunnelsCharacter>(PlayerController->GetCharacter()))
			{
				FHitResult Hit;
				if (GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), Character->GetActorLocation(), ECC_GameTraceChannel1)) // environment channel
				{
					continue; // there is no seeing through walls
				}
				if (UAbyssTunnelsGameplayStatics::IsCharacterFacing(this, Character))
				{
					SeenCharacters.AddUnique(Character);
				}
				if (UAbyssTunnelsGameplayStatics::IsCharacterFacing(Character, this))
				{
					bIsSeenByPlayer = true;
				}
			}
		}
	}
}

void AAbyssTunnelsCharacter::UpdateStealthEffectOnAllMaterials()
{
	const FName MaterialStealthParamName = FName(TEXT("OpacityMultiplier"));
	for (UMaterialInstanceDynamic* Material : DynamicMaterials)
	{
		if (bIsSeenByPlayer)
		{
			Material->SetScalarParameterValue(MaterialStealthParamName, 0.f);
		}
		else
		{
			Material->SetScalarParameterValue(MaterialStealthParamName, 1.f);
		}
	}
}

void AAbyssTunnelsCharacter::DoCameraFade(const bool bIsFadingIn)
{
	if (APlayerController* Player = GetLocalViewingPlayerController())
	{
		Player->PlayerCameraManager->bEnableFading = true;
		if (bIsFadingIn)
		{
			Player->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 3.0f, FLinearColor::Black, false, true);
		}
		else
		{
			Player->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, 3.0f, FLinearColor::Black, true, true);
		}
	}
}

void AAbyssTunnelsCharacter::OnRep_bIsSeenByPlayer()
{
	UpdateStealthEffectOnAllMaterials();
}

void AAbyssTunnelsCharacter::ConvertMaterialsToDynamic()
{
	int TotalMaterials = Mesh->GetNumMaterials();
	for (int i = 0; i < TotalMaterials; ++i)
	{
		UMaterialInstanceDynamic* Inst = UMaterialInstanceDynamic::Create(Mesh->GetMaterial(i), Mesh);
		Mesh->SetMaterial(i, Inst);
		DynamicMaterials.Add(Inst);
	}
}

void AAbyssTunnelsCharacter::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AAbyssTunnelsCharacter, bIsSeenByPlayer);
}
