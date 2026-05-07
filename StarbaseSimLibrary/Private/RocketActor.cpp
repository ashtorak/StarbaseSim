// see readme.txt


#include "RocketActor.h"
#include "Math/UnrealMathUtility.h"
#include "StarbaseSimCommon.h"
#include <Net/UnrealNetwork.h>
#include "NiagaraComponent.h"
#include "Components/AudioComponent.h"
#include "RocketControllerComponent.h"
#include "AeroDynamicsComponent.h"
#include "AsyncTickFunctions.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
ARocketActor::ARocketActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	bReplicates = true;

}



void ARocketActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARocketActor, isOpenValveRaptorChill);
	DOREPLIFETIME(ARocketActor, turbopumpTemperature);
	DOREPLIFETIME(ARocketActor, bDumpOxidizer);
	DOREPLIFETIME(ARocketActor, bDumpFuel);
	DOREPLIFETIME(ARocketActor, DumpSpeed);
	DOREPLIFETIME(ARocketActor, MainMeshTransform);
	DOREPLIFETIME(ARocketActor, Velocity);

} 

// Called when the game starts or when spawned
void ARocketActor::BeginPlay()
{
	Super::BeginPlay();
	
	TArray<URocketControllerComponent*> comps;
	GetComponents<URocketControllerComponent>(comps);
	if (comps.Num() > 0)
	{
		RocketControllerComp = comps[0];
	}

	TArray<UAeroDynamicsComponent*> aerocomps;
	GetComponents<UAeroDynamicsComponent>(aerocomps);
	if (comps.Num() > 0)
	{
		AeroDynamicsComponent = aerocomps[0];
	}

	turbopumpTemperature = getAmbientTemperature();
}

// Called every frame
void ARocketActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (RocketControllerComp)
	{
		if (!RocketControllerComp->isFullySwitchedOff && oxidizerTank && fuelTank)
		{
			if (!isExploded)
			{
				TankEffects();
				EngineChillEffects(DeltaTime);

				if (GetNetMode() == ENetMode::NM_Standalone || GetNetMode() == ENetMode::NM_ListenServer)
				{
					FTimerHandle ChillHandle;
					if (AreEnginesReleased)
					{
						if (turbopumpTemperature > (turbopumpTemperatureTargetNominal + turbopumpTemperatureTargetMargin) && (FMath::RandRange(1.0f, 100.0f) <= FMath::Pow(turbopumpTemperature / turbopumpTemperatureTargetNominal, 2.5f)))
						{
							GetWorldTimerManager().SetTimer(
								ChillHandle, this, &ARocketActor::explode, 2.0f, false);
						}
					}
					else GetWorldTimerManager().ClearTimer(ChillHandle);
				}
			}
		}
	}
}

void ARocketActor::AsyncPhysicsTickActor(float DeltaTime, float SimTime)
{
	Super::AsyncPhysicsTickActor(DeltaTime, SimTime);

	UpdatePhysicsState(DeltaTime);

	if (RocketControllerComp)
	{
		AreEnginesReleased = RocketControllerComp->AreEnginesReleased;

		if(!RocketControllerComp->isFullySwitchedOff && oxidizerTank && fuelTank)
		{
			if (!isExploded)
			{
				UpdateMass(); // can be run on clients as it uses tanks' replicated properties

				if (GetNetMode() == ENetMode::NM_Standalone || GetNetMode() == ENetMode::NM_ListenServer)
				{
					EngineChill(DeltaTime);
				}
			}
		}

	}
}

void ARocketActor::UpdatePhysicsState(float dT)
{
	if (MainMeshComponent)
	{
		MainMeshTransform = UAsyncTickFunctions::ATP_GetTransform(MainMeshComponent);
		//UE_LOG(LogTemp, Log, TEXT("RA: %f %f %f"), MainMeshTransform.GetLocation().X, MainMeshTransform.GetLocation().Y, MainMeshTransform.GetLocation().Z);

		// update physics state values
		CurrentPosition = MainMeshTransform.GetLocation() * 0.01f;
		Altitude = CurrentPosition.Z;
		DensityAir = 1.2 * FMath::Exp(-Altitude / 10000);

		Velocity = UAsyncTickFunctions::ATP_GetLinearVelocity(MainMeshComponent) * 0.01f;
		VelocityMag = Velocity.Length();

		Acceleration = UKismetMathLibrary::WeightedMovingAverage_FVector((Velocity - VelocityPrevious) / dT, Acceleration, AccelerationSmoothingFactor);
		BodyAcceleration = MainMeshTransform.InverseTransformVector(Acceleration - Gravity);
		BodyAccelerationMagnitude = BodyAcceleration.Length();

		VelocityPrevious = Velocity;

		// calculate a drag force for particle effects
		FVector normalizedVelocity = Velocity;
		normalizedVelocity.Normalize();
		EffectDragForce = Velocity * Velocity * -1.0f * normalizedVelocity * DensityAir * DensityAir;
	}
}

void ARocketActor::setPropellantDirectly(float value_in_t)
{
	fuelTank->setPropellantDirectly(value_in_t * 1000.0f * fuelRatio);
	oxidizerTank->setPropellantDirectly(value_in_t * 1000.0f * oxidizerRatio);

	if (value_in_t > 0)
	{
		isOpenValveRaptorChill = true;
		turbopumpTemperature = turbopumpTemperatureTarget;
	}
	else
	{
		isOpenValveRaptorChill = false;
	}
}

const float ARocketActor::oxidizerRatioMethaLOX = 0.7826f; // 3.6 Oxidizer : 1 CH4	
const float ARocketActor::fuelRatioMethaLOX = 1 - 0.7826f;

void ARocketActor::UpdateTanksAfterFiring(float TotalMassDelta)
{
	oxidizerTank->removeLiquidMass(TotalMassDelta * oxidizerRatio);
	oxidizerTank->addGasVolumeExternalDelta(TotalMassDelta * oxidizerRatio / oxidizerTank->liquidDensity); // simple autogeneous pressurization :)
	fuelTank->removeLiquidMass(TotalMassDelta * fuelRatio);
	fuelTank->addGasVolumeExternalDelta(TotalMassDelta * fuelRatio / fuelTank->liquidDensity); // simple autogeneous pressurization :)
	UpdateMass();
}

void ARocketActor::UpdateMass()
{
	totalLiquidMass = oxidizerTank->liquidMass + fuelTank->liquidMass;
	totalGasMass = oxidizerTank->gasMass + fuelTank->gasMass;
	totalPropMass = totalLiquidMass + totalGasMass;
}

void ARocketActor::EngineChill(float dT)
{
	if (oxidizerTank->liquidMass <= 0 && fuelTank->liquidMass <= 0)
	{
		isOpenValveRaptorChill = false;
	}


	if (isOpenValveRaptorChill)
	{
		if (oxidizerTank->liquidMass > 0)
		{
			
			oxidizerTank->removeLiquidMass(NumberOfEngines * dT);

			// temperatures in Kelvin!
			turbopumpTemperatureTarget = oxidizerTank->liquidTemperature + 5.0f; // pump will be a little bit warmer than LOX itself
			if (turbopumpTemperature > turbopumpTemperatureTargetNominal) turbopumpTemperature -= 30.0f * FMath::Clamp(1.0f - turbopumpTemperatureTarget / turbopumpTemperature, 0.0f, 1.0f) * dT;
		}
	}
	
	if (!isOpenValveRaptorChill || oxidizerTank->liquidMass <= 0) 
	{
		if (!AreEnginesReleased && turbopumpTemperature < getAmbientTemperature() - 0.3f)
			turbopumpTemperature += 10 * DensityAir * FMath::Clamp(FMath::Pow(turbopumpTemperatureTarget / turbopumpTemperature, 2), 0.02f, 1.0f) * dT;
		else if (!AreEnginesReleased && turbopumpTemperature > getAmbientTemperature() + 0.3f)
			turbopumpTemperature -= 10 * DensityAir * FMath::Clamp(FMath::Pow(turbopumpTemperatureTarget / turbopumpTemperature, 2), 0.02f, 1.0f) * dT;
		else if (AreEnginesReleased && turbopumpTemperature > turbopumpTemperatureTarget) turbopumpTemperature -= 30 * FMath::Clamp(1 - turbopumpTemperatureTarget / turbopumpTemperature, 0.1f, 1) * dT;
	}

	// dumping stuff

	if (bDumpOxidizer)
	{
		if (oxidizerTank->liquidMass > 0) oxidizerTank->removeLiquidMass(oxidizerTank->pressureG * DumpSpeed * oxidizerRatio * dT);
		else bDumpOxidizer = false;
	}
	if (bDumpFuel)
	{
		if (fuelTank->liquidMass > 0) fuelTank->removeLiquidMass(fuelTank->pressureG * DumpSpeed * fuelRatio * dT);
		else bDumpFuel = false;
	}

}

void ARocketActor::EngineChillEffects(float dT)
{
	if (ventEngineChillAudio && ventEngineChillEffect)
	{
		if (oxidizerTank->liquidMass > 0 && isOpenValveRaptorChill)
		{
			ventEngineChillEffect->Activate();

			ventEngineChillEffect->SetVariableFloat(FName("DensityAir"), DensityAir);

			float minScale = 0.6f;
			if (!AreEnginesReleased && RocketControllerComp)
			{
				scalingFactor = minScale + float(RocketControllerComp->NumberOfEnginesRequested) / float(NumberOfEngines) * (1 - minScale);
				
				//ventEngineChillEffect->SetVariableFloat(FName("LifetimeScale"), scalingFactor * FMath::Exp(-VelocityMag / steamRocketVelocityAdjustment));
				ventEngineChillEffect->SetVariableFloat(FName("SpawnRateScale"), 1.0f);
				ventEngineChillEffect->SetVariableFloat(FName("SizeScale"), scalingFactor * steamSizeAdjustment);
				ventEngineChillEffect->SetVariableFloat(FName("SpeedScale"), scalingFactor * steamSizeAdjustment);
			
				scalingFactor = 0.5f; // start value for when engines are not released anymore
			}
			else if(AreEnginesReleased)
			{
				if(scalingFactor > 0.01f) scalingFactor -= 0.3f * dT;
				else scalingFactor = 0.0f;

				ventEngineChillEffect->SetVariableFloat(FName("SpawnRateScale"), 1.0f);
				//ventEngineChillEffect->SetVariableFloat(FName("LifetimeScale"), scalingFactor * FMath::Exp(-VelocityMag / steamRocketVelocityAdjustment));
				ventEngineChillEffect->SetVariableFloat(FName("SizeScale"), scalingFactor * steamSizeAdjustment);
				ventEngineChillEffect->SetVariableFloat(FName("SpeedScale"), FMath::Clamp(1/scalingFactor, 1.0f, 5.0f) * steamSizeAdjustment);
			}
				
			// reduce lifetime with decreasing air density and increasing velocity
			//float lifetimeScaler = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 1.0f), FVector2D(0.33f, 1.0f), (1.0f - (FMath::Min(VelocityMag, 100.0f) / 100.0f)) * DensityAir);
			float lifetimeScaler = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 1.0f), FVector2D(0.1f, 1.0f), DensityAir * 30.0f / (FMath::Max(VelocityMag, 30.0f)));
			ventEngineChillEffect->SetVariableFloat(FName("LifetimeScale"), lifetimeScaler);

			ventEngineChillEffect->SetVariableVec3(FName("Force"), EffectDragForce);

			ventEngineChillAudioVolume = oxidizerTank->pressureG * 0.1f * FMath::Clamp(DensityAir, 0.1f, 1.0f);
			ventEngineChillAudio->SetVolumeMultiplier(ventEngineChillAudioVolume);
			ventEngineChillAudio->SetPaused(false);
		}
		else if (bDumpFuel || bDumpOxidizer)
		{
			ventEngineChillEffect->Activate();

			ventEngineChillEffect->SetVariableFloat(FName("DensityAir"), DensityAir);

			ventEngineChillEffect->SetVariableFloat(FName("SpawnRateScale"), 1.0f);
			ventEngineChillEffect->SetVariableFloat(FName("SizeScale"), steamSizeAdjustment);
			ventEngineChillEffect->SetVariableFloat(FName("SpeedScale"), steamSizeAdjustment);

			float lifetimeScaler = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 1.0f), FVector2D(0.1f, 1.0f), DensityAir * 30.0f / (FMath::Max(VelocityMag, 30.0f)));
			ventEngineChillEffect->SetVariableFloat(FName("LifetimeScale"), lifetimeScaler);

			ventEngineChillEffect->SetVariableVec3(FName("Force"), EffectDragForce);

			ventEngineChillAudioVolume = oxidizerTank->pressureG * 0.1f * FMath::Clamp(DensityAir, 0.1f, 1.0f);
			ventEngineChillAudio->SetVolumeMultiplier(ventEngineChillAudioVolume);
			ventEngineChillAudio->SetPaused(false);
		}
		else
		{
			ventEngineChillEffect->SetVariableFloat(FName("SpawnRateScale"), 0.0f);
			ventEngineChillAudio->SetPaused(true);
		}
	}
}

void ARocketActor::TankEffects()
{
	/////
	// OXIDIZER
	/////

	float oxidizerLevel = FMath::Clamp(oxidizerTank->liquidLevelNormalized, 0.0f, 1.0f);
	float iceCutoffHeight = 90000.0f;

	if (Altitude < iceCutoffHeight)
	{
		currentIceHeight = iceOxidizerHeight * oxidizerLevel;

		float snowAmount = 0.0f;
		float snowBlendMultiplier = 0.0f;

		// snow amount and blend are for oxidizer and fuel!
		if (Altitude > -50) 
		{

			if (Altitude < iceCutoffHeight)
			{
				snowAmount = 2 * (1.0f - Altitude / iceCutoffHeight);
				snowBlendMultiplier = 1.0f - Altitude / iceCutoffHeight;
			}

			for (UMaterialInstanceDynamic* dynMat : DynMatsMainTanks)
			{
				dynMat->SetScalarParameterValue("height1End", iceOxidizerStart + currentIceHeight);
				dynMat->SetScalarParameterValue("SnowAmount", snowAmount);
				dynMat->SetScalarParameterValue("SnowBlendMultiplier", snowBlendMultiplier);
			}
		}
		else
		{
			for (UMaterialInstanceDynamic* dynMat : DynMatsMainTanks)
			{
				dynMat->SetScalarParameterValue("SnowAmount", 0.0f);
			}
		}

		if (condensationEffect)
		{
			if (currentIceHeight > 2 && Altitude < 12000) {
				condensationEffect->Activate();

				// increase spawnrate with velocity a bit, but decrease with air density
				condensationEffect->SetVariableFloat(FName("SpawnRateScale"), DensityAir * (1.0f + VelocityMag/500.0f));

				// this will make the effect thinner/smaller with lower density
				condensationEffect->SetVariableFloat(FName("DensityAir"), DensityAir);

				// reduce lifetime with decreasing air density and increasing velocity above a certain level
				float lifetimeScaler = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 1.0f), FVector2D(0.03f, 1.0f), DensityAir * 25.0f / (FMath::Max(VelocityMag, 25.0f)) );
				condensationEffect->SetVariableFloat(FName("LifetimeScale"), lifetimeScaler);
				condensationEffect->SetVariableVec3(FName("Force"), EffectDragForce);
				condensationEffect->SetVariableFloat(FName("OxiScale"), oxidizerLevel);
			}
			else {
				condensationEffect->SetVariableFloat(FName("OxiScale"), 0.0f);
			}

		}
	}
	else {
		// deactivate for both oxi and fuel here when above cutoff height

		for (UMaterialInstanceDynamic* dynMat : DynMatsMainTanks)
		{
			dynMat->SetScalarParameterValue("SnowAmount", 0.0f);
		}
		//iceMistOxidizer.enabled = false;
		if (condensationEffect)
		{
			condensationEffect->SetVariableFloat(FName("SpawnRateScale"), 0.0f);
		}
	}

	/////
	// FUEL
	/////

	float fuelLevel = FMath::Clamp(fuelTank->liquidLevelNormalized, 0.0f, 1.0f);

	if (Altitude < iceCutoffHeight)
	{
		currentIceHeight = iceFuelHeight * fuelLevel;
		for (UMaterialInstanceDynamic* dynMat : DynMatsMainTanks)
		{
			dynMat->SetScalarParameterValue("height2End", iceFuelStart + currentIceHeight);
		}

		if (condensationEffect)
		{
			if (currentIceHeight > 2 && Altitude < 12000) {
				condensationEffect->SetVariableFloat(FName("FuelScale"), fuelLevel);
			}
			else
			{
				condensationEffect->SetVariableFloat(FName("FuelScale"), 0.0f);
			}
		}
	}

}

void ARocketActor::explode()
{
	if (!isExploded)
	{
		ExplodeFromCPP();
		isExploded = true;
		oxidizerTank->isExploded = true;
		fuelTank->isExploded = true;
	}
}