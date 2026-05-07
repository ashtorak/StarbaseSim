// see readme.txt

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CryoTank.generated.h"

class UNiagaraComponent;
class UAudioComponent;
class ARocketActor;

UCLASS(Blueprintable, ClassGroup=(Tanking), meta=(BlueprintSpawnableComponent) )
class STARBASESIMLIBRARY_API UCryoTank : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCryoTank();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	virtual void AsyncPhysicsTickComponent(float DeltaTime, float SimTime) override;

protected:
	int i = 0;
	float _DeltaTime = 0.016666f;
	bool isInitialized = false;
	TObjectPtr<class ARocketActor> Rocket;

public:

	UFUNCTION(BlueprintCallable)
		void ini();

	UPROPERTY(Category = "set up these values", EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "Number of Tanks - all calculated values are for total capacity of all tanks"))
		int numberOfTanks = 1;
	UPROPERTY(Category = "set up these values", EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "diameter of single cylindrical tank in m, has to be set before and should not be changed during runtime"))
		float diameter = 0.0f;
	UPROPERTY(Category = "set up these values", EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "height of single cylindrical tank in m, has to be set before and should not be changed during runtime"))
		float height = 0.0f;
	UPROPERTY(Category = "set up these values", EditAnywhere, BlueprintReadWrite)
		bool isHorizontal = false;
	UPROPERTY(Category = "set up these values", EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "Set capacity of all tanks directly. If not zero, it will be used instead of diameter and height. But height will be calculated with diameter for level"))
		float directCapacity = 0.0f;

	/// Thermal resistance calculation:
	/// In reality thermal resistance depends on a lot of factors and is really hard to calculate. So we make some huge simpflications here by 
	/// assuming a constant base resistance and then in parallel a dynamic component based on the height of the liquid.

	/// <summary>
	/// Thermal conductance from liquid to ambient air per meter of liquid height in Watt*meter/MegaKelvin (for easier handling)
	/// The inverse is mutlitplied with the tank height to get the thermal resistance when the tank is full with liquid.
	/// </summary>
	float G_th_per_m_SingleTank = 0.0025f; // default value for rocket tanks

	/// <summary>
	/// Thermal resistance base value, which is constant for a tank in MegaKelvin / (Watt*meter) (for easier handling)
	/// </summary>
	UPROPERTY(Category = "set up these values", EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "Thermal resistance base value, which is constant for a tank in MegaKelvin / (Watt*meter)"))
	float RthBaseSingleTank = 22.0f; // default value for rocket tanks

	/// <summary>
	/// Rth is multiplied by this which should be 100 for GSE
	/// </summary>
	UPROPERTY(Category = "set up these values", EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "Rth is multiplied by this which should be 100 for GSE"))
		float RthGSEmultiplier = 1.0f;

	/// <summary>
	/// If liquid volume fraction is above this value, liquid will be vented directly
	/// </summary>
	UPROPERTY(Category = "set up these values", EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "If liquid volume fraction is above this value, liquid will be vented directly, 0.99 for GSE"))
		float liquidVentingThresholdDefault = 0.97f;
	float liquidVentingThreshold;

	UPROPERTY(Category = "set up these values", EditAnywhere, BlueprintReadWrite, meta = (ToolTip = ""))
	float nominalPressureG = 6.0f;
	float pressureGSetpoint = 6.0f;

	UPROPERTY(Category = "set up these values", EditAnywhere, BlueprintReadWrite, meta = (ToolTip = ""))
		float valveOpeningSpeed = 0.05f;
	UPROPERTY(Category = "set up these values", EditAnywhere, BlueprintReadWrite, meta = (ToolTip = ""))
		float ventingCoefficient = 1;
	UPROPERTY(Category = "set up these values", EditAnywhere, BlueprintReadWrite, meta = (ToolTip = ""))
		float ventingCoefficientLiquid = 1;

	//UPROPERTY(Category = "set up these values", EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "This is used to find inlet height with inletOffset as well as altitude for ambient pressure calc"))
	UPROPERTY(Category = "set up these values", EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "this is for inletHeight calculation using mesh pos minus offset"))
		float inletOffset = 0.0f;
	UPROPERTY(Category = "set up these values", EditAnywhere, BlueprintReadWrite, meta = (ToolTip = ""))
		float inletDiameter = 0.4f;

	float adiabaticIndex = 1.4f; // adiabatic index is roughly the same for all compounds for simplicity

	////////////////////////////
	// vents
	////////////////////////////

	UPROPERTY(BlueprintReadWrite)
		UNiagaraComponent* ventEffect;
	
	UPROPERTY(BlueprintReadWrite)
		UAudioComponent* ventAudio;

	UPROPERTY(BlueprintReadWrite)
		float pressureScalerMultiplier = 1.0f;

	float pressureScaler;
	float volumeScaler;
	
	UPROPERTY(BlueprintReadWrite)
		float steamRocketVelocityAdjustment = 90.0f;

	//[Tooltip("for subcoolers the steam life time scales with pressure only, else with pressure AND valveVentOpening")]
	UPROPERTY(BlueprintReadWrite)
		float steamLifetimeAdjustment = 1.0f;
	UPROPERTY(BlueprintReadWrite)
		float steamSizeAdjustment = 1.0f;

	//[Tooltip("for subcoolers the steam speed scales with pressure only, else with pressure AND valveVentOpening")]
	UPROPERTY(BlueprintReadWrite)
		float steamSpeedAdjustment = 0.5f;

	//[Tooltip("for subcoolers the emission rate scales with speed, else it is constant")]
	UPROPERTY(BlueprintReadWrite)
		float steamEmissionAdjustment = 1.0f;
	UPROPERTY(BlueprintReadWrite)
		float steamVolumeFactor = 10.0f;

	UPROPERTY(BlueprintReadWrite)
		float sizeLowerLimit = 0.5f;
	UPROPERTY(BlueprintReadWrite)
		float speedLowerLimit = 0.2f;
	UPROPERTY(BlueprintReadWrite)
		float emissionLowerLimit = 0.5f;

		float ventAudioVolume = 0.0f;
	UPROPERTY(BlueprintReadWrite)
		float ventAudioVolumeMultiplier = 1.0f;

	bool isVenting, isVentSoundLooping;
	float ventDelay;

	void updateVentValveEffects();


	float domeHeight = 1.715f;
	float domeCurveFactor = 0.45f;
	float domeVolume;
	float domeVolumeFraction;

	UPROPERTY(Category = "set up these values", EditAnywhere, BlueprintReadWrite, meta = (ToolTip = ""))
		float startingMassInT = 0.0f;
	UPROPERTY(Category = "set up these values", EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "if 0, it will set it to ambient temperature"))
		float startingTemperature = 0.0f;


	UPROPERTY(Category = "GSE Setup", EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "Set GSE status before, don't change it on the fly or some values won't change"))
		bool isGSE = false;

	UPROPERTY(Category = "GSE", BlueprintReadWrite, Replicated)
		bool fillUpGSETank = false;

	UPROPERTY(Category = "GSE Setup", EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "single tanker mass in kg, will be increased by 5% for oxygen/nitrogen and reduced by 5% for methane"))
		float tankerMass = 20000.0f;
	UPROPERTY(Category = "GSE Setup", EditAnywhere, BlueprintReadWrite, meta = (ToolTip = ""))
		int tankersPerHour = 100;
	UPROPERTY(Category = "GSE Setup", EditAnywhere, BlueprintReadWrite, meta = (ToolTip = ""))
		float GSEpressureGSetPoint = 5.0f;


	UPROPERTY(Category = "Subcooler Setup", EditAnywhere, BlueprintReadWrite, meta = (ToolTip = ""))
		bool isSubcooler = false;

	int subCoolerAdditionalUpdates = 20;

	/// <summary>
	/// If liquid volume fraction is above this value, liquid will be vented directly
	/// </summary>
	UPROPERTY(Category = "Subcooler Setup", EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "If liquid volume fraction is above this value, liquid will be vented directly"))
		float liquidVentingThresholdSubcooler = 0.8f;

	UPROPERTY(BlueprintReadOnly, Replicated)
		float outTemp = 1.0f;
	//[Header("Subcooler internal values")]
	float efficiency = 1.0f;
	float efficiencyIceReduction = 1.0f;


	UPROPERTY(Category = "CryoTank UI", EditAnywhere, BlueprintReadWrite, Replicated, meta = (ToolTip = ""))
		bool isExploded;

	//[Header("calculated and other values")]
	bool isBoiling;

	bool isFullWithLiquid = false;
	bool isLiquidEmpty = false;;

	float surfaceArea;

	/// <summary>
	/// total capacity for all tanks in m³
	/// </summary>
	float capacity = 0.0f;
	float capacitySingleTank;


	/// <summary>
	/// goes true if valveVentOpening > 0
	/// </summary>
	UPROPERTY(Category = "CryoTank UI", EditAnywhere, BlueprintReadWrite, Replicated, meta = (ToolTip = "goes true if valveVentOpening > 0"))
		bool isOpenValveVent;

	/// <summary>
	/// true if opening <0.01 and > 0.99
	/// </summary>
	UPROPERTY(Category = "CryoTank UI", EditAnywhere, BlueprintReadWrite, Replicated, meta = (ToolTip = "true if opening <0.01 and > 0.99"))
		bool isValveVentPartiallyOpen;

	UPROPERTY(Category = "CryoTank UI", EditAnywhere, BlueprintReadWrite, Replicated, meta = (ToolTip = "0..1"))
		float valveVentNewSetpoint = 0.0f;
	UPROPERTY(Category = "CryoTank UI", EditAnywhere, BlueprintReadWrite, Replicated, meta = (ToolTip = "0..1"))
		float valveVentOpening = 0.0f;

	UPROPERTY(Category = "CryoTank UI", EditAnywhere, BlueprintReadWrite, Replicated, meta = (ToolTip = ""))
		bool isOnAutoPressureControl = true;

	/// <summary>
	/// in bar(a)
	/// </summary>
	float ambientPressure;
	UPROPERTY(Category = "CryoTank state values", EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "in bar(a)"))
		float gasPressure;

	/// <summary>
	/// for multiplying norminal pressure with Random.Range(1.05f, 1.15f), done in constructor
	/// </summary>
	float tankBurstPressureMultiplier = 1.0f;

	/// <summary>
	/// this is actually only calculated for methane subcooler
	/// </summary>
	UPROPERTY(Category = "CryoTank UI", BlueprintReadOnly, Replicated)
		float internalIceThickness;


	//[Header("Compound Properties")]

	/// <summary>
	/// default is oxygen, other available are methane and nitrogen
	/// </summary>
	UPROPERTY(Category = "Compound Properties", EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "default is oxygen, other available are methane and nitrogen"))
		FString compound = "oxygen";

	// calculated

	/// <summary>
	/// gas pressure in bar(g)
	/// </summary>
	UPROPERTY(Category = "CryoTank state values", EditAnywhere, BlueprintReadWrite, Replicated, meta = (ToolTip = "in bar(g)"))
		float pressureG = 0.0f;

	/// <summary>
	/// hydrostatic pressure in bar(g)
	/// </summary>
	float pressureHydroStaticG;

	float liquidLevel;

	UPROPERTY(Category = "CryoTank state values", EditAnywhere, BlueprintReadWrite, Replicated, meta = (ToolTip = ""))
		float liquidLevelNormalized = 0.0f;

	float liquidVolumeFraction;

	float liquidVolume, gasVolume;

	UPROPERTY(Category = "CryoTank state values", EditAnywhere, BlueprintReadWrite, Replicated, meta = (ToolTip = "in kg"))
		float liquidMass;
	UPROPERTY(Category = "CryoTank state values", EditAnywhere, BlueprintReadWrite, Replicated, meta = (ToolTip = "in kg"))
		float gasMass;
	UPROPERTY(Category = "CryoTank state values", EditAnywhere, BlueprintReadWrite, Replicated, meta = (ToolTip = "in K"))
		float liquidTemperature;
	UPROPERTY(Category = "CryoTank state values", EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "in K"))
		float gasTemperature;

	float liquidDensity, gasDensity, gasDensityAt1bar;

	/// <summary>
	/// heat capacity in kJ/kg
	/// </summary>
	float liquidHeatCapacity;

	/// <summary>
	/// in bar(a)
	/// </summary>
	float saturationPressure;

	/// <summary>
	/// heat capacity in kJ/kg
	/// </summary>
	float heatOfVaporization;

	// given

	/// <summary>
	/// in Pascal
	/// </summary>
	float criticalPressure;

	float criticalTemperature;

	/// <summary>
	/// in g/mol
	/// </summary>
	float molarMass;

	float acentricFactor;

	float CoeffPSat1, CoeffPSat2, CoeffPSat3;

	/// <summary>
	/// in J/(mol*kg)
	/// </summary>
	const float gasConstant = 8.314462618f;


	//[Header("internal stuff")]

	/// <summary>
	/// thermal resistance in MW/K
	/// </summary>
	float RthCurrentLiquidLevel, Rth;

	/// <summary>
	/// flow into tank is positive, out of tank negative
	/// </summary>
	float liquidMassExternalDelta, gasVolumeExternalDelta, heatExternalDelta;
	float liquidMassExternalTemperature;

	float deltaHeatInkJ, heatFlash;
	// smooth deltaHeat over a couple of updates
	float deltaHeatList[5];
	int heatListIterator = 0;

	float deltaValuesSmoothingFactor = 5;

	float SatTempAtCurrentPressure;

	float evapMass;

	/// <summary>
	/// positive means it's added to gas volume
	/// </summary>
	float evapVolume, ventVolume, liquidVolumeDiff, deltaVolume;

	// smooth deltaVolume over a couple of updates
	float deltaVolumeList[7];
	int volumeListIterator = 0;

	bool hasPropBeenSetDirectly;

	

	float altitude;

	void setCompoundParameters(FString newCompound);

	void updatePropellant(float deltaTime);
		
	void addLiquidMass(float mass_in_kg, float temperature);

	bool removeLiquidMass(float mass_in_kg);

	void addGasVolumeExternalDelta(float value);

	void addHeat(float value);

	UFUNCTION(BlueprintCallable)
		void setPropellantDirectly(float value_kg);

	UFUNCTION(BlueprintCallable)
		void setMassDirectly(float value_in_t, float temperature);

	void nullLists();

	void fillUp();


	void explode();

	float getInletHeightWorld();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
		void ExplodeFromCPP();
};
