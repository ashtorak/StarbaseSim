
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Valve.generated.h"

UCLASS(Blueprintable, ClassGroup=(Tanking), meta=(BlueprintSpawnableComponent) )
class STARBASESIMLIBRARY_API UValve : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UValve();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	virtual void AsyncPhysicsTickComponent(float DeltaTime, float SimTime) override;

protected:
	float _DeltaTime = 0.0f;

public:

	// lever;
	float leverAngleInitial;
	float openingSpeed = 0.05f;

	/// <summary>
	/// goes true if opening > 0
	/// </summary>
	UPROPERTY(BlueprintReadOnly)
		bool isOpen = false;
	UPROPERTY(BlueprintReadOnly)
		bool isFullyOpen = false;

	/// <summary>
	/// true if opening <0.01 and > 0.99
	/// </summary>
	UPROPERTY(BlueprintReadOnly)
		bool isPartiallyOpen = false;

	UPROPERTY(BlueprintReadOnly, Replicated)
		float setpoint = 0.0f;

	float opening = 0.0f;

	void openClose();
	void setOpening(float value);
};
