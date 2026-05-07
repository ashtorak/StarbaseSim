
#include "Valve.h"
#include "Math/UnrealMathUtility.h"
#include "Kismet/KismetMathLibrary.h"
#include "AsyncTickFunctions.h"
#include <Net/UnrealNetwork.h>
#include "StarbaseSimCommon.h"


// Sets default values for this component's properties
UValve::UValve()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	SetAsyncPhysicsTickEnabled(true);

	SetIsReplicatedByDefault(true);
}

//  needed for replication
void UValve::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UValve, setpoint);
}

// Called when the game starts
void UValve::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void UValve::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	opening = opening - (openingSpeed * (opening - setpoint));
	if (opening < 0.01f) opening = 0;
	else if (opening > 0.99f) opening = 1;

	isOpen = opening > 0 ? true : false;
	isFullyOpen = opening > 0.99f ? true : false;
	isPartiallyOpen = opening > 0.01f && opening < 0.99 ? true : false;

	//if (lever) lever.transform.localEulerAngles = new Vector3(0, leverAngleInitial + (opening * 90), 0);
}

void UValve::AsyncPhysicsTickComponent(float DeltaTime, float SimTime)
{
	_DeltaTime = DeltaTime;

}

void UValve::openClose()
{
	if (setpoint == 1) setpoint = 0;
	else setpoint = 1;
}

void UValve::setOpening(float value)
{
	setpoint = value;
}