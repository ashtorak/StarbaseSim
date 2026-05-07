// see readme.txt

#pragma once

#include "Valve.h"
#include "CryoTank.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TankFarmMassey.generated.h"

UCLASS(ClassGroup = (Tanking))
class STARBASESIMLIBRARY_API ATankFarmMassey : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATankFarmMassey();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void AsyncPhysicsTickActor(float DeltaTime, float SimTime) override;

	// Override Replicate Properties function
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class UValve> ValveShipOxidizer;

	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class UValve> ValveShipFuel;

	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class UValve> ValveFill;
	
	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class UValve> ValveDrain;

	// m³/s, for Fill and Drain
	UPROPERTY(BlueprintReadWrite, Replicated)
		float VolumetricFlowRate = 2.0f;

	UPROPERTY(BlueprintReadWrite, Replicated)
		bool isShipConnectedViaQD;

	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class UCryoTank> OxidizerTank;

	// fill temperature
	UPROPERTY(BlueprintReadWrite)
		float OxidizerTemperature = 79.0f;

	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class UCryoTank> FuelTank;

	// fill temperature
	UPROPERTY(BlueprintReadWrite)
		float FuelTemperature = 90.0f;

	UPROPERTY(BlueprintReadWrite)
		UAudioComponent* flowAudio;

	float flowAudioVolume;

	UPROPERTY(BlueprintReadWrite)
		float maxFlowrateForSound = 10.0f;

	UPROPERTY(BlueprintReadWrite, Replicated)
		float totalFlowrate = 0.0f;

	float Flowrate = 0.0f;

	UFUNCTION(BlueprintCallable)
		void operateValve(FString valve);
};

