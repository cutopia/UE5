// Fill out your copyright notice in the Description page of Project Settings.


#include "Equipment.h"

#include "Kismet/KismetMathLibrary.h"

AEquipment::AEquipment()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	NetDormancy = ENetDormancy::DORM_DormantAll;
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

}

FRotator AEquipment::GetPlaneRotation(const TArray<FVector>& PlaneSamples) const
{
	if (PlaneSamples.Num() == 3)
	{
		FVector FirstVec = PlaneSamples[0] - PlaneSamples[1];
		FVector SecondVec = PlaneSamples[0] - PlaneSamples[2];
		FVector CrossVec = UKismetMathLibrary::Cross_VectorVector(FirstVec, SecondVec);
		return UKismetMathLibrary::FindRelativeLookAtRotation(FTransform(PlaneSamples[0]), PlaneSamples[0] + (CrossVec * 2000.f));
	}
	
	return FRotator::ZeroRotator;
}

bool AEquipment::GetHitAtScreenPos(const FVector2d& ScreenPosition, FVector& OutHit) const
{
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PlayerController = World->GetFirstPlayerController())
		{
			FVector WorldLoc, WorldDir;
			if (PlayerController->DeprojectScreenPositionToWorld(ScreenPosition.X, ScreenPosition.Y, WorldLoc, WorldDir))
			{
				FHitResult Hit;
				FVector TraceEnd = WorldLoc + (WorldDir * 2000.f);
				// if we trace to center successfully after that we will take some additional samples to determine geometric plane.
				if (World->LineTraceSingleByChannel(Hit, WorldLoc, TraceEnd, ECC_GameTraceChannel1))
				{
					OutHit = Hit.Location;
					return true;
				}
			}
		}
	}
	return false;
}

void AEquipment::BeginPlay()
{
	SetReplicateMovement(true);
	Super::BeginPlay();
}
