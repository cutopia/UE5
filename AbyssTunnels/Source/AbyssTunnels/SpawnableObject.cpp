// Fill out your copyright notice in the Description page of Project Settings.


#include "SpawnableObject.h"


// Sets default values
ASpawnableObject::ASpawnableObject()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}
