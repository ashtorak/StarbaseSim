// this is based on some forum post or so

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SimpleBuoyancy.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class STARBASESIMLIBRARY_API USimpleBuoyancy : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USimpleBuoyancy();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void AsyncPhysicsTickComponent(float DeltaTime, float SimTime) override;

	TObjectPtr<class UPrimitiveComponent> OwnerPrimitive;
	FTransform OwnerTransform;


	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ToolTip = "in m³, used to calculate Archimedes force"))
	float VolumeOfBody = 1.0f;

	float WATER_DENSITY = 1000.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float waterLevelinCM = 0.0f;
		
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float voxelHalfHeightinCM = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FVector> voxels;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float DAMPFER = 1.0f;
	
	FVector localArchimedesForce;
};
