// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"

#include "RocketEngines.generated.h"

USTRUCT(BlueprintType)
struct FEngineProperties
{
	GENERATED_BODY()

		UPROPERTY(BlueprintReadWrite)
		FName BoneName;

	UPROPERTY(BlueprintReadWrite)
		FConstraintInstanceAccessor constraint;

	UPROPERTY(BlueprintReadWrite)
		float GimbalRotationOffset;

	UPROPERTY(BlueprintReadWrite)
		bool isGimbal;

	UPROPERTY(BlueprintReadWrite)
		float Throttle;

	UPROPERTY(BlueprintReadWrite)
		bool isRVac;
};

USTRUCT(BlueprintType)
struct FEngineReplicatedStates
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
		bool isFiring;

	UPROPERTY(BlueprintReadWrite)
		bool isFiringUp;

	UPROPERTY(BlueprintReadWrite)
		bool isShuttingDown;

	UPROPERTY(BlueprintReadWrite)
		bool isExisting;
};

/**
 *
 */
UCLASS(ClassGroup = (Rocket), meta = (BlueprintSpawnableComponent))
class STARBASESIMLIBRARY_API URocketEngines : public UActorComponent
{
	GENERATED_BODY()

public:

	URocketEngines();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void AsyncPhysicsTickComponent(float DeltaTime, float SimTime) override;

	UPROPERTY(BlueprintReadWrite, replicated)
		TArray<struct FEngineReplicatedStates> EngineReplicatedStates;

	UPROPERTY(BlueprintReadOnly)
		TArray<struct FEngineProperties> EngineProperties;




	//UPROPERTY(BlueprintReadWrite)
	//	bool isFiringRequested;



	UPROPERTY(replicated)
		float ThrottleRequest;

	void SetThrottleRequest(float in)
	{
		ThrottleRequest = FMath::Clamp(in, 0.01f, 1);
	}

	// time from 0 to 1
	UPROPERTY(BlueprintReadWrite)
		float ThrottleRate = 0.4f;

	// engine mass in kg, same for all
	UPROPERTY(BlueprintReadWrite)
		float EngineMass = 1500.0f;

	// does nothing if not isExisting
	UFUNCTION(BlueprintCallable)
		void StartUp(int number);

	// does nothing if not isExisting
	UFUNCTION(BlueprintCallable)
		void Stop(int number);

	UFUNCTION(BlueprintCallable)
		bool GetIsFiring(int number) const
	{
		if (number > 0 && EngineReplicatedStates.Num() >= number)
		{
			return EngineReplicatedStates[number - 1].isFiring;
		}
		else return false;
	}

	UFUNCTION(BlueprintCallable)
		bool GetIsExisting(int number) const
	{
		if (number > 0 && EngineReplicatedStates.Num() >= number)
		{
			return EngineReplicatedStates[number - 1].isExisting;
		}
		else return false;
	}

	UFUNCTION(BlueprintCallable)
	void SetIsExisting(int number, bool newState)
	{
		if (number > 0 && EngineReplicatedStates.Num() >= number)
		{
			EngineReplicatedStates[number - 1].isExisting = newState;
		}
	}

	UFUNCTION(BlueprintCallable)
		float GetThrottle(int number) const
	{
		if (number > 0 && EngineProperties.Num() >= number)
		{
			return EngineProperties[number - 1].Throttle;
		}
		else return -1;
	}

	UFUNCTION(BlueprintCallable)
		FName GetBoneName(int number) const
	{
		if (number > 0 && EngineProperties.Num() >= number)
		{
			return EngineProperties[number - 1].BoneName;
		}
		else return FName();
	}

	UFUNCTION(BlueprintCallable)
		void SetEngineParams(int number, FName bone, FConstraintInstanceAccessor constraint, float GimbalRoationOffset, bool isGimbal, bool isRVac)
	{
		if (number > 0 && EngineProperties.Num() >= number)
		{
			EngineProperties[number - 1].BoneName = bone;
			EngineProperties[number - 1].constraint = constraint;
			EngineProperties[number - 1].GimbalRotationOffset = GimbalRoationOffset;
			EngineProperties[number - 1].isGimbal = isGimbal;
			EngineProperties[number - 1].isRVac = isRVac;
		}
	}

	// also sets isExisting to true
	UFUNCTION(BlueprintCallable)
		void AddEngine();

	UFUNCTION(BlueprintCallable)
		void RemoveAllEngines();

	//float StartUpThrottleTarget = 0.2f;

	UPROPERTY(BlueprintReadWrite)
		float StartUpDuration = 0.3f;

	float StartUpDelay;
	float StartUpTime;
	float ShutDownDelay;
	float ShutDownTime;
	float CurrentSimTime;
};
