
#include "Tankfarm.h"
#include "Math/UnrealMathUtility.h"
#include <Net/UnrealNetwork.h>
#include "StarbaseSimCommon.h"
#include "Components/AudioComponent.h"

// Sets default values
ATankfarm::ATankfarm()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//  needed for replication
	bReplicates = true;
}

//  needed for replication
void ATankfarm::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATankfarm, totalFlowrate);
	DOREPLIFETIME(ATankfarm, isShipConnectedViaQD);
	DOREPLIFETIME(ATankfarm, isBoosterConnectedViaQD);
	DOREPLIFETIME(ATankfarm, subcoolerOxygenFillAutomaticControl);
	DOREPLIFETIME(ATankfarm, subcoolerMethaneFillAutomaticControl);
}

// Called when the game starts or when spawned
void ATankfarm::BeginPlay()
{
	Super::BeginPlay();

	if (flowAudioOxiPump)
	{
		flowAudioOxiPump->SetPaused(true);
		flowAudioFuelPump->SetPaused(true);
		flowAudioOxiSubcoolerN2->SetPaused(true);
		flowAudioFuelSubcoolerN2->SetPaused(true);
		flowAudioOxiValveBooster->SetPaused(true);
		flowAudioFuelValveBooster->SetPaused(true);
		flowAudioOxiValveShip->SetPaused(true);
		flowAudioFuelValveShip->SetPaused(true);
		flowAudioQDOxiValveBooster->SetPaused(true);
		flowAudioQDFuelValveBooster->SetPaused(true);
		flowAudioQDOxiValveShip->SetPaused(true);
		flowAudioQDFuelValveShip->SetPaused(true);
	}
}

// Called every frame
void ATankfarm::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TickAudio();

}

void ATankfarm::AsyncPhysicsTickActor(float DeltaTime, float SimTime)
{
	Super::AsyncPhysicsTickActor(DeltaTime, SimTime);
	// only run farm on server, valve setpoints and tank properties are replicated
	if (GetNetMode() == ENetMode::NM_Standalone || GetNetMode() == ENetMode::NM_ListenServer)
	{
		_DeltaTime = DeltaTime;

		updateSubCoolers();

		if (!isBoosterConnectedViaQD)
		{
			ValveBoosterOxidizer->setOpening(0);
			ValveBoosterFuel->setOpening(0);
		}
		if (!isShipConnectedViaQD)
		{
			ValveShipOxidizer->setOpening(0);
			ValveShipFuel->setOpening(0);
		}

		if (!FuelTank->isExploded)
		{
			if (!SubcoolerFuel->isExploded)
			{
				if (BoosterFuelTank && ValveBoosterFuel->isOpen && methanePump->isOn)
				{
					if ((BoosterFuelTank->isLiquidEmpty && methanePump->flowrate_in_m3_per_second < 0) || (BoosterFuelTank->isFullWithLiquid && methanePump->flowrate_in_m3_per_second > 0))
					{
						ValveBoosterFuel->setOpening(0);
					}
					boosterFlowrateMethane = calculateFlowrate(FuelTank, BoosterFuelTank, ValveBoosterFuel->opening, methanePump, SubcoolerFuel, false);
				}
			}
			else
			{
				methanePump->flowrate_in_m3_per_second = boosterFlowrateMethane = 0;
			}

			if (ShipFuelTank && ValveShipFuel->isOpen && methanePumpShip->isOn)
			{
				if ((ShipFuelTank->isLiquidEmpty && methanePumpShip->flowrate_in_m3_per_second < 0) || (ShipFuelTank->isFullWithLiquid && methanePumpShip->flowrate_in_m3_per_second > 0))
				{
					ValveShipFuel->setOpening(0);
				}
				shipFlowrateMethane = calculateFlowrate(FuelTank, ShipFuelTank, ValveShipFuel->opening, methanePumpShip, SubcoolerFuelShip, false);
			}
			else
			{
				methanePumpShip->flowrate_in_m3_per_second = shipFlowrateMethane = 0;
			}
		}
		else
		{
			methanePump->flowrate_in_m3_per_second = boosterFlowrateMethane = 0;
			methanePumpShip->flowrate_in_m3_per_second = shipFlowrateMethane = 0;
		}


		if (!OxidizerTank->isExploded)
		{
			if (!SubcoolerOxidizer->isExploded)
			{
				if (BoosterOxiTank && ValveBoosterOxidizer->isOpen && oxygenPump->isOn)
				{
					if ((BoosterOxiTank->isLiquidEmpty && oxygenPump->flowrate_in_m3_per_second < 0) || (BoosterOxiTank->isFullWithLiquid && oxygenPump->flowrate_in_m3_per_second > 0))
					{
						ValveBoosterOxidizer->setOpening(0);
					}
					boosterFlowrateOxygen = calculateFlowrate(OxidizerTank, BoosterOxiTank, ValveBoosterOxidizer->opening, oxygenPump, SubcoolerOxidizer, false);
				}
			}
			else
			{
				oxygenPump->flowrate_in_m3_per_second = boosterFlowrateMethane = 0;
			}

			if (ShipOxiTank && ValveShipOxidizer->isOpen && oxygenPumpShip->isOn)
			{
				if ((ShipOxiTank->isLiquidEmpty && oxygenPumpShip->flowrate_in_m3_per_second < 0) || (ShipOxiTank->isFullWithLiquid && oxygenPumpShip->flowrate_in_m3_per_second > 0))
				{
					ValveShipOxidizer->setOpening(0);
				}
				shipFlowrateMethane = calculateFlowrate(OxidizerTank, ShipOxiTank, ValveShipOxidizer->opening, oxygenPumpShip, SubcoolerOxidizerShip, false);
			}
			else
			{
				oxygenPumpShip->flowrate_in_m3_per_second = shipFlowrateMethane = 0;
			}
		}
		else
		{
			oxygenPump->flowrate_in_m3_per_second = boosterFlowrateMethane = 0;
			oxygenPumpShip->flowrate_in_m3_per_second = shipFlowrateMethane = 0;
		}
	}
}

/// <summary>
/// Returns flowrate in m³/s.
/// If pumpower>0 flow is from source to destination, else reversed.
/// Returns 0, if tanks are exploded.
/// If pump is null, flow is just calculated from tanks pressure differential (hydrostatic and gas).
/// Set subCooler to null, to not use it.
/// Pump has to be on to have flow through it, even without power.
/// Lots of simplification here again... :)
/// </summary>
float ATankfarm::calculateFlowrate(TObjectPtr<class UCryoTank> source, TObjectPtr<class UCryoTank> destination, float valveOpening, TObjectPtr<class UCryoPump> pump, TObjectPtr<class UCryoTank> subCooler, bool useMethaneHeatExchanger)
{
	float flowrate = 0;
	float flowrateAbs = 0;
	float deltaMass = 0;

	if (!(source->isExploded && destination->isExploded && subCooler->isExploded) && valveOpening > 0)
	{
		if (pump) pumpPressure = pump->pressure;
		else pumpPressure = 0;

		// first calculate possible flowrate from pressure difference
		if (!pump || pump->isOn) // pump has to be on to have flow through it, even without power
		{
			pressureHead = pumpPressure + source->pressureG + source->pressureHydroStaticG -
				(destination->pressureG + destination->pressureHydroStaticG + 
					(destination->getInletHeightWorld() - source->getInletHeightWorld()) * source->liquidDensity * 9.81f / 100000.0f);

			if ((pressureHead > 0 && !source->isLiquidEmpty) || (pressureHead < 0 && !destination->isLiquidEmpty))
			{
				if (subCooler) mechanicalResistance = 0.01f / FMath::Pow(destination->inletDiameter * valveOpening - subCooler->internalIceThickness, 5) / destination->numberOfTanks / subCooler->numberOfTanks;
				else mechanicalResistance = 0.01f / FMath::Pow(destination->inletDiameter * valveOpening, 5) / destination->numberOfTanks;
				// this is tuned so that it gives reasonably large flows at the used diameters while not needing insane pump pressure

				flowrate = pressureHead / mechanicalResistance;
				flowrateAbs = FMath::Abs(flowrate);
				deltaMass = flowrateAbs * source->liquidDensity * _DeltaTime;
			}
			else flowrate = deltaMass = 0;
		}

		// calculate ice effect which can only happen for methane
		if (source->compound == "methane" && subCooler)
		{
			float icingDirection = 0;
			if (flowrate != 0)
			{
				icingDirection = (90 - subCooler->liquidTemperature) * subCooler->efficiency
					+ (90 - source->liquidTemperature) * FMath::Clamp(subCoolerIcingMethaneFactor * (1 - subCooler->efficiency) * flowrateAbs, 0, 1);
			}
			else if (subCooler->liquidTemperature > 90) icingDirection = (90 - subCooler->liquidTemperature) * 0.1f;

			// change ice thickness depending on how much we are above or below methane freezing point and on how thick it is already
			subCooler->internalIceThickness += icingSpeed * icingDirection * (1 - subCooler->internalIceThickness / destination->inletDiameter);
			subCooler->internalIceThickness = FMath::Clamp(subCooler->internalIceThickness, 0, 0.33f * destination->inletDiameter);

			subCooler->efficiencyIceReduction = 1 - subCooler->internalIceThickness / destination->inletDiameter;
		}

		// then set mass and heat values accordingly
		if (flowrate > 0 && !source->isLiquidEmpty) // flow from source to destination
		{
			source->removeLiquidMass(deltaMass);

			if (subCooler)
			{
				// simplified reduction of cooling effect due to liquid level by mixing source and subcooler temperature

				subCooler->efficiency = subCooler->efficiencyIceReduction * FMath::Clamp(FMath::GetRangePct(0.0f, subCoolerNominalLiquidFraction - subCoolerNominalLiquidFractionHysteresis, subCooler->liquidVolumeFraction), 0, 0.95f);
				// reduce subcooler efficiency with increasing flowrate
				subCooler->efficiency = subCooler->efficiency * (1 - FMath::Clamp(subCoolerEfficiencyFlowrateFactor * flowrateAbs / subCooler->numberOfTanks, 0.0f, 0.3f));

				subCooler->outTemp = subCooler->liquidTemperature * subCooler->efficiency + source->liquidTemperature * (1 - subCooler->efficiency);
				if (source->compound == "methane" && subCooler->outTemp < 90) subCooler->outTemp = 90; // this should only happen for very short time when subcooler is very cool before runnign methane through it, then ice builds up

				destination->addLiquidMass(deltaMass, subCooler->outTemp);

				subCooler->addHeat(deltaMass * source->liquidHeatCapacity * (source->liquidTemperature - subCooler->liquidTemperature) * subCooler->efficiency);
			}
			else
			{
				if (useMethaneHeatExchanger) // only true for nitrogen going into methane subcooler
				{
					destination->addLiquidMass(deltaMass, 90);
				}
				else destination->addLiquidMass(deltaMass, source->liquidTemperature);
			}

		}
		else if (flowrate < 0 && !destination->isLiquidEmpty) // flow from destination to source
		{
			destination->removeLiquidMass(deltaMass);

			if (subCooler)
			{
				// simplified reduction of cooling effect due to liquid level by mixing source and subcooler temperature
				subCooler->efficiency = subCooler->efficiencyIceReduction * FMath::Clamp(FMath::GetRangePct(0.0f, subCoolerNominalLiquidFraction - subCoolerNominalLiquidFractionHysteresis, subCooler->liquidVolumeFraction), 0, 0.95f);
				// reduce subcooler efficiency with increasing flowrate
				subCooler->efficiency = subCooler->efficiency * (1 - FMath::Clamp(subCoolerEfficiencyFlowrateFactor * flowrateAbs / subCooler->numberOfTanks, 0, 0.3f));

				subCooler->outTemp = subCooler->liquidTemperature * subCooler->efficiency + destination->liquidTemperature * (1 - subCooler->efficiency);

				source->addLiquidMass(deltaMass, subCooler->outTemp);
				subCooler->addHeat(deltaMass * destination->liquidHeatCapacity * (destination->liquidTemperature - subCooler->liquidTemperature) * subCooler->efficiency);
				subCooler->outTemp = destination->liquidTemperature; // for display, in the UI it is shown at the inlet in this case
			}
			else source->addLiquidMass(deltaMass, destination->liquidTemperature);
		}
		else flowrate = 0;
	}


	if (pump) // set pump flow and power (only for info)
	{
		pump->flowrate_in_m3_per_second = flowrate;
	}

	return flowrate;
}

void ATankfarm::updateSubCoolers()
{
	if (SubcoolerOxidizer->isExploded)
	{
		ValveBoosterOxidizer->setOpening(0);
		ValveFillSubcoolerOxidizer->setOpening(0);
		subcoolerOxygenFlowrate = 0;
	}
	else
	{
		if (subcoolerOxygenFillAutomaticControl)
		{
			if (SubcoolerOxidizer->liquidVolumeFraction < (subCoolerNominalLiquidFraction - subCoolerNominalLiquidFractionHysteresis) && !ValveFillSubcoolerOxidizer->isOpen) ValveFillSubcoolerOxidizer->setOpening(1);
			else if (SubcoolerOxidizer->liquidVolumeFraction > (subCoolerNominalLiquidFraction + subCoolerNominalLiquidFractionHysteresis)) ValveFillSubcoolerOxidizer->setOpening(0);
		}

		if (ValveFillSubcoolerOxidizer->isOpen)
		{
			subcoolerOxygenFlowrate = calculateFlowrate(NitrogenTank, SubcoolerOxidizer, ValveFillSubcoolerOxidizer->opening, nullptr, nullptr, false);
		}
	}

	if (SubcoolerOxidizerShip->isExploded)
	{
		ValveShipOxidizer->setOpening(0);
		ValveFillSubcoolerOxidizerShip->setOpening(0);
		subcoolerOxygenFlowrateShip = 0;
	}
	else
	{
		if (subcoolerOxygenFillAutomaticControlShip)
		{
			if (SubcoolerOxidizerShip->liquidVolumeFraction < (subCoolerNominalLiquidFraction - subCoolerNominalLiquidFractionHysteresis) && !ValveFillSubcoolerOxidizerShip->isOpen) ValveFillSubcoolerOxidizerShip->setOpening(1);
			else if (SubcoolerOxidizerShip->liquidVolumeFraction > (subCoolerNominalLiquidFraction + subCoolerNominalLiquidFractionHysteresis)) ValveFillSubcoolerOxidizerShip->setOpening(0);
		}

		if (ValveFillSubcoolerOxidizerShip->isOpen)
		{
			subcoolerOxygenFlowrateShip = calculateFlowrate(NitrogenTank, SubcoolerOxidizerShip, ValveFillSubcoolerOxidizerShip->opening, nullptr, nullptr, false);
		}
	}

	if (SubcoolerFuel->isExploded)
	{
		ValveBoosterFuel->setOpening(0);
		ValveFillSubcoolerFuel->setOpening(0);
		subcoolerMethaneFlowrate = 0;
	}
	else
	{
		if (subcoolerMethaneFillAutomaticControl)
		{
			if (SubcoolerFuel->liquidVolumeFraction < (subCoolerNominalLiquidFraction - subCoolerNominalLiquidFractionHysteresis) && !ValveFillSubcoolerFuel->isOpen) ValveFillSubcoolerFuel->setOpening(1);
			else if (SubcoolerFuel->liquidVolumeFraction > (subCoolerNominalLiquidFraction + subCoolerNominalLiquidFractionHysteresis)) ValveFillSubcoolerFuel->setOpening(0);
		}

		if (ValveFillSubcoolerFuel->isOpen)
		{
			subcoolerMethaneFlowrate = calculateFlowrate(NitrogenTank, SubcoolerFuel, ValveFillSubcoolerFuel->opening, nullptr, nullptr, true);
		}
	}

	if (SubcoolerFuelShip->isExploded)
	{
		ValveShipFuel->setOpening(0);
		ValveFillSubcoolerFuelShip->setOpening(0);
		subcoolerMethaneFlowrateShip = 0;
	}
	else
	{
		if (subcoolerMethaneFillAutomaticControlShip)
		{
			if (SubcoolerFuelShip->liquidVolumeFraction < (subCoolerNominalLiquidFraction - subCoolerNominalLiquidFractionHysteresis) && !ValveFillSubcoolerFuelShip->isOpen) ValveFillSubcoolerFuelShip->setOpening(1);
			else if (SubcoolerFuelShip->liquidVolumeFraction > (subCoolerNominalLiquidFraction + subCoolerNominalLiquidFractionHysteresis)) ValveFillSubcoolerFuelShip->setOpening(0);
		}

		if (ValveFillSubcoolerFuelShip->isOpen)
		{
			subcoolerMethaneFlowrateShip = calculateFlowrate(NitrogenTank, SubcoolerFuelShip, ValveFillSubcoolerFuelShip->opening, nullptr, nullptr, true);
		}
	}


	// add some ambient heat to subCooler outlet temperature which is somewhere on the pipe, so that temperature slowly increases when flow is zero
	SubcoolerOxidizer->outTemp += (getAmbientTemperature() - SubcoolerOxidizer->outTemp) * 0.000001f;
	SubcoolerFuel->outTemp += (getAmbientTemperature() - SubcoolerFuel->outTemp) * 0.000001f;
	SubcoolerOxidizerShip->outTemp += (getAmbientTemperature() - SubcoolerOxidizerShip->outTemp) * 0.000001f;
	SubcoolerFuelShip->outTemp += (getAmbientTemperature() - SubcoolerFuelShip->outTemp) * 0.000001f;
}

void ATankfarm::operateValve(FString valve) const
{
	if (valve == "ShipFuel")
	{
		ValveShipFuel->openClose();
	}
	else if (valve == "ShipOxidizer")
	{
		ValveShipOxidizer->openClose();
	}
	else if (valve == "BoosterFuel")
	{
		ValveBoosterFuel->openClose();
	}
	else if (valve == "BoosterOxidizer")
	{
		ValveBoosterOxidizer->openClose();
	}
	else if (valve == "SubcoolerFillOxidizer")
	{
		ValveFillSubcoolerOxidizer->openClose();
	}
	else if (valve == "SubcoolerFillFuel")
	{
		ValveFillSubcoolerFuel->openClose();
	}
	else if (valve == "SubcoolerVentFuel")
	{
		SubcoolerFuel->valveVentNewSetpoint = SubcoolerFuel->isOpenValveVent ? 0.0f : 1.0f;
	}
	else if (valve == "SubcoolerFillOxidizerShip")
	{
		ValveFillSubcoolerOxidizerShip->openClose();
	}
	else if (valve == "SubcoolerFillFuelShip")
	{
		ValveFillSubcoolerFuelShip->openClose();
	}
	else if (valve == "SubcoolerVentFuelShip")
	{
		SubcoolerFuelShip->valveVentNewSetpoint = SubcoolerFuelShip->isOpenValveVent ? 0.0f : 1.0f;
	}
}

// interlock if only ship or booster shall be connectable
void ATankfarm::checkValveState()
{
	if (ValveShipFuel->isFullyOpen && ValveBoosterFuel->isOpen) ValveShipFuel->setOpening(0);
	if (ValveBoosterFuel->isFullyOpen && ValveShipFuel->isOpen) ValveBoosterFuel->setOpening(0);
	if (ValveShipOxidizer->isFullyOpen && ValveBoosterOxidizer->isOpen) ValveShipOxidizer->setOpening(0);
	if (ValveBoosterOxidizer->isFullyOpen && ValveShipOxidizer->isOpen) ValveBoosterOxidizer->setOpening(0);
}

void ATankfarm::TickAudio()
{
	float oxi = FMath::Abs(oxygenPump->flowrate_in_m3_per_second + oxygenPumpShip->flowrate_in_m3_per_second);
	float fuel = FMath::Abs(methanePump->flowrate_in_m3_per_second + methanePumpShip->flowrate_in_m3_per_second);
	setSound(true, flowAudioOxiPump, oxi);
	setSound(true, flowAudioFuelPump, fuel);
	setSound(true, flowAudioOxiSubcoolerN2, FMath::Abs(subcoolerOxygenFlowrate + subcoolerOxygenFlowrateShip));
	setSound(true, flowAudioFuelSubcoolerN2, FMath::Abs(subcoolerMethaneFlowrate + subcoolerMethaneFlowrateShip));
	setSound(true, flowAudioOxiValveBooster, oxi);
	setSound(true, flowAudioFuelValveBooster, fuel);
	setSound(true, flowAudioOxiValveShip, oxi);
	setSound(true, flowAudioFuelValveShip, fuel);
	setSound(true, flowAudioQDOxiValveBooster, oxi);
	setSound(true, flowAudioQDFuelValveBooster, fuel);
	setSound(true, flowAudioQDOxiValveShip, oxi);
	setSound(true, flowAudioQDFuelValveShip, fuel);
}

void ATankfarm::setSound(bool isValveOpen, TObjectPtr<class UAudioComponent> flowAudio, float flowrateAbsolute)
{
	if (isValveOpen && flowrateAbsolute > 0)
	{
		flowAudioVolume = FMath::Clamp(flowrateAbsolute / maxFlowrateForSound, 0.05f, 0.3f);
		flowAudio->SetVolumeMultiplier(flowAudioVolume);
		flowAudio->SetPitchMultiplier(1 + (flowrateAbsolute / maxFlowrateForSound - 0.5f) / 50.0f);
		flowAudio->SetPaused(false);
	}
	else flowAudio->SetPaused(true);
}