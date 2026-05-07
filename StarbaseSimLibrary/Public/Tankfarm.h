// see readme.txt

#pragma once

#include "Valve.h"
#include "CryoTank.h"
#include "CryoPump.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Tankfarm.generated.h"

UCLASS(ClassGroup = (Tanking))
class STARBASESIMLIBRARY_API ATankfarm : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATankfarm();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void AsyncPhysicsTickActor(float DeltaTime, float SimTime) override;

	// Override Replicate Properties function
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	void updateSubCoolers();

	void checkValveState();

	void TickAudio();

	void setSound(bool isValveOpen, TObjectPtr<class UAudioComponent> flowAudio, float flowrateAbsolute);

	float calculateFlowrate(TObjectPtr<class UCryoTank> source, TObjectPtr<class UCryoTank> destination, float valveOpening, TObjectPtr<class UCryoPump> pump, TObjectPtr<class UCryoTank> subCooler , bool useMethaneHeatExchanger);

	float _DeltaTime;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UValve> ValveBoosterOxidizer;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UValve> ValveBoosterFuel;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UValve> ValveShipOxidizer;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UValve> ValveShipFuel;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UValve> ValveFillSubcoolerOxidizer;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UValve> ValveFillSubcoolerFuel;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UValve> ValveFillSubcoolerOxidizerShip;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UValve> ValveFillSubcoolerFuelShip;
	
	UPROPERTY(BlueprintReadWrite, Replicated)
	bool isShipConnectedViaQD;

	UPROPERTY(BlueprintReadWrite, Replicated)
	bool isBoosterConnectedViaQD;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UCryoTank> OxidizerTank;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UCryoTank> NitrogenTank;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UCryoTank> FuelTank;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UCryoTank> SubcoolerFuel;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UCryoTank> SubcoolerOxidizer;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UCryoTank> SubcoolerFuelShip;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UCryoTank> SubcoolerOxidizerShip;
	
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UCryoTank> BoosterOxiTank;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UCryoTank> ShipOxiTank;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UCryoTank> BoosterFuelTank;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UCryoTank> ShipFuelTank;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UCryoPump> oxygenPump;
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UCryoPump> methanePump;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UCryoPump> oxygenPumpShip;
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UCryoPump> methanePumpShip;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UAudioComponent> flowAudioOxiPump;
	
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UAudioComponent> flowAudioFuelPump;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UAudioComponent> flowAudioOxiSubcoolerN2;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UAudioComponent> flowAudioFuelSubcoolerN2;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UAudioComponent> flowAudioOxiValveBooster;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UAudioComponent> flowAudioFuelValveBooster;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UAudioComponent> flowAudioOxiValveShip;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UAudioComponent> flowAudioFuelValveShip;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UAudioComponent> flowAudioQDOxiValveBooster;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UAudioComponent> flowAudioQDFuelValveBooster;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UAudioComponent> flowAudioQDOxiValveShip;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UAudioComponent> flowAudioQDFuelValveShip;

	float flowAudioVolume;


	/// <summary>
	/// in m³/s
	/// </summary>
	UPROPERTY(BlueprintReadWrite)
	float maxFlowrateForSound = 10.0f;
	UPROPERTY(BlueprintReadWrite, Replicated)
	float totalFlowrate = 0.0f;
	UPROPERTY(BlueprintReadWrite)
	float boosterFlowrateMethane;
	UPROPERTY(BlueprintReadWrite)
	float boosterFlowrateOxygen;
	UPROPERTY(BlueprintReadWrite)
	float shipFlowrateMethane;
	UPROPERTY(BlueprintReadWrite)
	float shipFlowrateOxygen;

	UPROPERTY(BlueprintReadWrite)
	float subcoolerOxygenFlowrate;
	UPROPERTY(BlueprintReadWrite)
	float subcoolerMethaneFlowrate;
	UPROPERTY(BlueprintReadWrite, Replicated)
	bool subcoolerOxygenFillAutomaticControl;
	UPROPERTY(BlueprintReadWrite, Replicated)
	bool subcoolerMethaneFillAutomaticControl;

	UPROPERTY(BlueprintReadWrite)
	float subcoolerOxygenFlowrateShip;
	UPROPERTY(BlueprintReadWrite)
	float subcoolerMethaneFlowrateShip;
	UPROPERTY(BlueprintReadWrite, Replicated)
	bool subcoolerOxygenFillAutomaticControlShip;
	UPROPERTY(BlueprintReadWrite, Replicated)
	bool subcoolerMethaneFillAutomaticControlShip;

	UFUNCTION(BlueprintCallable)
	void operateValve(FString valve) const;
	
	float subCoolerIcingMethaneFactor = 0.15f;
	float subCoolerEfficiencyFlowrateFactor = 0.02f;
	float subCoolerNominalLiquidFraction = 0.6f;
	float subCoolerNominalLiquidFractionHysteresis = 0.1f;
	float icingSpeed = 0.00005f;

	float mechanicalResistance;
	float pressureHead;
	float pumpPressure;

	
};
