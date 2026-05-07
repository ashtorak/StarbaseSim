// Fill out your copyright notice in the Description page of Project Settings.


#include "RocketEngines.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include <Net/UnrealNetwork.h>
#include "AsyncTickFunctions.h"
#include "Math/UnrealMathUtility.h"
#include "Kismet/KismetMathLibrary.h"

URocketEngines::URocketEngines()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	SetAsyncPhysicsTickEnabled(true);

	//  needed for replication
	SetIsReplicatedByDefault(true);
}

//  needed for replication
void URocketEngines::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(URocketEngines, EngineReplicatedStates);
	DOREPLIFETIME(URocketEngines, ThrottleRequest);

}


// Called when the game starts
void URocketEngines::BeginPlay()
{
	Super::BeginPlay();


}


// Called every frame
void URocketEngines::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

void URocketEngines::AsyncPhysicsTickComponent(float DeltaTime, float SimTime)
{
	if (!EngineReplicatedStates.IsEmpty() && !EngineProperties.IsEmpty())
	{
		int i = 0;
		for (FEngineReplicatedStates& Engine : EngineReplicatedStates)
		{
			if (Engine.isExisting)
			{
				if (Engine.isFiringUp)
				{
					Engine.isFiring = true;
						if (EngineProperties[i].Throttle < ThrottleRequest)	EngineProperties[i].Throttle += DeltaTime / StartUpDuration;
						else
						{
							Engine.isFiringUp = false;
						}
				}
				else if (Engine.isShuttingDown)
				{
					if (EngineProperties[i].Throttle < 0.1f) Engine.isShuttingDown = Engine.isFiring = false;
					else EngineProperties[i].Throttle -= DeltaTime / StartUpDuration;

				}
				else if (Engine.isFiring)
				{
					// let actual throttle follow input with fixed rate (simple simulation of a valve)
					float deltaRate = DeltaTime / ThrottleRate;
					float deltaThrottle = ThrottleRequest - EngineProperties[i].Throttle;
					if (FMath::Abs(deltaThrottle) < deltaRate)
					{
						EngineProperties[i].Throttle = ThrottleRequest;
					}
					else
					{
						EngineProperties[i].Throttle += FMath::Sign(deltaThrottle) * deltaRate;
					}
				}
			}
			else
			{
				Engine.isFiringUp = Engine.isFiring = Engine.isShuttingDown = false;
			}
			i++;
		}	
	}
}

void URocketEngines::StartUp(int number)
{
	FEngineReplicatedStates& state = EngineReplicatedStates[number-1];
	FEngineProperties& property = EngineProperties[number-1];

	if (state.isExisting && !state.isFiringUp && !state.isFiring)
	{
		state.isShuttingDown = state.isFiring = false;
		state.isFiringUp = true;
		property.Throttle = 0;
		/*EngineReplicatedStates[number - 1] = state;
		EngineProperties[number - 1] = property;*/
	}

}

void URocketEngines::Stop(int number)
{
	FEngineReplicatedStates& state = EngineReplicatedStates[number-1];

	if (state.isExisting && (state.isFiringUp || state.isFiring))
	{
		state.isFiringUp = false;
		state.isShuttingDown = true;
		//EngineReplicatedStates[number - 1] = state;
	}
}

void URocketEngines::AddEngine()
{
	EngineReplicatedStates.Add(FEngineReplicatedStates());
	EngineReplicatedStates.Last().isExisting = true;
	EngineProperties.Add(FEngineProperties());
}

void URocketEngines::RemoveAllEngines()
{
	EngineReplicatedStates.Empty();
	EngineProperties.Empty();
}