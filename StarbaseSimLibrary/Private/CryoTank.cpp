// see readme.txt


#include "CryoTank.h"
#include "Math/UnrealMathUtility.h"
#include "Kismet/KismetMathLibrary.h"
#include "AsyncTickFunctions.h"
#include "StarbaseSimCommon.h"
#include <Net/UnrealNetwork.h>
#include "NiagaraComponent.h"
#include "Components/AudioComponent.h"
#include "RocketActor.h"
#include "RocketControllerComponent.h"
#include "Settings/LyraSettingsLocal.h"

// Sets default values for this component's properties
UCryoTank::UCryoTank()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	SetAsyncPhysicsTickEnabled(true);

	SetIsReplicatedByDefault(true);

	Rocket = Cast<ARocketActor>(GetOwner());

}

//  needed for replication
void UCryoTank::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCryoTank, isExploded);
	DOREPLIFETIME(UCryoTank, isOpenValveVent);
	DOREPLIFETIME(UCryoTank, isValveVentPartiallyOpen);
	DOREPLIFETIME(UCryoTank, valveVentOpening);
	DOREPLIFETIME(UCryoTank, isOnAutoPressureControl);
	DOREPLIFETIME(UCryoTank, internalIceThickness);
	DOREPLIFETIME(UCryoTank, pressureG);
	DOREPLIFETIME(UCryoTank, liquidLevelNormalized);
	DOREPLIFETIME(UCryoTank, liquidMass);
	DOREPLIFETIME(UCryoTank, gasMass);
	DOREPLIFETIME(UCryoTank, liquidTemperature);
	DOREPLIFETIME(UCryoTank, fillUpGSETank);
	DOREPLIFETIME(UCryoTank, valveVentNewSetpoint);
	DOREPLIFETIME(UCryoTank, outTemp);
}

// Called when the game starts
void UCryoTank::BeginPlay()
{
	Super::BeginPlay();

}

void UCryoTank::ini()
{
	tankBurstPressureMultiplier = FMath::RandRange(1.05f, 1.15f); // have a different burst pressure for each instance

	domeVolume = numberOfTanks * UE_PI * diameter * diameter / (4 * (1 + 2 * domeCurveFactor)) * domeHeight; // when d = (h/h_Dome)^domeCurveFactor * d_Tank
	surfaceArea = UE_PI * diameter * diameter / 4;
	if (directCapacity > 0) // only used for vertical rocket tanks
	{
		capacity = directCapacity;
		height = (capacity - 2 * domeVolume) / surfaceArea / (float)numberOfTanks + 2 * domeHeight;
	}
	else if (isHorizontal) capacity = surfaceArea * height * numberOfTanks;
	else capacity = surfaceArea * (height - 2 * domeHeight) * numberOfTanks + 2 * domeVolume;

	domeVolumeFraction = domeVolume / capacity;
	gasVolume = capacity;
	capacitySingleTank = capacity / (float)numberOfTanks;

	pressureG = 0;
	liquidMass = 0;
	liquidTemperature = getAmbientTemperature();

	if (isGSE || isSubcooler)
	{
		altitude = 11;
	}
	else if(Rocket)
	{
		Rocket->UpdatePhysicsState(1); // dT doesn't matter here as it's used for acceleration calculation only anyway
		// note: updating here also makes sure that the rocket has an altitude value later for the tick function
		altitude = Rocket->Altitude;
	}
	
	setCompoundParameters(compound);

	nullLists();

	if (startingMassInT > 0)
	{
		if (startingTemperature==0) startingTemperature = getAmbientTemperature();
		setMassDirectly(startingMassInT, startingTemperature);
	}

	outTemp = getAmbientTemperature();

	isInitialized = true;
}


// Called every frame
void UCryoTank::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}


void UCryoTank::setCompoundParameters(FString newCompound)
{
	if (liquidVolume < capacity * 0.1f && pressureG < 0.1f * nominalPressureG)
	{// don't allow switching compound when there is a lot of stuff in the tank
		if (compound == "oxygen")
		{
			criticalPressure = 5043000;
			criticalTemperature = 155;
			molarMass = 32;
			acentricFactor = 0.022f;
			CoeffPSat1 = 3.9523f;
			CoeffPSat2 = 340.024f;
			CoeffPSat3 = 4.144f;
			liquidHeatCapacity = 1.7f;
			gasDensityAt1bar = 2.0f;
		}
		else if (compound == "nitrogen")
		{
			criticalPressure = 3395800;
			criticalTemperature = 126;
			molarMass = 28.013f;
			acentricFactor = 0.0372f;
			CoeffPSat1 = 3.7362f;
			CoeffPSat2 = 264.651f;
			CoeffPSat3 = 6.788f;
			liquidHeatCapacity = 2.05f;
			gasDensityAt1bar = 5.0f;
		}
		else if (compound == "methane")
		{
			criticalPressure = 4600000;
			criticalTemperature = 190.6f;
			molarMass = 16.043f;
			acentricFactor = 0.0114f;
			CoeffPSat1 = 3.80235f;
			CoeffPSat2 = 403.106f;
			CoeffPSat3 = 5.479f;
			liquidHeatCapacity = 3.5f;
			gasDensityAt1bar = 4.0f;
		}
	}

	ambientPressure = (isGSE || isSubcooler) ? 1 : FMath::Exp(-altitude / 10000.0f);
}

void UCryoTank::AsyncPhysicsTickComponent(float DeltaTime, float SimTime)
{
	Super::AsyncPhysicsTickComponent(DeltaTime, SimTime);

	if (isInitialized)
	{
		if (Rocket)
		{
			if (Rocket->RocketControllerComp->isFullySwitchedOff) return;
			altitude = Rocket->Altitude;
		}

		_DeltaTime = DeltaTime;

		// only run on server, UI relevant properties are replicated
		if (GetNetMode() == ENetMode::NM_Standalone || GetNetMode() == ENetMode::NM_ListenServer)
		{
			// do more iterations fo subcoolers to reduce pressure spikes and oscillations
			if (isSubcooler)
			{
				float _deltaTime = DeltaTime / (float)subCoolerAdditionalUpdates;
				for (i = 0; i < subCoolerAdditionalUpdates; i++)
				{
					updatePropellant(_deltaTime);
				}
			}
			else updatePropellant(DeltaTime);


			if (isExploded)
			{

			}
			else // do this, if not exploded
			{
				valveVentOpening = valveVentOpening - (valveOpeningSpeed * (valveVentOpening - valveVentNewSetpoint));
				if (valveVentOpening < 0.0001f) valveVentOpening = 0;
				else if (valveVentOpening > 0.9999f) valveVentOpening = 1;

				isOpenValveVent = valveVentOpening > 0.01f ? true : false;
				isValveVentPartiallyOpen = valveVentOpening > 0.01f && valveVentOpening < 0.99 ? true : false;


				if (isOnAutoPressureControl)
				{
					if (isGSE)
					{
						if (pressureG > 0.95f * nominalPressureG) valveVentNewSetpoint = 1;
						else if (isOpenValveVent && pressureG < 0.9f * nominalPressureG) valveVentNewSetpoint = 0;
					}
					else
					{
						if (pressureG > 1.0f * pressureGSetpoint || pressureG < -0.01f * nominalPressureG) valveVentNewSetpoint = 1;
						else if (isOpenValveVent && pressureG < 0.9f * pressureGSetpoint) valveVentNewSetpoint = 0;
					}
				}


				if (isGSE)
				{
					if (fillUpGSETank)
					{
						if (!isFullWithLiquid) fillUp();
						else fillUpGSETank = false;
					}
				}

				if (isGSE || isSubcooler)
				{
					/*if (collisionImpulseMax > collisionImpulseLimit)
					{
						explode(true);
					}*/
				}

				if (pressureG > tankBurstPressureMultiplier * nominalPressureG || pressureG < -0.4f * tankBurstPressureMultiplier)
				{
					ULyraSettingsLocal* GameSettings = ULyraSettingsLocal::Get();
					if (GameSettings->IsOnDestruction) explode();
					else pressureG = nominalPressureG;
				}

				if (liquidMass == 0) // if empty, increase the temperature value so that it feels like there is temperature sensor which warms up again
				{
					liquidTemperature += (getAmbientTemperature() - liquidTemperature) * 0.0003f * _DeltaTime;
				}
			}
		}


		if (!isGSE && ventEffect && ventAudio) updateVentValveEffects();

	}
}

/// <summary>
/// calculates new liquid and gas mass and volume
/// </summary>
void UCryoTank::updatePropellant(float deltaTime)
{
	float newAmbientPressure = (isGSE || isSubcooler) ? 1.0f : FMath::Exp(-altitude / 10000.0f);
	float ambientPressureDelta = ambientPressure - newAmbientPressure;
	ambientPressure = newAmbientPressure;

	if (!isExploded)
	{
		if (isGSE)
		{
			pressureG = GSEpressureGSetPoint;
			gasVolume = capacity - liquidVolume;
		}

		pressureG += ambientPressureDelta;
		gasPressure = pressureG + ambientPressure;
		gasDensity = gasDensityAt1bar * gasPressure;
		gasMass = gasDensity * gasVolume;
		// calculate some material parameters at current temperature

		saturationPressure = FMath::Pow(10, CoeffPSat1 - CoeffPSat2 / (liquidTemperature - CoeffPSat3));
		// after Antoine from http://webbook.nist.gov

		if (liquidTemperature < criticalTemperature)
		{
			heatOfVaporization = (7.08f * FMath::Pow(1 - liquidTemperature / criticalTemperature, 0.354f) + 10.95f * acentricFactor * FMath::Pow(1 - liquidTemperature / criticalTemperature, 0.456f)) * gasConstant * criticalTemperature / molarMass;
			// after Pitzer from https://chemicals.readthedocs.io/chemicals.phase_change.html#heat-of-vaporization-at-t-correlations

			liquidDensity = molarMass / 1000.0f / (gasConstant * criticalTemperature / criticalPressure * FMath::Pow((0.29056f - 0.08775f * acentricFactor), (1 + FMath::Pow(1 - liquidTemperature / criticalTemperature, 2 / 7.0f))));
			// after Yamada_Gunn from https://chemicals.readthedocs.io/chemicals.volume.html#pure-low-pressure-liquid-correlations
		}
		else // is typically not happening in our case, so just put some values
		{
			heatOfVaporization = 1000;
			liquidDensity = 1000;
		}

		// calculate liquid properties

		liquidVolume = liquidMass / liquidDensity;
		liquidVolumeFraction = liquidVolume / capacity;
		if (liquidVolume <= 0)
		{
			liquidLevel = liquidLevelNormalized = 0;
		}
		else if (isHorizontal)
		{
			liquidLevelNormalized = FMath::Asin(2 * liquidVolumeFraction - 1) / UE_PI + 0.5f;
			liquidLevel = liquidLevelNormalized * diameter;
		}
		else
		{
			if (liquidVolume < domeVolume) liquidLevel = FMath::Pow(liquidVolumeFraction * capacity / numberOfTanks * 4 * (1 + 2 * domeCurveFactor) / UE_PI * FMath::Pow(domeHeight, 0.84f) / (diameter * diameter), 1 / (1 + 2 * domeCurveFactor));
			else if (liquidVolume > capacity - domeVolume) liquidLevel = height - FMath::Pow(capacity * (1 - liquidVolumeFraction) / numberOfTanks * 4 * (1 + 2 * domeCurveFactor) / UE_PI * FMath::Pow(domeHeight, 2 * domeCurveFactor) / (diameter * diameter), 1 / (1 + 2 * domeCurveFactor));
			else liquidLevel = domeHeight + (liquidVolumeFraction - domeVolumeFraction) * (height - domeHeight);
			liquidLevelNormalized = liquidLevel / height;
		}

		pressureHydroStaticG = liquidLevel * liquidDensity * 9.81f/ 100000.0f;

		// if tank is full with liquid, release it directly
		liquidVentingThreshold = liquidVentingThresholdDefault;
		if (isSubcooler) liquidVentingThreshold = liquidVentingThresholdSubcooler;
		if (liquidVolumeFraction > liquidVentingThreshold)
		{
			isFullWithLiquid = true;
			if (isOpenValveVent)
			{
				if (isGSE) liquidVolume = liquidVentingThreshold * capacity;
				else liquidVolume -= valveVentOpening * ventingCoefficientLiquid * FMath::Clamp(pressureG, 1, 2) * numberOfTanks * deltaTime;
				liquidMass = liquidVolume * liquidDensity;
			}
		}
		else
		{
			isFullWithLiquid = false;
		}

		// heat flow into liquid and evaporation (no condensation for simplicity as it doesn't matter anyway in our application here)

		heatFlash = deltaHeatInkJ = evapMass = 0;

		if (liquidVolume > 0)
		{
			isLiquidEmpty = false;

			RthCurrentLiquidLevel = numberOfTanks * RthGSEmultiplier / liquidLevelNormalized / (G_th_per_m_SingleTank * height);
			// base and liquid level in parallel, increased by lower ambient pressure with min 10% of sea level value
			Rth = 1 / (1 / (RthBaseSingleTank / numberOfTanks * RthGSEmultiplier) + 1 / RthCurrentLiquidLevel) / FMath::Clamp(ambientPressure, 0.1f, 11.0f);

			deltaHeatInkJ = (getAmbientTemperature() - liquidTemperature) / (Rth / 1000.0f) * deltaTime + heatExternalDelta;
			heatExternalDelta = 0;

			// evaporation
			if (!isBoiling && gasPressure < saturationPressure) isBoiling = true;
			else isBoiling = false;

			if (isBoiling)
			{
				// calculate what evaporates due to lower pressure, in what is called "flash evaporation" (wiki and ISBN 9780852954317)
				SatTempAtCurrentPressure = CoeffPSat3 + CoeffPSat2 / (CoeffPSat1 - FMath::LogX(10.0f, gasPressure));

				float volumeAffectedByFlashEvaporation = 0; // only evaporate mass down to 2 m below the liquid level - how much sense does this actually make?
				if (isHorizontal) volumeAffectedByFlashEvaporation = FMath::Clamp(liquidVolume, 0, 2 / diameter * capacity);
				else volumeAffectedByFlashEvaporation = FMath::Clamp(liquidVolume, 0, 2 * surfaceArea);

				evapMass = volumeAffectedByFlashEvaporation * liquidDensity * liquidHeatCapacity * (liquidTemperature - SatTempAtCurrentPressure) / heatOfVaporization;
				evapVolume = evapMass / gasDensity;
				heatFlash = gasPressure * 100000 * evapVolume / (adiabaticIndex - 1) * liquidTemperature / SatTempAtCurrentPressure * (1 - FMath::Pow(gasPressure / saturationPressure, (adiabaticIndex - 1) / adiabaticIndex));

				// remove heatFlash from incoming delta heat
				deltaHeatInkJ -= heatFlash;
			}

			// smooth deltaHeat
			deltaHeatList[heatListIterator] = deltaHeatInkJ / deltaValuesSmoothingFactor;
			if (heatListIterator < 4) heatListIterator++;
			else heatListIterator = 0;
			deltaHeatInkJ = 0;
			for (float value : deltaHeatList)
			{
				deltaHeatInkJ += value;
			}

			float deltaHeatEvap = 0;
			if (deltaHeatInkJ > 0)
			{
				deltaHeatEvap = FMath::Pow(FMath::Clamp(saturationPressure / gasPressure,0.0f,1.0f), 2) * deltaHeatInkJ;
				evapMass += deltaHeatEvap / heatOfVaporization;
			}
			liquidTemperature = FMath::Clamp(liquidTemperature + (deltaHeatInkJ - deltaHeatEvap) / (liquidMass * liquidHeatCapacity), 63, 999); // 63 K is nitrogen freezing point

			if (liquidMass > evapMass) liquidMass -= evapMass;
			else // it is completely evaporated 
			{
				liquidMass = 0;
				liquidTemperature = getAmbientTemperature();
			}
		}
		else
		{
			// liquidTemperature = gameManager.ambientTemperature;
			isLiquidEmpty = true;
		}


		// gas calcs
		// not temperature dependend for simplicity

		if (!isGSE)
		{
			evapVolume = evapMass / gasDensity;

			ventVolume = 0;
			float ventingCoefficientPressureDependend = valveVentOpening * ventingCoefficient / FMath::Clamp(FMath::Abs(pressureG), 0.5f, nominalPressureG) * numberOfTanks;
			if (isOpenValveVent) ventVolume = -(pressureG * ventingCoefficientPressureDependend * deltaTime);

			liquidVolumeDiff = gasVolume - (capacity - liquidVolume);  // use gasVolume from previous iteration
			gasVolume = capacity - liquidVolume;
			if (gasVolume < 0.01f) gasVolume = 0.01f; // just in case make sure there is always a little volume to calculate pressure

			deltaVolume = evapVolume + ventVolume + liquidVolumeDiff + gasVolumeExternalDelta;
			
			deltaVolumeList[volumeListIterator] = deltaVolume / deltaValuesSmoothingFactor;
			if (volumeListIterator < 6) volumeListIterator++;
			else volumeListIterator = 0;
			deltaVolume = 0;
			for (float value : deltaVolumeList)
			{
				deltaVolume += value;
			}
			gasVolumeExternalDelta = 0;

			gasPressure = gasPressure * FMath::Pow((gasVolume + deltaVolume) / gasVolume, adiabaticIndex);
			if (gasPressure < 0) gasPressure = 0;
			pressureG = gasPressure - ambientPressure;
		}
	}
	else
	{ // when exploded, reduce mass and valveVent to zero
		if (liquidMass > 0) liquidMass -= FMath::Max(capacity * liquidDensity, 1000000) * deltaTime;
		else liquidMass = 0;
		valveVentNewSetpoint = 0;
	}
}

/// <summary>
/// adds liquid mass and calculates new average temperature
/// </summary>
void UCryoTank::addLiquidMass(float mass_in_kg, float temperature)
{
	if (!isExploded)
	{
		if (mass_in_kg > 0)
		{
			liquidTemperature = (liquidTemperature * liquidMass + temperature * mass_in_kg) / (liquidMass + mass_in_kg);
			liquidMass += mass_in_kg;
		}
	}

}

/// <summary>
/// Removes liquid mass, if supplied value is positive.
/// Returns true, if mass has been removed, and false, if is zero or exploded.
/// </summary>
bool UCryoTank::removeLiquidMass(float mass_in_kg)
{
	if (!isExploded && mass_in_kg > 0)
	{
		liquidMass -= mass_in_kg;
		if (liquidMass < 0) { liquidMass = 0; return false; }
		else return true;
	}
	else return false;
}

/// <summary>
/// All deltas in m³ are summed up and added to the gas volume in the slow update cycle.
/// </summary>
void UCryoTank::addGasVolumeExternalDelta(float value)
{
	if (!isExploded) gasVolumeExternalDelta += value;
}

/// <summary>
/// All heat amounts in kJ are summed up and added to the liquid volume in the slow update cycle.
/// </summary>
void UCryoTank::addHeat(float value)
{
	if (!isExploded) heatExternalDelta += value;
}

/// <summary>
/// Sets temperature to 90 K if compound is methane, else 77 K -> only use for rocket!
/// </summary>
void UCryoTank::setPropellantDirectly(float value_kg)
{
	if (!isExploded)
	{
		if (value_kg > 0)
		{
			if (compound == "methane")
			{
				liquidTemperature = 90;
			}
			else liquidTemperature = 77; // oxygen

			liquidDensity = molarMass / 1000.0f / (gasConstant * criticalTemperature / criticalPressure * FMath::Pow((0.29056f - 0.08775f * acentricFactor), (1 + FMath::Pow(1 - liquidTemperature / criticalTemperature, 2 / 7.0f))));

			liquidVolume = FMath::Clamp(value_kg / liquidDensity, 0, capacity * 0.97f); // 3% buffer for gas

			liquidMass = liquidVolume * liquidDensity;

			gasVolume = capacity - liquidVolume;

			pressureG = 0.99f * nominalPressureG;

			updatePropellant(_DeltaTime);

			nullLists();
		}
		else
		{
			liquidVolume = liquidMass = 0;
			updatePropellant(_DeltaTime);

		}
	}
}

void UCryoTank::nullLists()
{
	for (i = 0; i < deltaValuesSmoothingFactor; i++)
	{
		deltaHeatList[i] = 0;
		deltaVolumeList[i] = 0;
	}
}

/// <summary>
/// Sets mass with given temperature 
/// </summary>
void UCryoTank::setMassDirectly(float value_in_t, float temperature)
{
	if (!isExploded)
	{
		liquidTemperature = temperature;

		if (value_in_t > 0)
		{
			if (liquidTemperature < criticalTemperature)
			{
				liquidDensity = molarMass / 1000.0f / (gasConstant * criticalTemperature / criticalPressure * FMath::Pow((0.29056f - 0.08775f * acentricFactor), (1 + FMath::Pow(1 - liquidTemperature / criticalTemperature, 2 / 7.0f))));
				// after Yamada_Gunn from https://chemicals.readthedocs.io/chemicals.volume.html#pure-low-pressure-liquid-correlations
			}
			else
			{
				liquidDensity = 1000;
			}

			liquidVolume = FMath::Clamp(value_in_t * 1000 / liquidDensity, 0, capacity * 0.97f); // 1% buffer for gas

			liquidMass = liquidVolume * liquidDensity;
		}

		updatePropellant(_DeltaTime);

		nullLists();
	}
}

/// <summary>
/// fill up tank from update loop with boiling point temperatures
/// </summary>
void UCryoTank::fillUp()
{
	if (!isFullWithLiquid && !isExploded)
	{
		float tankerModifier = 1.05f; // small adjustment for compound
		float temperature = 92; // oxygen
		if (compound == "nitrogen")
		{
			temperature = 78;
		}
		else if (compound == "methane")
		{
			temperature = 112;
			tankerModifier = 0.95f;
		}

		addLiquidMass(tankerMass * tankerModifier * tankersPerHour / 3600 * _DeltaTime, temperature);
	}
}

/// <summary>
/// needs ventEffect and ventAudio to be set up
/// </summary>
void UCryoTank::updateVentValveEffects()
{

	pressureScaler = valveVentOpening * pressureG / nominalPressureG * pressureScalerMultiplier;
	if (pressureScaler > 0.0005f && !isExploded)
	{
		ventEffect->Activate();
		
		if(Rocket) ventEffect->SetVariableFloat(FName("DensityAir"), Rocket->DensityAir);
		else ventEffect->SetVariableFloat(FName("DensityAir"), 1);

		volumeScaler = -ventVolume / numberOfTanks * steamVolumeFactor; // ventVolume is total for all tanks

		float speedScaler = 1;
		if (isSubcooler) speedScaler = FMath::Clamp(FMath::Pow(pressureG / nominalPressureG, steamSpeedAdjustment), speedLowerLimit, 33.0f);
		else if (Rocket) speedScaler = FMath::Clamp(FMath::Pow(pressureScaler/3.0f, steamSpeedAdjustment), 0.01f, 33.0f);
		ventEffect->SetVariableFloat(FName("SpeedScale"), speedScaler);


		// reduce lifetime with decreasing air density and increasing velocity
		if (isSubcooler) ventEffect->SetVariableFloat(FName("LifetimeScale"), FMath::Clamp((1 - speedScaler) * steamLifetimeAdjustment, steamLifetimeAdjustment, 22.0f));
		else if (Rocket)
		{
			float lifetimeScaler = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 1.0f), FVector2D(0.33f, 1.0f), (1.0f - (FMath::Min(Rocket->VelocityMag, 100.0f) / 100.0f)) * Rocket->DensityAir);
			ventEffect->SetVariableFloat(FName("LifetimeScale"), lifetimeScaler);
		}

		
		if (Rocket)  ventEffect->SetVariableVec3(FName("Force"), Rocket->EffectDragForce);

		if (isSubcooler) ventEffect->SetVariableFloat(FName("SizeScale"), FMath::Clamp(FMath::Pow(pressureScaler * volumeScaler, steamSizeAdjustment), sizeLowerLimit, 5.0f));
		else if (Rocket) ventEffect->SetVariableFloat(FName("SizeScale"), pressureScaler * steamSizeAdjustment);

		if (isSubcooler) ventEffect->SetVariableFloat(FName("SpawnRateScale"), FMath::Clamp(FMath::Pow(speedScaler, steamEmissionAdjustment), emissionLowerLimit, 22.0f));
		else if (Rocket) ventEffect->SetVariableFloat(FName("SpawnRateScale"), steamEmissionAdjustment);


		ventAudioVolume = ventAudioVolume - 0.1f * (ventAudioVolume - pressureG/nominalPressureG);
		if (isSubcooler) ventAudio->SetVolumeMultiplier(ventAudioVolume * ventAudioVolumeMultiplier);
		else ventAudio->SetVolumeMultiplier(ventAudioVolume * 1.2f * FMath::Clamp(Rocket->DensityAir, 0.1f, 1.0f) * ventAudioVolumeMultiplier);
		ventAudio->SetPaused(false);
	}
	else
	{
		ventEffect->SetVariableFloat(FName("SpawnRateScale"), 0);
		ventAudio->SetPaused(true);
	}
}

/// <summary>
/// explosion forces and effects are determined by tank contents
/// </summary>
void UCryoTank::explode()
{
	if (!isExploded)
	{
		isExploded = true;
		ExplodeFromCPP();
	}
}

float UCryoTank::getInletHeightWorld()
{
	return altitude - inletOffset;
}