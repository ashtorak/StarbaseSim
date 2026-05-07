// not used rn

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "StarbaseSimSettings.generated.h"

/**
 * 
 */
UCLASS()
class STARBASESIMLIBRARY_API UStarbaseSimSettings : public UGameUserSettings
{
	GENERATED_BODY()
	
public:
		UStarbaseSimSettings();

	UFUNCTION(BlueprintCallable)
		static UStarbaseSimSettings* GetStarbaseSimSettings();

	UFUNCTION(BlueprintCallable)
		float GetControllerParameter(FString device, FString controller, FString parameter) const;
	UFUNCTION(BlueprintCallable)
		void SetControllerParameter(FString device, FString controller, FString parameter, float InValue);

	UPROPERTY(Config)
		float BoosterControllerThrottleKp;
	UPROPERTY(Config)
		float BoosterControllerThrottleKi;
	UPROPERTY(Config)
		float BoosterControllerThrottleKd;
	UPROPERTY(Config)
		float BoosterControllerThrottleMin;
	UPROPERTY(Config)
		float BoosterControllerThrottleMax;

	UPROPERTY(Config)
		float BoosterControllerGimbalXKp;
	UPROPERTY(Config)
		float BoosterControllerGimbalXKi;
	UPROPERTY(Config)
		float BoosterControllerGimbalXKd;
	UPROPERTY(Config)
		float BoosterControllerGimbalXMin;
	UPROPERTY(Config)
		float BoosterControllerGimbalXMax;

	UPROPERTY(Config)
		float BoosterControllerGimbalYKp;
	UPROPERTY(Config)
		float BoosterControllerGimbalYKi;
	UPROPERTY(Config)
		float BoosterControllerGimbalYKd;
	UPROPERTY(Config)
		float BoosterControllerGimbalYMin;
	UPROPERTY(Config)
		float BoosterControllerGimbalYMax;

	UPROPERTY(Config)
		float BoosterControllerGimbalZKp;
	UPROPERTY(Config)
		float BoosterControllerGimbalZKi;
	UPROPERTY(Config)
		float BoosterControllerGimbalZKd;
	UPROPERTY(Config)
		float BoosterControllerGimbalZMin;
	UPROPERTY(Config)
		float BoosterControllerGimbalZMax;
};
