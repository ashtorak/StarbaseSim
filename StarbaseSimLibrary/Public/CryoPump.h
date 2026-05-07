// see readme.txt

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CryoPump.generated.h"

UCLASS(Blueprintable, ClassGroup=(Tanking), meta=(BlueprintSpawnableComponent) )
class STARBASESIMLIBRARY_API UCryoPump : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCryoPump();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	virtual void AsyncPhysicsTickComponent(float DeltaTime, float SimTime) override;

	UFUNCTION(BlueprintCallable)
	void switchOnOff();

	UFUNCTION(BlueprintCallable)
	void switchOff();

protected:
	float _DeltaTime = 0.0f;

public:

	UPROPERTY(BlueprintReadWrite)
	float nominalPressureHead = 14;
	UPROPERTY(BlueprintReadWrite)
	int numberOfPumps = 1;

	UPROPERTY(BlueprintReadWrite, Replicated)
	float power = 0;
	UPROPERTY(BlueprintReadWrite)
	float pressure;
	UPROPERTY(BlueprintReadWrite, Replicated)
	bool isOn;
	UPROPERTY(BlueprintReadWrite, Replicated)
	float flowrate_in_m3_per_second;
	UPROPERTY(BlueprintReadWrite)
	float mechanicalPower_in_kW;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UAudioComponent> AudioLoop;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UAudioComponent> AudioStartup;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UAudioComponent> AudioShutdown;

	float _SimTime;
	float soundStartedTime;
	float soundStartDelay = 1.0f;
	float AudioVolumeMultiplier = 0.7f;

	void loopSound();

	void playStartupSound(float pitchAdjustment);

	void playShutdownSound(float pitchAdjustment);
};
