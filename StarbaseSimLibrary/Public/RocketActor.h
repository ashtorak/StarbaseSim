
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CryoTank.h"
#include "RocketActor.generated.h"

class UNiagaraComponent;
class UAudioComponent;
class URocketControllerComponent;
class UAeroDynamicsComponent;

UCLASS(ClassGroup = (Rocket))
class STARBASESIMLIBRARY_API ARocketActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARocketActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;



public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void AsyncPhysicsTickActor(float DeltaTime, float SimTime) override;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<class URocketControllerComponent> RocketControllerComp;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<class UAeroDynamicsComponent> AeroDynamicsComponent;

	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class USkeletalMeshComponent> MainMeshComponent;

	// This is used for attitude controllers and to get altitude .
	// replicated to not have jumps in control panel values when the actual transform gets updated on client
	UPROPERTY(BlueprintReadWrite, replicated)
		FTransform  MainMeshTransform;

	// c++ variable added way later, gets value from BP
	UPROPERTY(BlueprintReadWrite)
	FString IDstring = "";

	UPROPERTY(BlueprintReadWrite)
		TArray<UMaterialInstanceDynamic*> DynMatsMainTanks;

	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class UCryoTank> oxidizerTank;
	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class UCryoTank> fuelTank;

	const static float oxidizerRatioMethaLOX;
	UPROPERTY(BlueprintReadWrite)
		float oxidizerRatio = oxidizerRatioMethaLOX;
	const static float fuelRatioMethaLOX;
	UPROPERTY(BlueprintReadWrite)
		float fuelRatio = fuelRatioMethaLOX;


	// north, east, up in m
	UPROPERTY(BlueprintReadWrite)
		FVector CurrentPosition;

	UPROPERTY(BlueprintReadOnly)
		float Altitude = 0.0f;

	UPROPERTY(BlueprintReadOnly)
		float DensityAir = 1.0f;

	// in m/s
	UPROPERTY(BlueprintReadWrite, replicated)
		FVector Velocity;

	UPROPERTY(BlueprintReadOnly)
		float VelocityMag = 0.0f;

	UPROPERTY(BlueprintReadWrite)
		FVector VelocityPrevious;

	// in m/s²
	UPROPERTY(BlueprintReadWrite)
		FVector Acceleration;

	UPROPERTY(Category = "Physics", EditAnywhere, BlueprintReadWrite)
		float AccelerationSmoothingFactor = 0.33f;

	// in m/s²
	//UPROPERTY(BlueprintReadWrite)
	FVector Gravity = FVector(0, 0, -9.81);

	// in m/s² - body acceleration = acceleration - gravity in body frame
	UPROPERTY(Category = "Physics", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "BodyAcceleration - gravity in body frame", ForceUnits = "m/s²"))
		FVector BodyAcceleration;

	// in m/s² - body acceleration = acceleration - gravity in body frame
	UPROPERTY(BlueprintReadWrite)
		float BodyAccelerationMagnitude;



	// default is for ship, set this in blueprint for each rocket type
	UPROPERTY(BlueprintReadWrite)
		int NumberOfEngines = 6;

	UPROPERTY(BlueprintReadOnly)
		bool AreEnginesReleased = false;


	float propellantLoadingRateInTPerS = 20.0f;
	
	bool isOpenValveVentFuel, isOpenValveVentOxidizer;
	UPROPERTY(Category = "Rocket", BlueprintReadWrite, Replicated, meta = (ToolTip = "simple on/off valve"))
		bool isOpenValveRaptorChill = false;
	float pressureFuelTank, pressureOxidizerTank;
	float nominalPressure;
	float tankBurstPressure;
	float selfPressurizationRate = 0.01f;

	UPROPERTY(Category = "Rocket", BlueprintReadWrite, Replicated, meta = (ToolTip = "uses chill valve"))
	bool bDumpOxidizer = false;

	UPROPERTY(Category = "Rocket", BlueprintReadWrite, Replicated, meta = (ToolTip = "uses chill valve"))
	bool bDumpFuel = false;

	UPROPERTY(Category = "Rocket", BlueprintReadWrite, Replicated, meta = (ToolTip = "uses chill valve"))
	float DumpSpeed = 22.0f;

	UPROPERTY(Category = "Rocket", BlueprintReadOnly, Replicated, meta = (ToolTip = ""))
		float turbopumpTemperature;
	float turbopumpTemperatureTarget = 85.0f; // will be set to LOX tank temp + 5
	float turbopumpTemperatureTargetNominal = 85.0f;
	float turbopumpTemperatureTargetMargin = 25.0f;
	UPROPERTY(Category = "Rocket", BlueprintReadOnly, meta = (ToolTip = "above turbopump temperature plus that likelihood for explosion increases when starting engines"))
		float turbopumpTemperatureTargetPlusMargin = turbopumpTemperatureTargetNominal + turbopumpTemperatureTargetMargin;
	

	UPROPERTY(BlueprintReadWrite)
		UNiagaraComponent* condensationEffect;

	UPROPERTY(BlueprintReadWrite)
		UNiagaraComponent* ventEngineChillEffect;

	UPROPERTY(BlueprintReadWrite)
		UAudioComponent* ventEngineChillAudio;

	UPROPERTY(BlueprintReadWrite)
		float steamRocketVelocityAdjustment = 90.0f;
	UPROPERTY(BlueprintReadWrite)
		float steamSizeAdjustment = 1.0f;

	float scalingFactor = 0;
	
	float ventEngineChillAudioVolume = 0.0f;

	UPROPERTY(Category = "Rocket", BlueprintReadWrite, meta = (ToolTip = "default is for Ship block 1"))
		float iceOxidizerHeight = 1260.0f;
	UPROPERTY(Category = "Rocket", BlueprintReadWrite, meta = (ToolTip = "default is for Ship block 1"))
		float iceOxidizerStart = -2970.0f;
	float currentIceHeight;
	UPROPERTY(Category = "Rocket", BlueprintReadWrite, meta = (ToolTip = "default is for Ship block 1"))
		float iceFuelHeight = 525.0f;
	UPROPERTY(Category = "Rocket", BlueprintReadWrite, meta = (ToolTip = "default is for Ship block 1"))
		float iceFuelStart = -1680.0f;


	FVector EffectDragForce;


	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
		void ExplodeFromCPP();

	void explode();

	UPROPERTY(BlueprintReadOnly)
		bool isExploded = false;


	UPROPERTY(BlueprintReadOnly)
		float totalLiquidMass = 0.0f;
	UPROPERTY(BlueprintReadOnly)
		float totalGasMass = 0.0f;
	UPROPERTY(BlueprintReadOnly)
		float totalPropMass = 0.0f; // is used by RocketControllerComponent to update physics mass
	
	UFUNCTION(BlueprintCallable)
		void setPropellantDirectly(float value_in_t);

	void UpdateTanksAfterFiring(float TotalMassDelta);

	void UpdateMass();

	void UpdatePhysicsState(float dT);

	void EngineChill(float dT);

	void EngineChillEffects(float dT);

	void TankEffects();
};

