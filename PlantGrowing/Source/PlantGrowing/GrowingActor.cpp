// Enfield 2025
#include "GrowingActor.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Kismet/KismetMathLibrary.h"

AGrowingActor::AGrowingActor()
{
	PrimaryActorTick.bCanEverTick = false;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	Stalk = CreateDefaultSubobject<UInstancedStaticMeshComponent>("StalkComponent", false);
	Stalk->SetupAttachment(Root);
	Leaves = CreateDefaultSubobject<UInstancedStaticMeshComponent>("LeavesComponent", false);
	Leaves->SetupAttachment(Root);
}

void AGrowingActor::BeginPlay()
{
	Super::BeginPlay();
	if (const UWorld* World = GetWorld())
	{
		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, GetActorLocation(), GetActorLocation() + FVector(0,0,-100000), ECC_WorldStatic))
		{
			Grow(Hit.ImpactPoint);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Could not find the ground to grow in."));
		}
	}
}

void AGrowingActor::Grow(const FVector& StartingPoint)
{
	// if we are not configured to utilize hardcoded parameters, then we randomly generate them instead.
	if (!SpecifyParameters)
	{
		bNodesOpposite = FMath::RandBool();
		ApicalDominance = FMath::RandRange(0.0f, 0.6f);
		Iterations = FMath::RandRange(10, 30);
		LeafLifespan = FMath::RandRange(3, Iterations);
		ScaleStep = FMath::RandRange(0.7f, 0.9f);
	}

	FTransform Transform = FTransform(StartingPoint);
	int32 InstanceIndex = Stalk->AddInstance(Transform, true);
	// This check is a gatekeeper for a mesh not having been assigned to the ISM component yet.
	if (Stalk->InstanceBodies.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to add instance."));
		return;
	}
	FBox Bounds = Stalk->InstanceBodies[InstanceIndex]->GetBodyBounds();
	FVector Extents = Bounds.GetExtent();
	FTransform NewEndPoint = FTransform(StartingPoint + FVector(0, 0, Extents.Z));
	EndPoints.Add(NewEndPoint);
	TArray<FTransform> FreshStartPoints;
	UE_LOG(LogTemp, Log, TEXT("Iterations: %d"), Iterations);
	for (int i = 0; i < Iterations; ++i)
	{
		const float Percent = (i * 1.0f)/Iterations;
		while (EndPoints.Num() > 0)
		{
			FTransform EndPoint = EndPoints.Pop();
			FreshStartPoints.Append(GrowFromNode(EndPoint, i + LeafLifespan >= Iterations, Percent < (1-ApicalDominance)));
		}

		for (int j = 0; j < FreshStartPoints.Num() && j < 100; j++)
		{
			EndPoints.Add(FreshStartPoints[j]);
		}
		FreshStartPoints.Empty();
	}
}

void AGrowingActor::PlaceLeavesAtNode(const FTransform& StartingTransform, const FVector& Extents)
{
	FTransform LeafTransform = FTransform(StartingTransform);
	LeafTransform.AddToTranslation(FVector(Extents.X, Extents.Y, 0));
	LeafTransform.ConcatenateRotation(LeafRotationOrientation.Quaternion());
	Leaves->AddInstance(LeafTransform, true);
	LeafTransform.SetRotation(LeafTransform.GetRotation().Inverse());
	LeafTransform.SetLocation(StartingTransform.GetLocation());
	LeafTransform.AddToTranslation(FVector(-Extents.X, -Extents.Y, 0));
	Leaves->AddInstance(LeafTransform, true);
}

TArray<FTransform> AGrowingActor::GrowFromNode(const FTransform& StartingTransform, const bool bPlaceLeaves, const bool bPlaceSideShoots)
{
	TArray<FTransform> NewEndPoints;
	// place the new stalk
	FTransform CentralStalkTransform = FTransform(StartingTransform);
	FVector Scale = StartingTransform.GetScale3D();
	Scale *= ScaleStep;
	if (Scale.X < MinScaleFactor)
	{
		return NewEndPoints;
	}
	CentralStalkTransform.SetScale3D(FVector(Scale));
	FVector Extents;
	FTransform NewestCentralStalkEndPoint = PlaceShoot(CentralStalkTransform, false, Extents);
	FTransform TwistedNewEndPoint = FTransform(NewestCentralStalkEndPoint);
	TwistedNewEndPoint.ConcatenateRotation(FRotator(0,GrowthTwist,0).Quaternion());
	NewEndPoints.Add(TwistedNewEndPoint);
		
	// if the apical dominance checks out then we also place side stalks spreading out at an angle from the main stalk.
	if (bPlaceSideShoots)
	{
		// side shoot condition met
		FTransform SideShootTransform = FTransform(CentralStalkTransform);
		SideShootTransform.ConcatenateRotation(FRotator(90,0,0).Quaternion());
		FVector _;
		NewEndPoints.Add(PlaceShoot(SideShootTransform, true, _));
		SideShootTransform = FTransform(CentralStalkTransform);
		SideShootTransform.ConcatenateRotation(FRotator(-90, 0, 0).Quaternion());
		if (!bNodesOpposite)
		{
			SideShootTransform.AddToTranslation(NewestCentralStalkEndPoint.GetLocation()-CentralStalkTransform.GetLocation());
		}
		NewEndPoints.Add(PlaceShoot(SideShootTransform, true, _));
	}
	// place leaves too if LeafLifespan permits. they should be at a slightly lower angle from any side stalks to avoid overlap.
	if (bPlaceLeaves)
	{
		PlaceLeavesAtNode(CentralStalkTransform, Extents);
	}
	
	return NewEndPoints;
}

FTransform AGrowingActor::PlaceShoot(const FTransform& PlacementTransform, const bool bApplyApicalDominance, FVector& OutExtents) const
{
	const int32 InstanceIndex = Stalk->AddInstance(PlacementTransform, true);
	const FBox Bounds = Stalk->InstanceBodies[InstanceIndex]->GetBodyBoundsLocal();
	const FVector Extents = Bounds.GetExtent();
	const FVector DirectionVector = UKismetMathLibrary::GetUpVector(PlacementTransform.GetRotation().Rotator());
	FTransform NewEndPoint = FTransform(PlacementTransform);
	NewEndPoint.AddToTranslation(DirectionVector * Extents.Z);
	// if (bApplyApicalDominance)
	// {
	// 	NewEndPoint.MultiplyScale3D(FVector(ApicalDominance));
	// }
	return NewEndPoint;
}
