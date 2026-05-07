// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PIDComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class STARBASESIMLIBRARY_API UPIDComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UPIDComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	UPROPERTY(BlueprintReadWrite)
		bool bIsOpenLoop = true;
	UPROPERTY(BlueprintReadWrite, Replicated)
		float Kp = 1.0f;
	UPROPERTY(BlueprintReadWrite, Replicated)
		float Ki = 1.0f;
	UPROPERTY(BlueprintReadWrite, Replicated)
		float Kd = 1.0f;
	UPROPERTY(BlueprintReadWrite)
		float K_factor = 1.0f;
	UPROPERTY(BlueprintReadWrite)
		float Kp_default = 1.0f;
	UPROPERTY(BlueprintReadWrite)
		float Ki_default = 1.0f;
	UPROPERTY(BlueprintReadWrite)
		float Kd_default = 1.0f;
	UPROPERTY(BlueprintReadWrite, Replicated)
		float OutputMin = 0.0f;
	UPROPERTY(BlueprintReadWrite, Replicated)
		float OutputMax = 0.0f;
	UPROPERTY(BlueprintReadWrite)
		float OutputMin_default = 0.0f;
	UPROPERTY(BlueprintReadWrite)
		float OutputMax_default = 1.0f;
	UPROPERTY(BlueprintReadWrite)
		float Output = 0.0f;
	UPROPERTY(BlueprintReadWrite)
		float OutputOpenLoop = 0.0f;
	UPROPERTY(BlueprintReadWrite)
		float Error = 0.0f;
	UPROPERTY(BlueprintReadWrite)
		float Error_previous = 0.0f;
		float TotalError_previous = 0.0f;
	UPROPERTY(BlueprintReadWrite)
		float TotalError = 0.0f;
	UPROPERTY(BlueprintReadWrite)
		float P = 0.0f;
	UPROPERTY(BlueprintReadWrite, Replicated)
		float I = 0.0f;
	UPROPERTY(BlueprintReadWrite)
		float D = 0.0f;
	UPROPERTY(BlueprintReadWrite)
		float ProcessValue = 0.0f;
	UPROPERTY(BlueprintReadWrite)
		float Setpoint = 0.0f;
	UPROPERTY(BlueprintReadWrite)
		bool flipOutput;
	UPROPERTY(BlueprintReadWrite)
		float Limit = 0.0f;

	UPROPERTY(BlueprintReadWrite)
		float SmoothingFactor = 0.07f;

	UPROPERTY(BlueprintReadWrite)
		float SmoothingFactorD = 0.005f;

	UFUNCTION(BlueprintCallable)
		void SetOpenLoop(bool newValue);
	UFUNCTION(BlueprintCallable)
		void Zero();
	UFUNCTION(BlueprintCallable)
		void ResetParametersToDefaults();
	UFUNCTION(BlueprintCallable)
		void CalculateOutput(float InDeltaTime);

//private:

	UPROPERTY(BlueprintReadWrite)
		float PV_avg;
	UPROPERTY(BlueprintReadWrite)
		float D_avg;

};
