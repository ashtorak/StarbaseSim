// Fill out your copyright notice in the Description page of Project Settings.


#include "StarbaseSimSettings.h"

UStarbaseSimSettings::UStarbaseSimSettings()
{


}

UStarbaseSimSettings* UStarbaseSimSettings::GetStarbaseSimSettings()
{
	return GEngine ? CastChecked<UStarbaseSimSettings>(GEngine->GetGameUserSettings()) : nullptr;
}

float UStarbaseSimSettings::GetControllerParameter(FString device, FString controller, FString parameter) const
{
	return BoosterControllerGimbalXKp;
}

void UStarbaseSimSettings::SetControllerParameter(FString device, FString controller, FString parameter, float InValue)
{
	BoosterControllerGimbalXKp = InValue;
}