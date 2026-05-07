// see readme.txt


#include "CryoPump.h"
#include "Math/UnrealMathUtility.h"
#include "Kismet/KismetMathLibrary.h"
#include "AsyncTickFunctions.h"
#include "Components/AudioComponent.h"
#include <Net/UnrealNetwork.h>
#include "StarbaseSimCommon.h"


// Sets default values for this component's properties
UCryoPump::UCryoPump()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	SetAsyncPhysicsTickEnabled(true);

	SetIsReplicatedByDefault(true);
}

//  needed for replication
void UCryoPump::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCryoPump, isOn);
	DOREPLIFETIME(UCryoPump, power);
	DOREPLIFETIME(UCryoPump, flowrate_in_m3_per_second);
}

// Called when the game starts
void UCryoPump::BeginPlay()
{
	Super::BeginPlay();
	if(AudioLoop) AudioLoop->SetPaused(true);

}

// Called every frame
void UCryoPump::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	
}

void UCryoPump::AsyncPhysicsTickComponent(float DeltaTime, float SimTime)
{
	_SimTime = SimTime;

	//_DeltaTime = DeltaTime;

	if (power >= 0) pressure = nominalPressureHead * FMath::Pow(power, 0.7f);
	else pressure = -(nominalPressureHead * FMath::Pow(-power, 0.7f));
	
	mechanicalPower_in_kW = flowrate_in_m3_per_second * pressure * 100;


	if (isOn)
	{
		if (SimTime > soundStartDelay + soundStartedTime)
		{
			AudioLoop->SetPaused(false);
			loopSound();
		}
	}
	else
	{
		AudioLoop->SetPaused(true);
		AudioStartup->Stop();
	}
}


void UCryoPump::switchOnOff()
{
	isOn = !isOn;
	if (!isOn) power = 0;
	if (isOn) playStartupSound(power);
	else playShutdownSound(power);
}

void UCryoPump::switchOff()
{
	if (isOn)
	{
		isOn = false;
		power = flowrate_in_m3_per_second = 0;
		playShutdownSound(power);
	}
}

void UCryoPump::loopSound()
{
	AudioLoop->SetVolumeMultiplier(AudioVolumeMultiplier * (1 + 0.66f * FMath::Abs(power)));
	AudioLoop->SetPitchMultiplier(1.0f + FMath::Abs(power) * 0.1f);
}

void UCryoPump::playStartupSound(float pitchAdjustment)
{
	AudioStartup->SetVolumeMultiplier(AudioVolumeMultiplier);
	AudioStartup->SetPitchMultiplier(1.0f + (pitchAdjustment * 0.1f));
	AudioStartup->Activate();
	AudioStartup->Play();
	soundStartedTime = _SimTime;

}

void UCryoPump::playShutdownSound(float pitchAdjustment)
{
	AudioShutdown->SetVolumeMultiplier(AudioVolumeMultiplier);
	AudioShutdown->SetPitchMultiplier(1.0f + (pitchAdjustment * 0.1f));
	AudioShutdown->Activate();
	AudioShutdown->Play();
}
