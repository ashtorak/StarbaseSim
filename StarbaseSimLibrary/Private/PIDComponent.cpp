// Fill out your copyright notice in the Description page of Project Settings.

#include "PIDComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include <Net/UnrealNetwork.h>

// Sets default values for this component's properties
UPIDComponent::UPIDComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	//  needed for replication
	SetIsReplicatedByDefault(true);
}


//  needed for replication
void UPIDComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UPIDComponent, Kp);
	DOREPLIFETIME(UPIDComponent, Ki);
	DOREPLIFETIME(UPIDComponent, Kd);
	DOREPLIFETIME(UPIDComponent, OutputMin);
	DOREPLIFETIME(UPIDComponent, OutputMax);
	DOREPLIFETIME(UPIDComponent, I);
}

// Called when the game starts
void UPIDComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UPIDComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UPIDComponent::CalculateOutput(float InDeltaTime){
	if (bIsOpenLoop)
	{
		Output = OutputOpenLoop;
	}
	else
	{
		// don't process inf or NaN
		if (!isfinite(ProcessValue) || !isfinite(Setpoint)) {
			return;
		}
		PV_avg = UKismetMathLibrary::WeightedMovingAverage_Float(ProcessValue, PV_avg, SmoothingFactor);
		Error = Setpoint - PV_avg;

		P = Kp * K_factor * Error;

		if (Limit * Error > 0 && Limit * TotalError > 0)
		{
			// don't increase total error when limit and error are going in same direction
		}
		else TotalError = UKismetMathLibrary::FClamp(InDeltaTime * Error + TotalError_previous, UKismetMathLibrary::SafeDivide(OutputMin, Ki * K_factor), UKismetMathLibrary::SafeDivide(OutputMax, Ki * K_factor));
		
		I = Ki * K_factor * TotalError;

		D = Kd * K_factor * 0.5f * (Error - Error_previous) / InDeltaTime; // don't increase Kd as much with K_factor
		D_avg = UKismetMathLibrary::WeightedMovingAverage_Float(D, D_avg, SmoothingFactorD);

		Output = UKismetMathLibrary::FClamp(P + I + D_avg, OutputMin, OutputMax);
		if (flipOutput) Output = -Output;

		Error_previous = Error;
		TotalError_previous = TotalError;
	}
}

void UPIDComponent::SetOpenLoop(bool newValue)
{
	bIsOpenLoop = newValue;
	if (bIsOpenLoop)
	{
		Zero();
	}
}

void UPIDComponent::Zero()
{
	P = I = D = D_avg = 0;
	Error_previous = TotalError_previous = Error;
	PV_avg = ProcessValue;
}

void UPIDComponent::ResetParametersToDefaults()
{
	Kp = Kp_default;
	Ki = Ki_default;
	Kd = Kd_default;
	OutputMin = OutputMin_default;
	OutputMax = OutputMax_default;
}



