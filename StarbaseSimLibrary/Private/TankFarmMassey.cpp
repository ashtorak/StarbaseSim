
#include "TankFarmMassey.h"
#include "Math/UnrealMathUtility.h"
#include <Net/UnrealNetwork.h>
#include "StarbaseSimCommon.h"
#include "Components/AudioComponent.h"


// Sets default values
ATankFarmMassey::ATankFarmMassey()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//  needed for replication
	bReplicates = true;
}

//  needed for replication
void ATankFarmMassey::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATankFarmMassey, VolumetricFlowRate);
	DOREPLIFETIME(ATankFarmMassey, totalFlowrate);
	DOREPLIFETIME(ATankFarmMassey, isShipConnectedViaQD);
}

// Called when the game starts or when spawned
void ATankFarmMassey::BeginPlay()
{
	Super::BeginPlay();
	flowAudio->SetPaused(true);
}

// Called every frame
void ATankFarmMassey::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATankFarmMassey::AsyncPhysicsTickActor(float DeltaTime, float SimTime)
{
	Super::AsyncPhysicsTickActor(DeltaTime, SimTime);
	// only run farm on server, valve setpoints and tank properties are replicated
	if (GetNetMode() == ENetMode::NM_Standalone || GetNetMode() == ENetMode::NM_ListenServer)
	{
		totalFlowrate = 0;
		if (isShipConnectedViaQD && OxidizerTank && FuelTank)
		{
			if (ValveFill->isOpen)
			{
				if (ValveShipOxidizer->isOpen)
				{
					Flowrate = ValveFill->opening * ValveShipOxidizer->opening * VolumetricFlowRate;
					totalFlowrate += Flowrate;
					if (OxidizerTank->isFullWithLiquid || OxidizerTank->isExploded) ValveShipOxidizer->setOpening(0.0f);
					else OxidizerTank->addLiquidMass(Flowrate * OxidizerTank->liquidDensity * DeltaTime, OxidizerTemperature);
				}
				if (ValveShipFuel->isOpen)
				{
					Flowrate = ValveFill->opening * ValveShipFuel->opening * VolumetricFlowRate;
					totalFlowrate += Flowrate;
					if (FuelTank->isFullWithLiquid || FuelTank->isExploded) ValveShipFuel->setOpening(0.0f);
					else FuelTank->addLiquidMass(Flowrate * FuelTank->liquidDensity * DeltaTime, FuelTemperature);
				}
			}
			else if (ValveDrain->isOpen)
			{
				if (ValveShipOxidizer->isOpen)
				{
					Flowrate = ValveDrain->opening * ValveShipOxidizer->opening * VolumetricFlowRate;
					totalFlowrate += Flowrate;
					if (OxidizerTank->isLiquidEmpty || OxidizerTank->isExploded) ValveShipOxidizer->setOpening(0.0f);
					else OxidizerTank->removeLiquidMass(Flowrate * OxidizerTank->liquidDensity * DeltaTime);

				}
				if (ValveShipFuel->isOpen)
				{
					Flowrate = ValveDrain->opening * ValveShipFuel->opening * VolumetricFlowRate;
					totalFlowrate += Flowrate;
					if (FuelTank->isLiquidEmpty || FuelTank->isExploded) ValveShipFuel->setOpening(0.0f);
					else FuelTank->removeLiquidMass(Flowrate * FuelTank->liquidDensity * DeltaTime);

				}
			}
		}
		else  // hard close valves if not connected or no tanks
		{
			ValveShipOxidizer->setOpening(0.0f);
			ValveShipFuel->setOpening(0.0f);
		}

	}

	// totalFlowRate is replicated, so we have sound on client also
		if (totalFlowrate > 0.0f)
		{
			flowAudioVolume = FMath::Clamp(totalFlowrate / maxFlowrateForSound, 0.05f, 0.3f);
			flowAudio->SetVolumeMultiplier(flowAudioVolume);
			flowAudio->SetPitchMultiplier(1 + (totalFlowrate / maxFlowrateForSound - 0.5f) / 50.0f);
			flowAudio->SetPaused(false);
		}
		else flowAudio->SetPaused(true);
}

void ATankFarmMassey::operateValve(FString valve)
{
	if (valve == "ShipFuel")
	{
		ValveShipFuel->openClose();
	}
	else if (valve == "ShipOxidizer")
	{
		ValveShipOxidizer->openClose();
	}
	else if (valve == "Fill")
	{
		ValveFill->openClose();
		ValveDrain->setOpening(0.0f);
	}
	else if (valve == "Drain")
	{
		ValveDrain->openClose();
		ValveFill->setOpening(0.0f);
	}
	
}