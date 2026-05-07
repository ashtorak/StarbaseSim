#include "AeroDynamicsComponent.h"
#include "Math/UnrealMathUtility.h"
#include "Kismet/KismetMathLibrary.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include <Net/UnrealNetwork.h>
#include "AsyncTickFunctions.h"

// Sets default values for this component's properties
UAeroDynamicsComponent::UAeroDynamicsComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	SetAsyncPhysicsTickEnabled(true);
}


// Called when the game starts
void UAeroDynamicsComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerPrimitive = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent());
}


// Called every frame
void UAeroDynamicsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

void UAeroDynamicsComponent::AsyncPhysicsTickComponent(float DeltaTime, float SimTime)
{
	if (DoAsyncTick)
	{
		if (!OwnerPrimitive)
		{
			OwnerPrimitive = PrimitiveBP;
			UE_LOG(LogTemp, Log, TEXT("AeroDynamicsComponent"));
		}

		if (OwnerPrimitive && OwnerPrimitive->IsSimulatingPhysics())
		{
			OwnerTransform = UAsyncTickFunctions::ATP_GetTransform(OwnerPrimitive);
			Altitude = OwnerTransform.GetLocation().Z * 0.01f;
			DensityAir = 1.2 * FMath::Exp(-Altitude / 10000);

			Velocity = UAsyncTickFunctions::ATP_GetLinearVelocity(OwnerPrimitive) * 0.01f;

			FVector upVector = OwnerTransform.GetRotation().GetUpVector();
			upVector.Normalize();
		
			// this so that when up vector is in direction of velocity, we reduce the cos about the same ratio,
			// since the surface area is typically smaller on the rocket's nose compared to the side
			CosAngleOfAttack = FMath::Clamp(1-FMath::Abs(upVector.Dot(Velocity)), CosLowerClamp, 1);
		
			// add contributions of axial and radial drag
			FVector velocity1 = Velocity.ProjectOnTo(upVector);
			float squaredLength = velocity1.SquaredLength();
			velocity1.Normalize();
			FVector drag1 = 127.0f * squaredLength * -velocity1; // 9 m diameter circle surface same for booster and ship

			FVector velocity2 = Velocity.VectorPlaneProject(Velocity, upVector);
			squaredLength = velocity2.SquaredLength();
			velocity2.Normalize();
			FVector drag2 = AeroSurface * squaredLength * -velocity2;

			FVector dragSum = drag1 + drag2;

			DragForce = DensityAir * CoefficientOfDrag * DragMainBodyFactor * 0.5f * dragSum ;
			UAsyncTickFunctions::ATP_AddForceAtPosition(OwnerPrimitive, OwnerTransform.TransformPosition(AeroAttackPosLocal), DragForce * 100.0f);


			// have some angular damping in atmosphere depending on density
			FVector AngVel = UAsyncTickFunctions::ATP_GetAngularVelocity(OwnerPrimitive);
			squaredLength = AngVel.SquaredLength();
			AngVel.Normalize();
			FVector angularDrag = CoefficientOfAngularDrag * squaredLength * -AngVel;

			// provide íncreased damping at low angular velocity to bring it to zero easier without need for perfect control
			/*if (squaredLength < 0.01f) UAsyncTickFunctions::ATP_AddTorque(OwnerPrimitive, angularDrag * DensityAir * AngularDragLowVelocityMultiplier * -AngVel, false);
			else UAsyncTickFunctions::ATP_AddTorque(OwnerPrimitive, angularDrag * DensityAir, false);e*/
			
			UAsyncTickFunctions::ATP_AddTorque(OwnerPrimitive, angularDrag * DensityAir, false);

			if (DoParts)
			{
				for (FAeroPart& part : AeroParts)
				{
					Velocity = UAsyncTickFunctions::ATP_GetLinearVelocity(OwnerPrimitive, part.BoneName) * 0.01f;
					VelocitySquare = Velocity.SquaredLength() * FMath::Clamp(333*333/Velocity.SquaredLength(), 0.1f, 1.0f); // reduce drag at hyper sonic speeds (in a very simple way) - what would be a realistic minimum here?

					const FTransform t = UAsyncTickFunctions::ATP_GetSocketTransform(OwnerPrimitive, part.BoneName);
					const FVector worldPosition = t.GetLocation();
					const FQuat worldRotation = t.GetRotation();

					upVector = worldRotation.GetUpVector();
					Velocity.Normalize();
					CosAngleOfAttack = FMath::Abs(upVector.Dot(Velocity));
					float PartCosAoA = 1.0f;
					if (part.PartType == "gridfin")
					{
						part.velocity = Velocity;
						// CosAoA goes from 1 to 0 while the effective cos of part in case of
						// grid fin goes from 0.05->1->0.05
						PartCosAoA = FMath::Clamp(FMath::Sin(2 * FMath::Acos(CosAngleOfAttack)),0.05f,1);

						// add contribution of each grid diagonal and make a combined vector with normal velocity based drag
						drag1 = worldRotation.RotateVector(FVector(1, 1, 0));
						drag2 = worldRotation.RotateVector(FVector(1, -1, 0));
						dragSum = -(drag1.Dot(Velocity) * drag1 + drag2.Dot(Velocity) * drag2 + CosAngleOfAttack * Velocity);
						dragSum.Normalize();

						part.DragForce = DensityAir * part.CoefficientOfDrag * 0.25f * part.SurfaceArea * VelocitySquare * dragSum * PartCosAoA;
						// 0.25 here because we have 0.5 anywany and then we use only half of the flat laying grid fin area effectively when it's 45° angled at max
					}
					else if (part.PartType == "flap")
					{
						FVector direction = worldRotation.RotateVector(FVector(0, 1, 0)); // have force always pointing in flap normal direction for simplicity
						direction.Normalize();
						float dot = direction.Dot(Velocity);
						PartCosAoA = FMath::Clamp(FMath::Abs(dot), 0.05f, 1);

						part.DragForce = DensityAir * part.CoefficientOfDrag * 0.5f * part.SurfaceArea * VelocitySquare* direction * PartCosAoA * -FMath::Sign(dot);
					}

			
					UAsyncTickFunctions::ATP_AddForceAtPosition(OwnerPrimitive, worldPosition, part.DragForce * 100.0f, part.BoneName);

					// calculate total resulting force for external use
					DragForce += part.DragForce;
				}
			}
		}

	}
}