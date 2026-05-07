// Fill out your copyright notice in the Description page of Project Settings.


#include "RocketEngine.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include <Net/UnrealNetwork.h>
#include "AsyncTickFunctions.h"
#include "Math/UnrealMathUtility.h"
#include "Kismet/KismetMathLibrary.h"

URocketEngine::URocketEngine()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	SetAsyncPhysicsTickEnabled(true);

	//  needed for replication
	SetIsReplicatedByDefault(true);
}

//  needed for replication
void URocketEngine::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(URocketEngine, isFiring);
	DOREPLIFETIME(URocketEngine, isFiringUp);
	DOREPLIFETIME(URocketEngine, isShuttingDown);

}


// Called when the game starts
void URocketEngine::BeginPlay()
{
	Super::BeginPlay();


}


// Called every frame
void URocketEngine::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

void URocketEngine::AsyncPhysicsTickComponent(float DeltaTime, float SimTime)
{
	if (isFiringUp && SimTime > StartUpDelay + StartUpTime)
	{
		isFiring = true;
		if (Throttle < ThrottleRequest)	Throttle += DeltaTime / StartUpDuration;
		else
		{
			isFiringUp = false;
		}
	}
	else if (isShuttingDown && SimTime > ShutDownDelay + ShutDownTime)
	{
		if(Throttle < 0.1f) isShuttingDown = isFiring = false;
		else Throttle -= DeltaTime / StartUpDuration;

	}
	else if (isFiring)
	{
		// let actual throttle follow input with fixed rate (simple simulation of a valve)
		float deltaRate = DeltaTime / ThrottleRate;
		float deltaThrottle = ThrottleRequest - Throttle;
		if (FMath::Abs(deltaThrottle) < deltaRate)
		{
			Throttle = ThrottleRequest;
		}
		else
		{
			Throttle += FMath::Sign(deltaThrottle) * deltaRate;
		}

	}

	CurrentSimTime = SimTime;
}

void URocketEngine::StartUp(float delay)
{
	if (!isFiringUp && !isFiring)
	{
		isShuttingDown = isFiring = false;
		isFiringUp = true;
		Throttle = 0;
		StartUpDelay = delay;
		StartUpTime = CurrentSimTime;
	}
	
}

void URocketEngine::Stop(float delay)
{
	if (isFiringUp || isFiring)
	{
		isFiringUp = false;
		isShuttingDown = true;
		ShutDownDelay = delay;
		ShutDownTime = CurrentSimTime;
	}
}