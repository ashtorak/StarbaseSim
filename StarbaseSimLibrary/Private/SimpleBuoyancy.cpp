
#include "SimpleBuoyancy.h"
#include <Net/UnrealNetwork.h>
#include "AsyncTickFunctions.h"

// Sets default values for this component's properties
USimpleBuoyancy::USimpleBuoyancy()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	SetAsyncPhysicsTickEnabled(true);
}


// Called when the game starts
void USimpleBuoyancy::BeginPlay()
{
	Super::BeginPlay();

	OwnerPrimitive = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent());
	
	float archimedesForceMagnitude = WATER_DENSITY * 9.81f * VolumeOfBody * 100.0f; // in Unreal units
	localArchimedesForce = FVector(0, 0, archimedesForceMagnitude) / voxels.Num();
	
}


// Called every frame
void USimpleBuoyancy::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void USimpleBuoyancy::AsyncPhysicsTickComponent(float DeltaTime, float SimTime)
{
	if (OwnerPrimitive && OwnerPrimitive->IsSimulatingPhysics())
	{
		for(const FVector& point : voxels)
		{
			OwnerTransform = UAsyncTickFunctions::ATP_GetTransform(OwnerPrimitive);
			FVector worldPos = OwnerTransform.TransformPosition(point);

			if ( (worldPos.Z - voxelHalfHeightinCM) < waterLevelinCM)
			{
				float k = (waterLevelinCM - worldPos.Z) / (2 * voxelHalfHeightinCM) + 0.5f;
				k = FMath::Clamp(k, 0,1);

				FVector velocity = UAsyncTickFunctions::ATP_GetLinearVelocityAtPoint(OwnerPrimitive, worldPos);
				FVector localDampingForce = -velocity * DAMPFER * OwnerPrimitive->GetMass();
				FVector force = localDampingForce + FMath::Sqrt(k) * localArchimedesForce;
				UAsyncTickFunctions::ATP_AddForceAtPosition(OwnerPrimitive, worldPos, force);
			}
		}
	}
}
