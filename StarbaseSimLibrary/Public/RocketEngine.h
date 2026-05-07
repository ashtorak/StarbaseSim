// this is an older class, which is not used in latest version

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"

#include "RocketEngine.generated.h"

/**
 * 
 */
UCLASS(ClassGroup = (Rocket), meta = (BlueprintSpawnableComponent))
class STARBASESIMLIBRARY_API URocketEngine : public UActorComponent
{
	GENERATED_BODY()

public:

	URocketEngine();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void AsyncPhysicsTickComponent(float DeltaTime, float SimTime) override;

	UPROPERTY(BlueprintReadWrite)
		FConstraintInstanceAccessor constraint;

	UPROPERTY(BlueprintReadWrite)
		float GimbalRoationOffset;

	UPROPERTY(BlueprintReadWrite)
		FName bone;

	UPROPERTY(BlueprintReadWrite)
		int Number;

	UPROPERTY(BlueprintReadWrite)
		bool isGimbal;

	UPROPERTY(BlueprintReadWrite, replicated)
		bool isFiring;

	UPROPERTY(BlueprintReadWrite, replicated)
		bool isFiringUp;

	UPROPERTY(BlueprintReadWrite, replicated)
		bool isShuttingDown;

	//UPROPERTY(BlueprintReadWrite)
	//	bool isFiringRequested;

	UPROPERTY(BlueprintReadWrite)
		float Throttle;

	//UPROPERTY(BlueprintReadWrite)
		float ThrottleRequest;

		void SetThrottleRequest(float in)
		{
			ThrottleRequest = FMath::Clamp(in, 0.01f, 1);
		}

	// time from 0 to 1
	UPROPERTY(BlueprintReadWrite)
		float ThrottleRate = 0.4f;

	float GetGimbalRotationOffsetRadians() const
	{
		return FMath::DegreesToRadians(GimbalRoationOffset);
	}

	UFUNCTION(BlueprintCallable)
		void StartUp(float delay);

	UFUNCTION(BlueprintCallable)
		void Stop(float delay);

	//float StartUpThrottleTarget = 0.2f;

	UPROPERTY(BlueprintReadWrite)
		float StartUpDuration = 0.4f;

	float StartUpDelay;
	float StartUpTime;
	float ShutDownDelay;
	float ShutDownTime;
	float CurrentSimTime;
};
