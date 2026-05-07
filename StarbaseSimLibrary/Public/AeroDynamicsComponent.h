// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include "AeroDynamicsComponent.generated.h"

USTRUCT(BlueprintType)
struct FAeroPart
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
		FName BoneName;

	// gridfin, flap
	UPROPERTY(BlueprintReadWrite)
		FString PartType;

	// in m²
	UPROPERTY(BlueprintReadWrite)
		float SurfaceArea;

	UPROPERTY(BlueprintReadWrite)
		float CoefficientOfDrag;

	// in N
	UPROPERTY(BlueprintReadWrite)
		FVector DragForce;

	// in m/s
	UPROPERTY(BlueprintReadWrite)
		FVector velocity;

	UPROPERTY(BlueprintReadWrite)
		bool isBroken;

	UPROPERTY(BlueprintReadWrite)
		FVector InitialLocation;
};

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class STARBASESIMLIBRARY_API UAeroDynamicsComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAeroDynamicsComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void AsyncPhysicsTickComponent(float DeltaTime, float SimTime) override;

	UPROPERTY(BlueprintReadWrite)
		bool DoAsyncTick = true;

	UPROPERTY(BlueprintReadWrite)
		bool DoParts = true;


	TObjectPtr<class UPrimitiveComponent> OwnerPrimitive;
	FTransform OwnerTransform;

	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class UPrimitiveComponent> PrimitiveBP;


	// in m above sea level
		float Altitude;

	// in kg/m³
		float DensityAir;

		FVector Velocity;
		float VelocitySquare;

	// in N
	UPROPERTY(BlueprintReadWrite)
		FVector DragForce;

	UPROPERTY(BlueprintReadWrite)
		float CoefficientOfDrag = 0.5f;

	UPROPERTY(BlueprintReadWrite)
		float DragMainBodyFactor = 1.0f;

	UPROPERTY(BlueprintReadWrite)
		float CoefficientOfAngularDrag = 2.0f;

	UPROPERTY(BlueprintReadWrite)
		float AngularDragLowVelocityMultiplier = 22.0f;


	// in m²
	UPROPERTY(BlueprintReadWrite)
		float AeroSurface = 630.0f;

	UPROPERTY(BlueprintReadWrite)
		float CosAngleOfAttack;

	// this acts as a rough method to reduce the effective surface area when it's moving head on
	float CosLowerClamp = 0.2f;

	UPROPERTY(BlueprintReadWrite)
		FVector AeroAttackPosLocal;

	UPROPERTY(BlueprintReadWrite)
		TArray<struct FAeroPart> AeroParts;

};
