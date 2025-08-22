// Fill out your copyright notice in the Description page of Project Settings.


#include "InstancedMeshActor.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Net/UnrealNetwork.h"


// Sets default values
AInstancedMeshActor::AInstancedMeshActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	//SetNetDormancy(ENetDormancy::DORM_DormantAll);
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(Root);
	InstancedStaticMeshComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("SomeStaticMesh"));
	if (InstancedStaticMeshComponent)
	{
		InstancedStaticMeshComponent->SetupAttachment(Root);
	}
}

void AInstancedMeshActor::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AInstancedMeshActor, Instances);
}

void AInstancedMeshActor::OnRep_Instances()
{
	for (FInstanceEntryData& Instance : Instances)
	{
		if (!ServerInstanceIndexToLocalIndexInstanceMap.Contains(Instance.Index))
		{
			int32 ClientIndex = InstancedStaticMeshComponent->AddInstance(Instance.InstanceTransform, true);
			ServerInstanceIndexToLocalIndexInstanceMap.Add(Instance.Index, ClientIndex);
		}
	}
}

void AInstancedMeshActor::AddInstance(const FTransform& NewInstanceTransform)
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Error, TEXT("AddInstance -- This only should happen on authority"));
		return;
	}
	FInstanceEntryData NewEntry;
	NewEntry.InstanceTransform = NewInstanceTransform;
	NewEntry.Index = InstancedStaticMeshComponent->AddInstance(NewInstanceTransform, true);
	FlushNetDormancy();
	Instances.Add(NewEntry);
}
