// Fill out your copyright notice in the Description page of Project Settings.


#include "RocketControllerComponent.h"
#include "AeroDynamicsComponent.h"
#include "PIDComponent.h"
#include "RocketEngines.h"
#include "PosControl.h"
#include "Math/UnrealMathUtility.h"
#include "Kismet/KismetMathLibrary.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include "Physics/Experimental/PhysScene_Chaos.h"
#include "Physics/Experimental/ChaosScopedSceneLock.h"
#include <Net/UnrealNetwork.h>
#include "AsyncTickFunctions.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "ProceduralMeshComponent.h"
#include "LyraGame/Settings/LyraSettingsLocal.h"

DEFINE_LOG_CATEGORY(LogRocketController);

// Sets default values for this component's properties
URocketControllerComponent::URocketControllerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	SetAsyncPhysicsTickEnabled(true);

	//  needed for replication
	SetIsReplicatedByDefault(true);

	Rocket = Cast<ARocketActor>(GetOwner());

	PIDGimbalXcpp = CreateDefaultSubobject<UPIDComponent>(TEXT("PID-GimbalX"));
	PIDGimbalYcpp = CreateDefaultSubobject<UPIDComponent>(TEXT("PID-GimbalY"));
	PIDGimbalZcpp = CreateDefaultSubobject<UPIDComponent>(TEXT("PID-GimbalZ"));
	PIDGimbalAngVelX = CreateDefaultSubobject<UPIDComponent>(TEXT("PID-GimbalAngVelX"));
	PIDGimbalAngVelY = CreateDefaultSubobject<UPIDComponent>(TEXT("PID-GimbalAngVelY"));
	PIDGimbalAngVelZ = CreateDefaultSubobject<UPIDComponent>(TEXT("PID-GimbalAngVelZ"));
	PIDDragX = CreateDefaultSubobject<UPIDComponent>(TEXT("PID-DragX"));
	PIDDragY = CreateDefaultSubobject<UPIDComponent>(TEXT("PID-DragY"));
	PIDDragZ = CreateDefaultSubobject<UPIDComponent>(TEXT("PID-DragZ"));
	PIDDragAngVelX = CreateDefaultSubobject<UPIDComponent>(TEXT("PID-DragAngVelX"));
	PIDDragAngVelY = CreateDefaultSubobject<UPIDComponent>(TEXT("PID-DragAngVelY"));
	PIDDragAngVelZ = CreateDefaultSubobject<UPIDComponent>(TEXT("PID-DragAngVelZ"));
	PIDRCSX = CreateDefaultSubobject<UPIDComponent>(TEXT("PID-RCSX"));
	PIDRCSY = CreateDefaultSubobject<UPIDComponent>(TEXT("PID-RCSY"));
	PIDRCSZ = CreateDefaultSubobject<UPIDComponent>(TEXT("PID-RCSZ"));
	PIDRCSAngVelX = CreateDefaultSubobject<UPIDComponent>(TEXT("PID-RCSAngVelX"));
	PIDRCSAngVelY = CreateDefaultSubobject<UPIDComponent>(TEXT("PID-RCSAngVelY"));
	PIDRCSAngVelZ = CreateDefaultSubobject<UPIDComponent>(TEXT("PID-RCSAngVelZ"));

	PID_vel_z = CreateDefaultSubobject<UPIDComponent>(TEXT("PID_vel_z"));
	PID_accel_z = CreateDefaultSubobject<UPIDComponent>(TEXT("PID_accel_z"));

	// Create the procedural mesh component
	TrajectoryMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("TrajectoryMesh"));

	if (GetOwner() && GetOwner()->GetRootComponent())
	{
		TrajectoryMesh->SetupAttachment(GetOwner()->GetRootComponent());
		
		TrajectoryMesh->SetUsingAbsoluteLocation(true);
		TrajectoryMesh->SetUsingAbsoluteRotation(true);
		TrajectoryMesh->SetUsingAbsoluteScale(false);
	}

	// Optimization: The trajectory mesh doesn't need to collide or cast shadows
	TrajectoryMesh->SetCanEverAffectNavigation(false);
	TrajectoryMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TrajectoryMesh->SetCastShadow(false);

}

//  needed for replication
void URocketControllerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(URocketControllerComponent, PositionTarget);
	DOREPLIFETIME(URocketControllerComponent, GimbalAngVelMaxSetpoint);
	DOREPLIFETIME(URocketControllerComponent, AngVelDragMaxSetpoint);
	DOREPLIFETIME(URocketControllerComponent, AngVelRCSMaxSetpoint);
	DOREPLIFETIME(URocketControllerComponent, PropMassTotal);
	DOREPLIFETIME(URocketControllerComponent, PropMassLiquid);
	DOREPLIFETIME(URocketControllerComponent, PropMassGas);
	DOREPLIFETIME(URocketControllerComponent, ThrottleRequest);
	DOREPLIFETIME(URocketControllerComponent, EngineThrust);
	DOREPLIFETIME(URocketControllerComponent, EngineMassFlow);
	DOREPLIFETIME(URocketControllerComponent, useEngineGimbals);
	DOREPLIFETIME(URocketControllerComponent, useGridFins);
	DOREPLIFETIME(URocketControllerComponent, useFlaps);
	DOREPLIFETIME(URocketControllerComponent, foldFlaps);
	DOREPLIFETIME(URocketControllerComponent, useRCS);
	DOREPLIFETIME(URocketControllerComponent, RCSThrust);
	DOREPLIFETIME(URocketControllerComponent, isShipHotStaging);
	DOREPLIFETIME(URocketControllerComponent, isBoosterLanding);
	DOREPLIFETIME(URocketControllerComponent, IsOnPosControlXY);
	DOREPLIFETIME(URocketControllerComponent, IsOnPosControlZ);
	DOREPLIFETIME(URocketControllerComponent, sitsOnOLM);
	DOREPLIFETIME(URocketControllerComponent, GimbalAngleMax);
	DOREPLIFETIME(URocketControllerComponent, isFullySwitchedOff);
	DOREPLIFETIME(URocketControllerComponent, NumberOfEnginesRequested);
	DOREPLIFETIME(URocketControllerComponent, _attitude_target);
	DOREPLIFETIME(URocketControllerComponent, RunScriptArray);
}


// Called when the game starts
void URocketControllerComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	TArray<UActorComponent*> Comps = GetOwner()->GetComponentsByTag(USceneComponent::StaticClass(), "AttitudeTarget");
	if(!Comps.IsEmpty()) AttitudeTargetComponent = Cast<USceneComponent>(Comps[0]);

	// RCS stuff
	xplus = xminus = yplus = yminus = zplus = zminus = false;
	RCS_X_lastTime = RCS_Y_lastTime = RCS_Z_lastTime = 0;

	// position controller
	P_pos_xy = NewObject<UP_2D>();
	PID_vel_xy = NewObject<UPID_2D>();
	P_pos_z = NewObject<UP_1D>();

	pos_control = NewObject<UPosControl>();
	pos_control->RocketController = this;
	
	pos_control->_p_pos_z = P_pos_z;
	pos_control->_pid_vel_z = PID_vel_z;
	pos_control->_pid_accel_z = PID_accel_z;
	pos_control->_p_pos_xy = P_pos_xy;
	pos_control->_pid_vel_xy = PID_vel_xy;

	UpdateZParams();
	PID_vel_z->OutputMin = max_speed_down;
	PID_vel_z->OutputMax = max_speed_up;
	PID_vel_z->bIsOpenLoop = false;
	PID_accel_z->OutputMin = -max_accel_z;
	PID_accel_z->OutputMax = max_accel_z;
	PID_accel_z->bIsOpenLoop = false;
	setXYMaxSpeedAccel(80.0f, 11.0f);

	// Get the GameState for processing script commands
	AGameStateBase* GameState = GetWorld()->GetGameState();
	if (GameState)
	{
		GSBoca = Cast<AGameStateBocaBase>(GameState);
	}

	GameSettings = ULyraSettingsLocal::Get();
}


// Called every frame
void URocketControllerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	slowTickAccumulator += DeltaTime;
	if (slowTickAccumulator > slowTickSeconds)
	{
		SlowTickComponent(slowTickAccumulator);
		slowTickAccumulator = 0;
	}

	// update attitude target indicator rotation
	if (AttitudeTargetComponent)
	{
		AttitudeTargetComponent->SetWorldRotation(_attitude_target);
	}

	



}

// this should be moved into a booster/ship class so GetCenterOfMass() gives correct value tick wise. For now it's ticked from the actor.
FVector URocketControllerComponent::GetOverallCenterOfMass()
{
	if (MainMeshComponent && !isFullySwitchedOff)
	{
		posCoM = MainMeshComponent->GetCenterOfMass();
		FVector localPosEnginesAtCenter = FVector(0, 0, MainMeshComponent->GetSocketTransform(Engines->EngineProperties[0].BoneName, RTS_ParentBoneSpace).GetLocation().Z);
		posEngines = MainMeshComponent->GetComponentTransform().TransformPosition(localPosEnginesAtCenter);
		OverallCenterOfMass = posCoM + (posEngines - posCoM) * TotalMassEngines / TotalMass;

	}
	return OverallCenterOfMass;
}

// Called every slowTickSeconds
void URocketControllerComponent::SlowTickComponent(float DeltaTime)
{
	if (MainMeshComponent && !isFullySwitchedOff && Rocket)
	{
		//UE_LOG(LogRocketController, Log, TEXT("errorZ: %f expAng: %f expAngVel: %f"), _attitude_error_body_Degree.Z, ZexpAng, ZexpAngVel);
		
		/*if (GEngine)
		{
			FString s = FString::Printf(TEXT("x: %f y: %f z: %f"), q.X, q.Y, q.Z);
			GEngine->AddOnScreenDebugMessage(-1, DeltaTime, FColor::White, s);
		}*/
	}
}

void URocketControllerComponent::AsyncPhysicsTickComponent(float DeltaTime, float SimTime)
{
	Update(DeltaTime, SimTime);
}



void URocketControllerComponent::Update(float DeltaTime, float SimTime)
{
	if (MainMeshComponent && !isFullySwitchedOff && Rocket)
	{
		if (partsChanged)
		{
			TotalMassEngines = 0.0f;
			float MassFactor = 1.0f;
			int i = -1;
			for (FEngineProperties& engine : Engines->EngineProperties)
			{
				i++;
				if (Engines->EngineReplicatedStates[i].isExisting)
				{
					if (engine.isRVac) MassFactor = 2.0f;
					else MassFactor = 1.0f;

					TotalMassEngines += MassFactor * Engines->EngineMass; // todo: put mass in each engine property directly
				}
			}

			partsChanged = false;
		}

		MainMeshTransform = UAsyncTickFunctions::ATP_GetTransform(MainMeshComponent);
		
		// update physics state values
		CurrentPosition = MainMeshTransform.GetLocation() * 0.01f;
		Altitude = CurrentPosition.Z;
		DensityAir = 1.2 * FMath::Exp(-Altitude / 10000);

		Velocity = UAsyncTickFunctions::ATP_GetLinearVelocity(MainMeshComponent) * 0.01f;
		VelocityMagnitude = Velocity.Length();

		Acceleration = UKismetMathLibrary::WeightedMovingAverage_FVector((Velocity - VelocityPrevious) / DeltaTime, Acceleration, AccelerationSmoothingFactor);
		BodyAcceleration = MainMeshTransform.InverseTransformVector(Acceleration - Gravity);
		BodyAccelerationMagnitude = BodyAcceleration.Length();

		VelocityPrevious = Velocity;

		// set pos control input

		if (IsOnPosControlXY || IsOnPosControlZ)
		{
			pos_control->input_pos_xyz(PositionTarget, 0, 0, DeltaTime);
		}

		// update throttle controller
		if (GetNetMode() == ENetMode::NM_Standalone || GetNetMode() == ENetMode::NM_ListenServer)
		{
			CalculateThrottle(DeltaTime);
		}

		// update attitude target
		if (IsOnPosControlXY)
		{
			pos_control->update_xy_controller(DeltaTime);
			if (GetNetMode() == ENetMode::NM_Standalone || GetNetMode() == ENetMode::NM_ListenServer)
			{
				_attitude_target = Attitude_from_thrust_vector_rate_heading(pos_control->get_thrust_vector(), YawTargetRate, false, DeltaTime);
			}
		}
		else
		{
			pos_control->is_active_xy = false;
		}
			
		CalculateAngularErrorWithVelocity();

		// reset the position request
		GimbalPos.Set(0, 0, 0);

		CalculateGimbalRequest(0, PIDGimbalXcpp, PIDGimbalAngVelX, DeltaTime);
		CalculateGimbalRequest(1, PIDGimbalYcpp, PIDGimbalAngVelY, DeltaTime);
		CalculateGimbalRequest(2, PIDGimbalZcpp, PIDGimbalAngVelZ, DeltaTime);
		CalculateDragRequest(0, PIDDragX, PIDDragAngVelX, DeltaTime);
		CalculateDragRequest(1, PIDDragY, PIDDragAngVelY, DeltaTime);
		CalculateDragRequest(2, PIDDragZ, PIDDragAngVelZ, DeltaTime);
		CalculateRCSRequest(0, PIDRCSX, PIDRCSAngVelX, DeltaTime);
		CalculateRCSRequest(1, PIDRCSY, PIDRCSAngVelY, DeltaTime);
		CalculateRCSRequest(2, PIDRCSZ, PIDRCSAngVelZ, DeltaTime);

		deltaRate = GimbalRate * DeltaTime;

		int i = -1;
		//if (GetNetMode() == ENetMode::NM_Standalone || GetNetMode() == ENetMode::NM_ListenServer)
		{
			EngineMassFlowTotal = 0;
			EnginesThatAreRunningBitmask = 0;
			// apply thrust force and such
			if (NumberOfEnginesRunning > 0)
			{
				AreEnginesReleased = true;

				Engines->ThrottleRequest = ThrottleRequest;

				CountActiveGimbalEngines = TotalThrust = 0;
				for (FEngineReplicatedStates& engine : Engines->EngineReplicatedStates)
				{
					i++;
					// add force on each engine bone pointing upwards along the bone up axis
					if (engine.isFiring || engine.isFiringUp)
					{
						EnginesThatAreRunningBitmask += static_cast<long long>(1) << i;
						float EngineThrustMagnitude = Engines->EngineProperties[i].Throttle * EngineThrust;
						if (Engines->EngineProperties[i].isRVac) EngineThrustMagnitude *= 1.12f;
						const FTransform t = UAsyncTickFunctions::ATP_GetSocketTransform(MainMeshComponent, Engines->EngineProperties[i].BoneName);
						const FQuat worldRotation = t.GetRotation(); // go via this transform as it is already ATPed
						const FVector force = worldRotation.GetUpVector() * EngineThrustMagnitude * 100;
						UAsyncTickFunctions::ATP_AddForce(MainMeshComponent, force, false, Engines->EngineProperties[i].BoneName);
						//UE_LOG(LogRocketController, Log, TEXT("fX: %f fY: %f fZ: %f bone: %s"), force.X, force.Y, force.Z, *engine.bone.ToString());

						EngineMassFlowTotal += Engines->EngineProperties[i].Throttle * EngineMassFlow;

						if (Engines->EngineProperties[i].isGimbal) CountActiveGimbalEngines++;

						TotalThrust += EngineThrustMagnitude;
					}
				}

				Rocket->UpdateTanksAfterFiring(EngineMassFlowTotal * DeltaTime);
				PropMassLiquid = Rocket->totalLiquidMass;
				PropMassGas = Rocket->totalGasMass;
				PropMassTotal = Rocket->totalPropMass;
				TWR = TotalThrust / ((TotalMass + MassOfShipOnTop) * FMath::Abs(Gravity.Z));

				if (GetNetMode() == ENetMode::NM_Standalone || GetNetMode() == ENetMode::NM_ListenServer)
				{
					if (!isStoppingEngines && (Rocket->oxidizerTank->liquidMass <= 0 || Rocket->fuelTank->liquidMass <= 0))
					{
						StopEnginesCommandFromCPP();
						isStoppingEngines = true; // to have the command only sent once, will be reset from blueprint
					}
					
					if(sitsOnOLM && TWR>1) ReleaseOLMClamps();
				}

				K_factor = FMath::Clamp(FMath::Lerp(0.1f, 2.0f, FMath::Clamp((PropMassTotal / PropCapacity), 0.125f, 1.0f) / (ThrottleRequest * FMath::Clamp(CountActiveGimbalEngines, 1, NumberOfGimbalEngines) / NumberOfGimbalEngines)), 0.0f, 1.0f);
			}
			else
			{
				K_factor = 1;
				AreEnginesReleased = false;
			}


		}
		// update mass every tick
		PropMassTotal = Rocket->totalPropMass;
		float boneMass = TotalMassEngines + TotalMassAeroParts + TotalMassHotStageRing;
		// make sure there is always some mass set to main body
		if (DryMass < (boneMass + 1000)) DryMass = boneMass + 1000;
		// set skeletal mesh physics component accordingly
		MainMeshComponent->SetMassOverrideInKg(NAME_None, PropMassTotal + DryMass - boneMass);

		TotalMass = DryMass + PropMassTotal;

		// calculate TWR_max
		TWR_max = NumberOfEnginesRequested * EngineThrust / ((TotalMass + MassOfShipOnTop) * FMath::Abs(Gravity.Z));

		// set gimbals
		if (useEngineGimbals)
		{
			if (isBooster) hardGimbalMax = hardGimbalMaxBooster;
			
			i = -1;
			for (FEngineProperties& engine : Engines->EngineProperties)
			{
				i++;
				if (engine.isGimbal && Engines->EngineReplicatedStates[i].isExisting)
				{
					// set constraints of gimbals for each engine with fixed gimbal rate
					FConstraintInstance* ci = engine.constraint.Get();
					FRotator OrientationTarget = ci->GetAngularOrientationTarget();

					float offset = FMath::DegreesToRadians(engine.GimbalRotationOffset);

					if (isBoosterLanding && i < 3) _GimbalAngleMax = hardGimbalMaxBoosterLanding;
					else _GimbalAngleMax = hardGimbalMax;

					// internal Vector for calcs
					FVector2d _gimbalPos;

					// only add twist if roll/pitch is low
					float twist = FMath::Clamp(FMath::Sin(offset) * GimbalPos.Z, -15.0f - FMath::Abs(GimbalPos.Y), 15.0f - FMath::Abs(GimbalPos.Y));
					_gimbalPos.X = GimbalPos.X + twist;

					twist = FMath::Clamp(FMath::Cos(offset) * GimbalPos.Z, -15.0f - FMath::Abs(GimbalPos.X), 15.0f - FMath::Abs(GimbalPos.X));
					_gimbalPos.Y = GimbalPos.Y + twist;
					
					// make sure that combined gimbal action is not too large
					if (_gimbalPos.Length() > _GimbalAngleMax)
					{
						_gimbalPos.Normalize();
						_gimbalPos *= _GimbalAngleMax;
					}

					// move hard outwards when staging or landing with booster
					if (isShipHotStaging || (isBoosterLanding && i > 2)) _gimbalPos.X = FMath::Cos(offset) * _GimbalAngleMax;
					if (isShipHotStaging || (isBoosterLanding && i > 2)) _gimbalPos.Y = -FMath::Sin(offset) * _GimbalAngleMax;


					// slowly adjust roll target with deltaRate, unless it's smaller than deltaX
					float deltaX = _gimbalPos.X - OrientationTarget.Roll;

					if (FMath::Abs(deltaX) < deltaRate) OrientationTarget.Roll = _gimbalPos.X;
					else OrientationTarget.Roll = OrientationTarget.Roll + FMath::Sign(deltaX) * deltaRate;

					float deltaY = _gimbalPos.Y - OrientationTarget.Pitch;

					if (FMath::Abs(deltaY) < deltaRate) OrientationTarget.Pitch = _gimbalPos.Y;
					else OrientationTarget.Pitch = OrientationTarget.Pitch + FMath::Sign(deltaY) * deltaRate;
					
					// update the constraint
					ci->SetAngularOrientationTarget(OrientationTarget.Quaternion());
				}
			}

		}

		if (useGridFins)
		{
			float leftQD, left, rightQD, right;
			leftQD=left=rightQD=right=0;

			
				left += DragPos.X;
				leftQD += DragPos.X;
				right -= DragPos.X;
				rightQD -= DragPos.X;
			
			
			if (DragPos.Y > 0)
			{
				leftQD -= DragPos.Y;
				left += DragPos.Y;
			}
			else if (DragPos.Y < 0)
			{
				rightQD -= DragPos.Y;
				right += DragPos.Y;
			}
			
				leftQD += DragPos.Z;
				left += DragPos.Z;
				rightQD += DragPos.Z;
				right += DragPos.Z;
			
			deltaRate = GridFinRate * DeltaTime;
			SetGridFinTarget(GridFinLeft, left);
			SetGridFinTarget(GridFinLeftQD, leftQD);
			SetGridFinTarget(GridFinRight, right);
			SetGridFinTarget(GridFinRightQD, rightQD);
		}
		else
		{
			SetGridFinTarget(GridFinLeft, 0);
			SetGridFinTarget(GridFinLeftQD, 0);
			SetGridFinTarget(GridFinRight, 0);
			SetGridFinTarget(GridFinRightQD, 0);
		}

		if (useFlaps)
		{
			foldFlaps = false;

			float AftLeft, AftRight, FrontLeft, FrontRight;
			AftLeft = AftRight = FrontLeft = FrontRight = 0;

			AftLeft = AftRight = DragPos.X;
			FrontLeft = FrontRight = -DragPos.X;
		
			if (DragPos.Y > 0)
			{
				AftLeft -= DragPos.Y;
				FrontRight -= DragPos.Y;
			}
			else if (DragPos.Y < 0)
			{
				AftRight += DragPos.Y;
				FrontLeft += DragPos.Y;
			}

			AftRight += DragPos.Z;
			FrontRight += DragPos.Z;
			AftLeft -= DragPos.Z;
			FrontLeft -= DragPos.Z;

			deltaRate = FlapsRate * DeltaTime;
			SetFlapTarget(FlapAftLeft, AftLeft, FlapAftMaxTarget);
			SetFlapTarget(FlapAftRight, AftRight, FlapAftMaxTarget);
			SetFlapTarget(FlapFrontLeft, FrontLeft, FlapFrontMaxTarget);
			SetFlapTarget(FlapFrontRight, FrontRight, FlapFrontMaxTarget);
		}
		else if (foldFlaps)
		{
			if (flapFolder >= 1.0f) // flapFolder is a timer, so when you flip bool, it counts up/down and then executes the fold/unfold
			{
				useFlaps = false;
				SetFlapTarget(FlapAftLeft, 1.0f, FlapAftMaxTarget);
				SetFlapTarget(FlapAftRight, 1.0f, FlapAftMaxTarget);
				SetFlapTarget(FlapFrontLeft, 1.0f, FlapAftMaxTarget); // for folding front flaps can be folded more in case they weren't (block 2)
				SetFlapTarget(FlapFrontRight, 1.0f, FlapAftMaxTarget);
			}
			else flapFolder += 3*DeltaTime;
		}
		else
		{
			if (flapFolder <= -1.0f)
			{
				SetFlapTarget(FlapAftLeft, -1.0f, FlapAftMaxTarget);
				SetFlapTarget(FlapAftRight, -1.0f, FlapAftMaxTarget);
				SetFlapTarget(FlapFrontLeft, -1.0f, FlapAftMaxTarget);
				SetFlapTarget(FlapFrontRight, -1.0f, FlapAftMaxTarget);
			}
			else flapFolder -= 3 * DeltaTime;
		}

		if (useRCS)
		{
			RCSWasOn = true;

			float AbsOutput = FMath::Abs(RCSPos.X);

			if (RCSPos.X > RCSThreshold)
			{
				xminus = false;
				if (xplus && SimTime > RCS_X_lastTime + AbsOutput)
				{
					xplus = false;
					RCS_X_lastTime = SimTime;
				}
				else if (!xplus && SimTime > RCS_X_lastTime + 1 - AbsOutput)
				{
					xplus = true;
					RCS_X_lastTime = SimTime;
				}
			}
			else if (RCSPos.X < -RCSThreshold)
			{
				xplus = false;
				if (xminus && SimTime > RCS_X_lastTime + AbsOutput)
				{
					xminus = false;
					RCS_X_lastTime = SimTime;
				}
				else if (!xminus && SimTime > RCS_X_lastTime + 1 - AbsOutput)
				{
					xminus = true;
					RCS_X_lastTime = SimTime;
				}
			}
			else // thruster off
			{
				xplus = xminus = false;
				for (URCSThruster* thruster : RCSThrusters)
				{
					if ( thruster->isExisting && ( thruster->xplus || thruster->xminus) ) thruster->vent->SetVariableFloat(FName("SpawnRate"), 0);
				}
			}

			AbsOutput = FMath::Abs(RCSPos.Y);

			if (RCSPos.Y > RCSThreshold)
			{
				yminus = false;
				if (yplus && SimTime > RCS_Y_lastTime + AbsOutput)
				{
					yplus = false;
					RCS_Y_lastTime = SimTime;
				}
				else if (!yplus && SimTime > RCS_Y_lastTime + 1 - AbsOutput)
				{
					yplus = true;
					RCS_Y_lastTime = SimTime;
				}
			}
			else if (RCSPos.Y < -RCSThreshold)
			{
				yplus = false;
				if (yminus && SimTime > RCS_Y_lastTime + AbsOutput)
				{
					yminus = false;
					RCS_Y_lastTime = SimTime;
				}
				else if (!yminus && SimTime > RCS_Y_lastTime + 1 - AbsOutput)
				{
					yminus = true;
					RCS_Y_lastTime = SimTime;
				}
			}
			else // thruster off
			{
				yplus = yminus = false;
				for (URCSThruster* thruster : RCSThrusters)
				{
					if (thruster->isExisting && (thruster->yplus || thruster->yminus)) thruster->vent->SetVariableFloat(FName("SpawnRate"), 0);
				}
			}

			AbsOutput = FMath::Abs(RCSPos.Z);

			if (RCSPos.Z > RCSThreshold)
			{
				zminus = false;
				if (zplus && SimTime > RCS_Z_lastTime + AbsOutput)
				{
					zplus = false;
					RCS_Z_lastTime = SimTime;
				}
				else if (!zplus && SimTime > RCS_Z_lastTime + 1 - AbsOutput)
				{
					zplus = true;
					RCS_Z_lastTime = SimTime;
				}
			}
			else if (RCSPos.Z < -RCSThreshold)
			{
				zplus = false;
				if (zminus && SimTime > RCS_Z_lastTime + AbsOutput)
				{
					zminus = false;
					RCS_Z_lastTime = SimTime;
				}
				else if (!zminus && SimTime > RCS_Z_lastTime + 1 - AbsOutput)
				{
					zminus = true;
					RCS_Z_lastTime = SimTime;
				}
			}
			else // thruster off
			{
				zplus = zminus = false;
				for (URCSThruster* thruster : RCSThrusters)
				{
					if (thruster->isExisting && (thruster->zplus || thruster->zminus)) thruster->vent->SetVariableFloat(FName("SpawnRate"), 0);
				}
			}
			
			for (URCSThruster* thruster : RCSThrusters)
			{
				if (thruster->isExisting)
				{
					if (IsValid(thruster->vent))
					{
						bool requestFiring = (xplus & thruster->xplus) | (xminus & thruster->xminus) | (yplus & thruster->yplus) | (yminus & thruster->yminus) | (zplus & thruster->zplus) | (zminus & thruster->zminus);

						if (requestFiring)
						{
							float pressureScaler = FMath::Abs(Rocket->oxidizerTank->pressureG) / Rocket->oxidizerTank->nominalPressureG;

							if (pressureScaler > 0.01f)
							{
								thruster->isFiring = true;

								tBoneLocal = MainMeshComponent->GetSocketTransform(thruster->bone, RTS_ParentBoneSpace);
								RCSForcePos = MainMeshTransform.TransformPosition(tBoneLocal.GetLocation());
								FVector pos = MainMeshTransform.TransformPosition(tBoneLocal.GetLocation());
								if (thruster->bone == "RCS_CowBell") RCSForceVectorCalc = MainMeshTransform.TransformRotation(tBoneLocal.GetRotation()).GetUpVector() * ShipCowBellReductionFactor * RCSThrust * 100 * pressureScaler;
								else RCSForceVectorCalc = MainMeshTransform.TransformRotation(tBoneLocal.GetRotation()).GetUpVector() * RCSThrust * 100 * pressureScaler;
								FVector force = RCSForceVectorCalc;
								FVector WorldCOM = UAsyncTickFunctions::ATP_AddForceAtPositionR(MainMeshComponent, pos, force);

								if (IsValid(thruster->vent))	thruster->vent->SetVariableFloat(FName("SpawnRate"), 222);

								thruster->vent->SetVariableFloat(FName("DensityAir"), DensityAir);
								// reduce lifetime with decreasing air density and increasing velocity
								float lifetimeScaler = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 1.0f), FVector2D(0.33f, 1.0f), (1.0f - (FMath::Min(VelocityMagnitude, 100.0f) / 100.0f)) * DensityAir);
								thruster->vent->SetVariableFloat(FName("LifetimeScale"), lifetimeScaler);

								thruster->vent->SetVariableVec3(FName("Force"), Rocket->EffectDragForce);

								float speedScaler = FMath::Clamp(FMath::Pow(pressureScaler / 2.0f, Rocket->oxidizerTank->steamSpeedAdjustment), 0.01f, 33.0f);
								thruster->vent->SetVariableFloat(FName("SpeedScale"), speedScaler);

								thruster->vent->SetVariableFloat(FName("SizeScale"), pressureScaler * Rocket->oxidizerTank->steamSizeAdjustment);

								Rocket->oxidizerTank->addGasVolumeExternalDelta(-0.1f * Rocket->oxidizerTank->pressureG * DeltaTime);
							}
							else
							{
								thruster->isFiring = false;
								thruster->vent->SetVariableFloat(FName("SpawnRate"), 0);
							}
						}
						else
						{
							thruster->isFiring = false;
							thruster->vent->SetVariableFloat(FName("SpawnRate"), 0);
						}
					}
					else thruster->isFiring = false;
				}
			}
		}
		else if(RCSWasOn) // reset RCS state when it was on and useRCS is set to false
		{
			xplus = xminus = yplus = yminus = zplus = zminus = false;
			for (URCSThruster* thruster : RCSThrusters)
			{
				thruster->isFiring = false;
				thruster->vent->SetVariableFloat(FName("SpawnRate"), 0);
			}
			RCSWasOn = false;
		}

		////////////////////////
		/// event based script
		////////////////////////

		if (RunScriptArray)
		{
			if (ScriptCommands.IsEmpty())
			{
				RunScriptArray = false;
				ScriptCommandsIterator = 0;
			}
			else
			{
				if (ScriptCommandsIterator < ScriptCommands.Num())
				{
					bool RunStep = true;
					while(RunStep) // run every item for which one of the conditions is true
					{
						if (ScriptType == 0) RunStep = SimTime > ScriptStartTime + ScriptCommands[ScriptCommandsIterator].EventFloat;
						else if (ScriptType == 1) RunStep = Altitude <= ScriptCommands[ScriptCommandsIterator].EventFloat;
						else if (ScriptType == 2) RunStep = VelocityMagnitude <= ScriptCommands[ScriptCommandsIterator].EventFloat && VelocityMagnitude != 0; // && since at start it will be 0
						if (RunStep)
						{
							GSBoca->ProcessCommands(ScriptCommands[ScriptCommandsIterator], Rocket);
							ScriptCommandsIterator++;
							if (ScriptCommandsIterator >= ScriptCommands.Num()) break;
						}
						else continue;
					}
					
				}
				else ScriptCommands.Empty(); // we are through all commands and can clear the array

			}
		}

		////////////////////////
		/// anti-gravity when going above 3 km/s
		////////////////////////

		if (GameSettings->bAntiGravity)
		{
			// sharp transit to max force when going near 3000 m/s
			double MappedValue = UKismetMathLibrary::MapRangeClamped(
				VelocityMagnitude,
				0.0, 3000.0, // InRange A & B
				0.0, 0.975   // OutRange A & B
			);

			double PoweredValue = UKismetMathLibrary::MultiplyMultiply_FloatFloat(MappedValue, 2.0f);

			// up force with mass * g * 100
			double FinalZForce = PoweredValue * (TotalMass * 980.0);

			FVector ForceVector = FVector(0.0, 0.0, FinalZForce);

			UAsyncTickFunctions::ATP_AddForce(MainMeshComponent, ForceVector, false, NAME_None);
		}

		////////////////////////
		/// trajectory prediction
		////////////////////////

		if (TrajectoryMesh && bTrajectoryEnabled)
		{
			// Re-calculate trajectory

			//if (GetWorld()->GetTimeSeconds() - LastTrajectoryUpdateTime > TrajectoryUpdateInterval)
			{
				UpdateTrajectoryPrediction();
				LastTrajectoryUpdateTime = GetWorld()->GetTimeSeconds();
			}
		}

		if (TrajectoryMesh)
		{
			TrajectoryMesh->SetVectorParameterValueOnMaterials(TEXT("RocketPos"), GetOwner()->GetActorLocation());
		}
	}

}

void URocketControllerComponent::SetGridFinTarget(FConstraintInstanceAccessor& GridFin, float newTarget)
{
	if (FConstraintInstance* GridFinCI = GridFin.Get())
	{
		FRotator GridFinOrientationTarget = GridFinCI->GetAngularOrientationTarget();

		newTarget = FMath::Clamp(newTarget, -1.0f, 1.0f) * GridFinMax;
		float delta = newTarget - GridFinOrientationTarget.Roll;
		if (FMath::Abs(delta) < deltaRate) GridFinOrientationTarget.Roll = newTarget;
		else GridFinOrientationTarget.Roll = GridFinOrientationTarget.Roll + FMath::Sign(delta) * deltaRate;

		GridFinCI->SetAngularOrientationTarget(GridFinOrientationTarget.Quaternion());
	}
}

void URocketControllerComponent::SetFlapTarget(FConstraintInstanceAccessor& Flap, float newTarget, float maxTarget)
{
	if (FConstraintInstance* CI = Flap.Get())
	{
		FRotator GridFinOrientationTarget = CI->GetAngularOrientationTarget();

		newTarget = ( 1.0f + FMath::Clamp(newTarget, -1.0f, 1.0f) ) * 0.5f * maxTarget;
		float delta = newTarget - GridFinOrientationTarget.Yaw;
		if (FMath::Abs(delta) < deltaRate) GridFinOrientationTarget.Yaw = newTarget;
		else GridFinOrientationTarget.Yaw = GridFinOrientationTarget.Yaw + FMath::Sign(delta) * deltaRate;

		CI->SetAngularOrientationTarget(GridFinOrientationTarget.Quaternion());
	}
}

void URocketControllerComponent::CalculateThrottle(float DeltaTime)
{
	if (IsOnPosControlZ)
	{
		if (useHoverThrottle)
		{
			HoverThrottle = FMath::Clamp( TotalMass * -Gravity.Z / (EngineThrust * NumberOfEnginesRunning), 0.2f, 1.0f);
			ThrottleRequest = FMath::Clamp(pos_control->update_z_controller(DeltaTime), 0.2f, 1.0f);
		}
		else
		{
			HoverThrottle = 0.0f;
			ThrottleRequest += FMath::Clamp(pos_control->update_z_controller(DeltaTime), 0.2f, 1.0f);
		}

	}
	else
	{
		pos_control->is_active_z = false;
	}
}

// pidNumbers relate to x,y,z to get corresponding component from the vectors
void URocketControllerComponent::CalculateGimbalRequest(int pidNumber, TObjectPtr<class UPIDComponent> pidGimbal, TObjectPtr<class UPIDComponent> pidAngVel, float DeltaTime)
{
	// calculate angular error and output (setpoint is zero always as we want to turn it to the target)
	pidGimbal->ProcessValue = _attitude_error_body_Degree[pidNumber];
	pidGimbal->K_factor = pidAngVel->K_factor = K_factor;
	pidGimbal->CalculateOutput(DeltaTime);

	// if angular controller is in open loop just take this one as final pos request, else add velocity controller output
	if (pidGimbal->bIsOpenLoop) GimbalPos[pidNumber] = pidGimbal->Output;
	else
	{
		float AngVel = AngularVelocityBody_Degree[pidNumber];
		float AngVelMax = GimbalAngVelMaxSetpoint[pidNumber];

		// angular velocity controller output is added based on velocity magnitude and direction to limit the velocity
		pidAngVel->Setpoint = FMath::Sign(AngVel) * AngVelMax; // setpoint reference is a single abs value here, but if body velocity is negative the setpoint should also be
		pidAngVel->ProcessValue = AngVel;
		pidAngVel->CalculateOutput(DeltaTime);

		// Simple fade between the two contollers. Doesn't work great like this.
		float expAng = 1.0f / (1.0f + FMath::Exp(10.0f / AngVelMax * (FMath::Abs(AngVel) - 1.2f * AngVelMax)));
		float expAngVel = 1.0f / (1.0f + FMath::Exp(-10.0f / AngVelMax * (FMath::Abs(AngVel) - AngVelMax)));
		GimbalPos[pidNumber] = FMath::Clamp(pidGimbal->Output * expAng + GimbalDirectionAngVel * pidAngVel->Output * expAngVel, pidGimbal->OutputMin, pidGimbal->OutputMax);
	}
}

// pidNumbers relate to x,y,z to get corresponding component from the vectors
void URocketControllerComponent::CalculateDragRequest(int pidNumber, TObjectPtr<class UPIDComponent> pidAng, TObjectPtr<class UPIDComponent> pidAngVel, float DeltaTime)
{
	// calculate angular error and output (setpoint is zero always as we want to turn it to the target)
	pidAng->ProcessValue = _attitude_error_body_Degree[pidNumber];
	pidAng->CalculateOutput(DeltaTime);

	// if angular controller is in open loop just take this one as final pos request, else add velocity controller output
	if (pidAng->bIsOpenLoop) DragPos[pidNumber] = pidAng->Output;
	else
	{
		float AngVel = AngularVelocityBody_Degree[pidNumber];
		float AngVelMax = AngVelDragMaxSetpoint[pidNumber];
		pidAngVel->Setpoint = FMath::Sign(AngVel) * AngVelMax;
		pidAngVel->ProcessValue = AngVel;
		pidAngVel->CalculateOutput(DeltaTime);

		float expAng = 1.0f / (1.0f + FMath::Exp( 10.0f / AngVelMax * (FMath::Abs(AngVel) - 1.2f*AngVelMax)));
		float expAngVel = 1.0f / (1.0f + FMath::Exp( -10.0f / AngVelMax * (FMath::Abs(AngVel) - AngVelMax)));
		DragPos[pidNumber] = FMath::Clamp(pidAng->Output * expAng + GimbalDirectionAngVel * pidAngVel->Output * expAngVel, pidAng->OutputMin, pidAng->OutputMax);
		if (pidNumber == 2) {
			ZexpAng = expAng;
			ZexpAngVel = expAngVel;
		}
	}
}

// pidNumbers relate to x,y,z to get corresponding component from the vectors
void URocketControllerComponent::CalculateRCSRequest(int pidNumber, TObjectPtr<class UPIDComponent> pidAng, TObjectPtr<class UPIDComponent> pidAngVel, float DeltaTime)
{
	// calculate angular error and output (setpoint is zero always as we want to turn it to the target)
	pidAng->ProcessValue = _attitude_error_body_Degree[pidNumber];
	pidAng->CalculateOutput(DeltaTime);

	// if angular controller is in open loop just take this one as final pos request, else add velocity controller output
	if (pidAng->bIsOpenLoop) RCSPos[pidNumber] = pidAng->Output;
	else
	{
		float AngVel = AngularVelocityBody_Degree[pidNumber];
		float AngVelMax = AngVelRCSMaxSetpoint[pidNumber];

		// angular velocity controller output is added based on velocity magnitude and direction to limit the velocity
		pidAngVel->Setpoint = FMath::Sign(AngVel) * AngVelMax; // setpoint reference is a single abs value here, but if body velocity is negative the setpoint should also be
		pidAngVel->ProcessValue = AngVel;
		pidAngVel->CalculateOutput(DeltaTime);

		float expAng = 1.0f / (1.0f + FMath::Exp(10.0f / AngVelMax * (FMath::Abs(AngVel) - 1.2f * AngVelMax)));
		float expAngVel = 1.0f / (1.0f + FMath::Exp(-10.0f / AngVelMax * (FMath::Abs(AngVel) - AngVelMax)));
		RCSPos[pidNumber] = FMath::Clamp(pidAng->Output * expAng + GimbalDirectionAngVel * pidAngVel->Output * expAngVel, pidAng->OutputMin, pidAng->OutputMax);
	}
}


void URocketControllerComponent::CalculateAngularErrorWithVelocity()
{
	if (MainMeshComponent)
	{
		AttitudeBody = MainMeshTransform.GetRotation();
		// this is weird: getting rotation from atp transform gives a different wrapping behaviour with large angles, instead we use normal game thread function here that is also used for target rotation

		// This vector represents the angular error to rotate the thrust vector using x and y and heading using z
		FVector attitude_error;
		thrust_heading_rotation_angles(_attitude_target, AttitudeBody, attitude_error, _thrust_angle, _thrust_error_angle);

		// Compute the angular velocity corrections in the body frame from the attitude error
		_attitude_error_body_Degree = update_attitude_target_from_att_error(attitude_error);

		// ensure angular velocity does not go over configured limits
		//ang_vel_limit_Degree(_attitude_error_body_Degree, _ang_vel_roll_max, _ang_vel_pitch_max, _ang_vel_yaw_max);

		// rotation from the target frame to the body frame
		FQuat rotation_target_to_body = AttitudeBody.Inverse() * _attitude_target;

		// target angle velocity vector in the body frame
		//FVector ang_vel_body_feedforward = rotation_target_to_body * _ang_vel_target;

		// get angular velocity of body in body frame
		AngularVelocityBody_Degree = FMath::RadiansToDegrees(AttitudeBody.Inverse().RotateVector(UAsyncTickFunctions::ATP_GetAngularVelocity(MainMeshComponent)));

		// Correct the thrust vector and smoothly add feedforward and yaw input
		float _feedforward_scalar = 1.0f;
		// adjusting gimbal error not really needed here?
		/*if (_thrust_error_angle > ThrustVectorErrorRollLimit * 2.0f) {
			_attitude_error_body_Degree.Z = AngularVelocityBody_Degree.Z;
		}

		else if (_thrust_error_angle > ThrustVectorErrorRollLimit) {
			_feedforward_scalar = (1.0f - (_thrust_error_angle - ThrustVectorErrorRollLimit) / ThrustVectorErrorRollLimit);
			_attitude_error_body_Degree.X += ang_vel_body_feedforward.X * _feedforward_scalar;
			_attitude_error_body_Degree.Y += ang_vel_body_feedforward.Y * _feedforward_scalar;
			_attitude_error_body_Degree.Z += ang_vel_body_feedforward.Z;
			_attitude_error_body_Degree.Z = AngularVelocityBody_Degree.Z * (1.0 - _feedforward_scalar) + _attitude_error_body_Degree.Z * _feedforward_scalar;
		}
		else {
			_attitude_error_body_Degree += ang_vel_body_feedforward;
		}*/

		// don't control x and y when it's rotating very fast around it's up axis
		if (AngularVelocityBody_Degree.Z > 2 * AngularVelocityZLimit_Degree)
		{
			_attitude_error_body_Degree.X = 0;
			_attitude_error_body_Degree.Y = 0;
		}
		else if (AngularVelocityBody_Degree.Z > AngularVelocityZLimit_Degree)
		{
			_feedforward_scalar = (1.0f - (AngularVelocityBody_Degree.Z - AngularVelocityZLimit_Degree) / AngularVelocityZLimit_Degree);
			_attitude_error_body_Degree.X *= _feedforward_scalar;
			_attitude_error_body_Degree.Y *= _feedforward_scalar;
		}

		/*
		if (_rate_bf_ff_enabled) {
			// rotate target and normalize
			Quaternion attitude_target_update;
			attitude_target_update.from_axis_angle(Vector3f{ _ang_vel_target.x * _dt, _ang_vel_target.y * _dt, _ang_vel_target.z * _dt });
			_attitude_target = _attitude_target * attitude_target_update;
		}

		// ensure Quaternion stay normalised
		_attitude_target.normalize();

		// Record error to handle EKF resets
		_attitude_ang_error = attitude_body.inverse() * _attitude_target;
		*/


	}

}

// thrust_heading_rotation_angles - calculates two ordered rotations to move the attitude_body quaternion to the attitude_target quaternion.
// The maximum error in the yaw axis is limited based on static output saturation.
void URocketControllerComponent::thrust_heading_rotation_angles(FQuat& attitude_target, const FQuat& attitude_body, FVector& attitude_error, float& thrust_angle, float& thrust_error_angle)
{
	FQuat thrust_vector_correction;
	thrust_vector_rotation_angles(attitude_target, attitude_body, thrust_vector_correction, attitude_error, thrust_angle, thrust_error_angle);

	// Limit Yaw Error
	float max = FMath::DegreesToRadians(AngularErrorZLimit_Degree);
	attitude_error.Z = FMath::Clamp(attitude_error.Z, -max, max);
	heading_vec_correction_quat.MakeFromRotationVector(FVector{ 0.0f, 0.0f, attitude_error.Z });
	attitude_target = attitude_body * thrust_vector_correction * heading_vec_correction_quat;
}

// thrust_vector_rotation_angles - calculates two ordered rotations to move the attitude_body quaternion to the attitude_target quaternion.
// The first rotation corrects the thrust vector and the second rotation corrects the heading vector.
void URocketControllerComponent::thrust_vector_rotation_angles(const FQuat& attitude_target, const FQuat& attitude_body, FQuat& thrust_vector_correction, FVector& attitude_error, float& thrust_angle, float& thrust_error_angle)
{
	// The direction of thrust is [0,0,1] in any body-fixed frame, inc. body frame and target frame.
	const FVector thrust_vector_up{ 0.0f, 0.0f, 1.0f };

	// attitude_target and attitude_body are passive rotations from target / body frames to the NED frame

	// Rotating [0,0,1] by attitude_target expresses (gets a view of) the target thrust vector in the inertial frame
	FVector att_target_thrust_vec = attitude_target.RotateVector(thrust_vector_up); // target thrust vector

	FVector att_body_thrust_vec = attitude_body.RotateVector(thrust_vector_up); // current thrust vector

	// the dot product is used to calculate the current lean angle for use of external functions
	thrust_angle = acosf(FMath::Clamp(thrust_vector_up.Dot(att_body_thrust_vec), -1.0f, 1.0f));

	// the cross product of the desired and target thrust vector defines the rotation vector
	FVector thrust_vec_cross = att_body_thrust_vec.Cross(att_target_thrust_vec);

	// the dot product is used to calculate the angle between the target and desired thrust vectors
	thrust_error_angle = acosf(FMath::Clamp(att_body_thrust_vec.Dot(att_target_thrust_vec), -1.0f, 1.0f));

	// Normalize the thrust rotation vector
	float thrust_vector_length = thrust_vec_cross.Length();
	if (FMath::IsNearlyZero(thrust_vector_length) || FMath::IsNearlyZero(thrust_error_angle)) {
		thrust_vec_cross = thrust_vector_up;
	}
	else {
		thrust_vec_cross /= thrust_vector_length;
	}

	// thrust_vector_correction is defined relative to the body frame but its axis `thrust_vec_cross` was computed in
	// the inertial frame. First rotate it by the inverse of attitude_body to express it back in the body frame
	thrust_vec_cross = attitude_body.Inverse().RotateVector(thrust_vec_cross);
	thrust_vec_cross.Normalize();
	thrust_vector_correction = thrust_vector_correction.MakeFromRotationVector(thrust_vec_cross*thrust_error_angle);

	// calculate the angle error in x and y.
	FVector rotation;
	thrust_vector_correction.Normalize();
	rotation = thrust_vector_correction.ToRotationVector();
	attitude_error.X = rotation.X;
	attitude_error.Y = rotation.Y;

	// calculate the remaining rotation required after thrust vector is rotated transformed to the body frame
	// heading_vector_correction
	heading_vec_correction_quat = thrust_vector_correction.Inverse() * attitude_body.Inverse() * attitude_target;

	// the following is a suggestion by Grok as the ToRotationVector() function is not quite doing the same as
	// the one from the arducopter library and there was a problem when the error was approaching zero when the body
	// rotated from + to - 180°. It actually seems to work! :D (need to check rest of code for similar issues...)

	// Compute Z (yaw) error directly from the quaternion, avoiding ToRotationVector
    // Extract the yaw error in radians, assuming the quaternion represents a rotation around Z after thrust correction
	float w = heading_vec_correction_quat.W;
	float z = heading_vec_correction_quat.Z;
	float sin_half_angle = FMath::Sqrt(z * z + w * w); // Magnitude of Z-W plane
	float yaw_error = 2.0f * FMath::Atan2(z, w); // Yaw angle in radians

	if (FMath::IsNearlyZero(sin_half_angle)) {
		yaw_error = 0.0f; // Avoid division by zero for identity quaternion
	}

	// Wrap yaw_error to [-pi, pi]
	yaw_error = FMath::Fmod(yaw_error + 3.0f * PI, 2.0f * PI) - PI;

	// Assign Z error (in radians)
	attitude_error.Z = yaw_error;
	rot_z = yaw_error; // For logging

	att_err_z = attitude_error.Z;
}

// Update rate_target_ang_vel using attitude_error_rot_vec_rad
FVector URocketControllerComponent::update_attitude_target_from_att_error(const FVector& attitude_error_rot_vec_rad)
{
	FVector rate_target_ang_vel;

	rate_target_ang_vel = FMath::RadiansToDegrees(attitude_error_rot_vec_rad) * GimbalErrorExtraKp; // degree because controllers were tuned like that earlier, can be reverted to rad when overhauling
	/*
	// Compute the roll angular velocity demand from the roll angle error
	const float angleP_roll = _p_angle_roll.kP() * _angle_P_scale.x;
	if (_use_sqrt_controller && !is_zero(get_accel_roll_max_radss())) {
		rate_target_ang_vel.x = sqrt_controller(attitude_error_rot_vec_rad.x, angleP_roll, constrain_float(get_accel_roll_max_radss() / 2.0f, AC_ATTITUDE_ACCEL_RP_CONTROLLER_MIN_RADSS, AC_ATTITUDE_ACCEL_RP_CONTROLLER_MAX_RADSS), _dt);
	}
	else {
		rate_target_ang_vel.x = angleP_roll * attitude_error_rot_vec_rad.x;
	}

	// Compute the pitch angular velocity demand from the pitch angle error
	const float angleP_pitch = _p_angle_pitch.kP() * _angle_P_scale.y;
	if (_use_sqrt_controller && !is_zero(get_accel_pitch_max_radss())) {
		rate_target_ang_vel.y = sqrt_controller(attitude_error_rot_vec_rad.y, angleP_pitch, constrain_float(get_accel_pitch_max_radss() / 2.0f, AC_ATTITUDE_ACCEL_RP_CONTROLLER_MIN_RADSS, AC_ATTITUDE_ACCEL_RP_CONTROLLER_MAX_RADSS), _dt);
	}
	else {
		rate_target_ang_vel.y = angleP_pitch * attitude_error_rot_vec_rad.y;
	}

	// Compute the yaw angular velocity demand from the yaw angle error
	const float angleP_yaw = _p_angle_yaw.kP() * _angle_P_scale.z;
	if (_use_sqrt_controller && !is_zero(get_accel_yaw_max_radss())) {
		rate_target_ang_vel.z = sqrt_controller(attitude_error_rot_vec_rad.z, angleP_yaw, constrain_float(get_accel_yaw_max_radss() / 2.0f, AC_ATTITUDE_ACCEL_Y_CONTROLLER_MIN_RADSS, AC_ATTITUDE_ACCEL_Y_CONTROLLER_MAX_RADSS), _dt);
	}
	else {
		rate_target_ang_vel.z = angleP_yaw * attitude_error_rot_vec_rad.z;
	}

	// reset angle P scaling, saving used value
	_angle_P_scale_used = _angle_P_scale;
	_angle_P_scale = VECTORF_111;
	*/
	return rate_target_ang_vel;
}

// limits angular velocity
void URocketControllerComponent::ang_vel_limit_Degree(FVector& euler, float ang_vel_roll_max, float ang_vel_pitch_max, float ang_vel_yaw_max) const
{
	if (FMath::IsNearlyZero(ang_vel_roll_max) || FMath::IsNearlyZero(ang_vel_pitch_max)) {
		if (!FMath::IsNearlyZero(ang_vel_roll_max)) {
			euler.X = FMath::Clamp(euler.X, -ang_vel_roll_max, ang_vel_roll_max);
		}
		if (!FMath::IsNearlyZero(ang_vel_pitch_max)) {
			euler.Y = FMath::Clamp(euler.Y, -ang_vel_pitch_max, ang_vel_pitch_max);
		}
	}
	else {
		FVector2f thrust_vector_ang_vel(euler.X / ang_vel_roll_max, euler.Y / ang_vel_pitch_max);
		float thrust_vector_length = thrust_vector_ang_vel.Length();
		if (thrust_vector_length > 1.0f) {
			euler.X = thrust_vector_ang_vel.X * ang_vel_roll_max / thrust_vector_length;
			euler.Y = thrust_vector_ang_vel.Y * ang_vel_pitch_max / thrust_vector_length;
		}
	}
	if (!FMath::IsNearlyZero(ang_vel_yaw_max)) {
		euler.Z = FMath::Clamp(euler.Z, -ang_vel_yaw_max, ang_vel_yaw_max);
	}
}


// Command a thrust vector and heading rate
FQuat URocketControllerComponent::Attitude_from_thrust_vector_rate_heading(const FVector3f& thrust_vector, float heading_rate, bool slew_yaw, float DeltaTime)
{
	if (slew_yaw) {
		// a zero _angle_vel_yaw_max means that setting is disabled
		/*const float slew_yaw_max_rads = get_slew_yaw_max_rads();
		heading_rate = constrain_float(heading_rate, -slew_yaw_max_rads, slew_yaw_max_rads);*/
	}

	// convert thrust vector to a quaternion attitude
	FQuat thrust_vec_quat = attitude_from_thrust_vector(thrust_vector, 0.0f);

	// calculate the angle error in x and y.
	float thrust_vector_diff_angle;
	FQuat thrust_vec_correction_quat;
	FVector attitude_error;
	float returned_thrust_vector_angle;
	thrust_vector_rotation_angles(thrust_vec_quat, _attitude_target, thrust_vec_correction_quat, attitude_error, returned_thrust_vector_angle, thrust_vector_diff_angle);

	FQuat yaw_quat;
	yaw_quat = yaw_quat.MakeFromRotationVector(FVector{ 0.0f, 0.0f, 1.0f } * heading_rate * DeltaTime);
	
	return _attitude_target * thrust_vec_correction_quat * yaw_quat;
}

FQuat URocketControllerComponent::attitude_from_thrust_vector(FVector3f thrust_vector, float heading_angle)
{
	const FVector3f thrust_vector_up{ 0.0f, 0.0f, 1.0f };

	if (FMath::IsNearlyZero(thrust_vector.SquaredLength())) {
		thrust_vector = thrust_vector_up;
	}
	else {
		thrust_vector.Normalize();
	}

	// the cross product of the desired and target thrust vector defines the rotation vector
	FVector3f thrust_vec_cross = thrust_vector_up.Cross(thrust_vector);

	// the dot product is used to calculate the angle between the target and desired thrust vectors
	const float thrust_vector_angle = acosf(FMath::Clamp(thrust_vector_up.Dot(thrust_vector), -1.0f, 1.0f));

	// Normalize the thrust rotation vector
	const float thrust_vector_length = thrust_vec_cross.Length();
	if (FMath::IsNearlyZero(thrust_vector_length) || FMath::IsNearlyZero(thrust_vector_angle)) {
		thrust_vec_cross = thrust_vector_up;
	}
	else {
		thrust_vec_cross /= thrust_vector_length;
	}

	FQuat thrust_vec_quat;
	thrust_vec_quat = thrust_vec_quat.MakeFromRotationVector(FVector(thrust_vec_cross) * thrust_vector_angle);
	FQuat yaw_quat;
	yaw_quat = yaw_quat.MakeFromRotationVector(FVector{ 0.0f, 0.0f, 1.0f } * heading_angle);
	return thrust_vec_quat * yaw_quat;
}

FQuat URocketControllerComponent::attitude_from_thrust_vector_no_yaw(FVector3f thrust_vector)
{
	const FVector3f thrust_vector_up{ 0.0f, 0.0f, 1.0f };

	if (FMath::IsNearlyZero(thrust_vector.SquaredLength())) {
		thrust_vector = thrust_vector_up;
	}
	else {
		thrust_vector.Normalize();
	}

	// the cross product of the desired and target thrust vector defines the rotation vector
	FVector3f thrust_vec_cross = thrust_vector_up.Cross(thrust_vector);

	// the dot product is used to calculate the angle between the target and desired thrust vectors
	const float thrust_vector_angle = acosf(FMath::Clamp(thrust_vector_up.Dot(thrust_vector), -1.0f, 1.0f));

	// Normalize the thrust rotation vector
	const float thrust_vector_length = thrust_vec_cross.Length();
	if (FMath::IsNearlyZero(thrust_vector_length) || FMath::IsNearlyZero(thrust_vector_angle)) {
		thrust_vec_cross = thrust_vector_up;
	}
	else {
		thrust_vec_cross /= thrust_vector_length;
	}

	FQuat thrust_vec_quat;
	thrust_vec_quat = thrust_vec_quat.MakeFromRotationVector(FVector(thrust_vec_cross) * thrust_vector_angle);
	
	// calculate the angle error in x and y.
	float thrust_vector_diff_angle;
	FQuat thrust_vec_correction_quat;
	FVector attitude_error;
	float returned_thrust_vector_angle;
	thrust_vector_rotation_angles(thrust_vec_quat, _attitude_target, thrust_vec_correction_quat, attitude_error, returned_thrust_vector_angle, thrust_vector_diff_angle);

	return _attitude_target * thrust_vec_correction_quat;
}

void URocketControllerComponent::UpdateZParams()
{
	pos_control->set_max_speed_accel_z(max_speed_down, max_speed_up, max_accel_z);
	P_pos_z->set_limits(max_speed_down, max_speed_up, max_accel_z);
	P_pos_z->set_error_limits(max_speed_down, max_speed_up);
}

void URocketControllerComponent::setXYMaxSpeedAccel(float MaxSpeed, float MaxAccel)
{
	P_pos_xy->set_limits(MaxSpeed, MaxAccel, 0.0f);
	pos_control->set_max_speed_accel_xy(MaxSpeed, MaxAccel);
}

void URocketControllerComponent::SetBoneCollision(FName BoneName, bool state)
{
	ECollisionEnabled::Type collState = ECollisionEnabled::NoCollision;
	if (state) collState = ECollisionEnabled::QueryAndPhysics;
	TArray<FPhysicsShapeHandle> shapes;

	// Get the physics scene
	FPhysScene_Chaos* PhysScene = GetWorld()->GetPhysicsScene();
	if (PhysScene)
	{
		// Lock the Chaos scene using FScopedSceneLock_Chaos
		FScopedSceneLock_Chaos ScopedLock = FScopedSceneLock_Chaos(PhysScene, EPhysicsInterfaceScopedLockType::Write);
		
		FBodyInstance* bi = MainMeshComponent->GetBodyInstance(BoneName);
		if (bi)
		{
			bi->GetAllShapes_AssumesLocked(shapes);
			if (!shapes.IsEmpty())
			{
				for (int i = 0; i < shapes.Num(); i++)
				{
					bi->SetShapeCollisionEnabled(i, collState);
				}

			}
		}
	}

	
}


void URocketControllerComponent::SetTrajectoryEnabled(bool bEnable)
{
	bTrajectoryEnabled = bEnable;

	if (TrajectoryMesh)
	{
		TrajectoryMesh->SetVisibility(bEnable);

		if (bEnable)
		{
			// Create procedrual mesh here once (and later just update) with following parameters.
			// In order to update the mesh the number of verts can't change, so pre-allocate everything.

			PredictedPath.Init(FTrajectoryPoint(), TrajectoryMaxIterations);
			
			MeshVertices.Init(FVector(), 2 * TrajectoryMaxIterations);
			MeshNormals.Init(FVector(), 2 * TrajectoryMaxIterations);   // Stores DIRECTION (Tangent)
			MeshUVs.Init(FVector2D(), 2 * TrajectoryMaxIterations);      // Stores Side & Tiling
			MeshTriangles.Reset();

			// Create Triangles (Standard strip) indices which stay same throughout
			for (int32 i = 1; i < TrajectoryMaxIterations; i++)
			{
				int32 v = (i - 1) * 2;
				MeshTriangles.Append({ v, v + 2, v + 1, v + 1, v + 2, v + 3 });
			}

			TrajectoryMesh->CreateMeshSection_LinearColor(0, MeshVertices, MeshTriangles, MeshNormals, MeshUVs, TArray<FVector2D>(), TArray<FVector2D>(), TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false, false);

			// Ensure the bounding box is huge so it doesn't get culled when the root is off-screen
			TrajectoryMesh->SetBoundsScale(10.0f);

			if (TrajectoryMaterial)
			{
				TrajectoryMesh->SetMaterial(0, TrajectoryMaterial);
			}
		}
		else
		{
			TrajectoryMesh->ClearAllMeshSections();
			PredictedPath.Empty();

		}
	}
}

void URocketControllerComponent::UpdateTrajectoryPrediction()
{
	// prevent stacking multiple async tasks if one is already running
	if (!bTrajectoryEnabled || bIsPredictionRunning) return;

	if (!Rocket || !MainMeshComponent) return;

	if (PredictedPath.Num() < TrajectoryMaxIterations) return;

	// Snapshot current state (Must copy values, cannot access Actor from background thread!)
	FVector StartPos = CurrentPosition;
	FVector StartVel = Velocity;
	FVector UpVector = MainMeshTransform.GetRotation().GetUpVector();
	float StartMass = TotalMass;
	float GravityZ = Gravity.Z;

	bIsPredictionRunning = true;

	// Launch background task
	Async(EAsyncExecution::ThreadPool, [this, StartPos, StartVel, UpVector, StartMass, GravityZ]()
		{
			RunAsyncPrediction(StartPos, StartVel, UpVector, StartMass, GravityZ);
		});
}


void URocketControllerComponent::RunAsyncPrediction(FVector StartPos, FVector StartVel, FVector UpVector, float StartMass, float GravityZ)
{

	// Simulation State
	FVector CurrentPos = StartPos;
	FVector CurrentVel = StartVel;
	float CurrentTime = 0.0f;
	float MaxPredictionTime = 600.0f; // Predict 10 minutes into future

	// --- Orientation Assumption (Guidance Logic) ---
	FVector CurrentUpVector = UpVector;
	// mainmesh up vector is only directly used in GuidanceMode 0

	// Loop config
	float dt = 2.0f; // Initial step size

	TArray<FTrajectoryPoint> LocalPath;
	LocalPath.Init(FTrajectoryPoint(), TrajectoryMaxIterations);
	
	FTrajectoryPoint NewPoint;
	// set rocket pos has first point
	NewPoint.Location = StartPos;
	NewPoint.VelocityMag = CurrentVel.Length();
	NewPoint.Time = CurrentTime;
	LocalPath[0] = NewPoint;

	int i = 1; // start at index 1 as 0 is rocket pos
	while (i < TrajectoryMaxIterations)
	{
		if (CurrentTime >= MaxPredictionTime) break;

		float _Altitude = CurrentPos.Z;

		if (i > 1)
		{
			if (_Altitude == 0) // Hit ground exactly
				break;
			else if (_Altitude < 0)
			{
				// put last point on z=0 before breaking
				
				FVector A = LocalPath[LastTrajectoryPointIndex-1].Location;
				FVector B = LocalPath[LastTrajectoryPointIndex].Location;
				if (B.Z < 0 && A.Z > 0) // else something is wrong
				{
					LocalPath[LastTrajectoryPointIndex].Location = A + (B - A) * -A.Z / (B.Z - A.Z);

				}

				break;
			}
		}

		// --- Adaptive Step Size ---
		// If high up and slow, take big steps. If fast or low, take small steps.
		float _DensityAir = 1.2f * FMath::Exp(-_Altitude / 10000.0f); //
		float VelMag = CurrentVel.Length();

		if (_Altitude > 30000.0f && _DensityAir < 0.05f) dt = 2.0f; // Space/Upper Atmo
		else if (VelMag > 200.0f || _Altitude < 5000.0f) dt = 0.1f; // High dynamic pressure or landing
		else dt = 0.3f;                                             // Cruising

		
		if (isBooster)
		{
			if (GuidanceMode == 1)
			{
				// BALLISTIC/ASCENT: UpVector aligns with Velocity (Nose into wind)
				CurrentUpVector = CurrentVel.GetSafeNormal();
			}

		}
		else // is ship
		{
			if (GuidanceMode == 1)
			{
				// BELLY FLOP: UpVector is perpendicular to Velocity
				// Cross velocity with Y-axis to get a stable "Normal" vector
				FVector RightVec = FVector(0, 1, 0);
				CurrentUpVector = FVector::CrossProduct(CurrentVel, RightVec).GetSafeNormal();

				// Fallback if velocity is perfectly vertical
				if (CurrentUpVector.IsNearlyZero()) CurrentUpVector = FVector(1, 0, 0);
			}
		}

		// --- Drag Calculation (Ported from AeroDynamicsComponent) ---
		FVector DragForce = FVector::ZeroVector;

		if (_DensityAir > 0.0001f)
		{
			// Project velocity onto UpVector (Axial component)
			FVector velocity1 = CurrentVel.ProjectOnTo(CurrentUpVector);
			float sqLen1 = velocity1.SquaredLength();
			velocity1.Normalize();
			// 127.0f is the constant from AeroComponent for nose drag
			FVector drag1 = 127.0f * sqLen1 * -velocity1;

			// Project velocity onto Plane (Radial/Belly component)
			FVector velocity2 = FVector::VectorPlaneProject(CurrentVel, CurrentUpVector);
			float sqLen2 = velocity2.SquaredLength();
			velocity2.Normalize();
			FVector drag2 = Rocket->AeroDynamicsComponent->AeroSurface * sqLen2 * -velocity2;

			FVector dragSum = drag1 + drag2;

			// Apply coefficients
			DragForce = _DensityAir * Rocket->AeroDynamicsComponent->CoefficientOfDrag * Prediction_DragFactor * 0.5f * dragSum;

			for (FAeroPart& part : Rocket->AeroDynamicsComponent->AeroParts)
			{
				float VelocitySquare = CurrentVel.SquaredLength() * FMath::Clamp(333 * 333 / CurrentVel.SquaredLength(), 0.1f, 1.0f); // reduce drag at hyper sonic speeds (in a very simple way) - what would be a realistic minimum here?

				const FTransform t = UAsyncTickFunctions::ATP_GetSocketTransform(MainMeshComponent, part.BoneName);
				const FVector worldPosition = t.GetLocation();
				const FQuat worldRotation = t.GetRotation();

				FVector worldUpVector = worldRotation.GetUpVector();
				float CosAngleOfAttack = FMath::Abs(worldUpVector.Dot(CurrentVel.GetSafeNormal()));
				float PartCosAoA = 1.0f;
				if (part.PartType == "gridfin")
				{
					part.velocity = Velocity;
					// CosAoA goes from 1 to 0 while the effective cos of part in case of
					// grid fin goes from 0.05->1->0.05
					PartCosAoA = FMath::Clamp(FMath::Sin(2 * FMath::Acos(CosAngleOfAttack)), 0.05f, 1);

					// add contribution of each grid diagonal and make a combined vector with normal velocity based drag
					drag1 = worldRotation.RotateVector(FVector(1, 1, 0));
					drag2 = worldRotation.RotateVector(FVector(1, -1, 0));
					dragSum = -(drag1.Dot(Velocity) * drag1 + drag2.Dot(Velocity) * drag2 + CosAngleOfAttack * Velocity);
					dragSum.Normalize();

					part.DragForce = DensityAir * part.CoefficientOfDrag * 0.25f * part.SurfaceArea * VelocitySquare * dragSum * PartCosAoA;
					// 0.25 here because we have 0.5 anywany and then we use only half of the flat laying grid fin area effectively when it's 45° angled at max
				}
				else if (part.PartType == "flap")
				{
					FVector direction = worldRotation.RotateVector(FVector(0, 1, 0)); // have force always pointing in flap normal direction for simplicity
					direction.Normalize();
					float dot = direction.Dot(Velocity);
					PartCosAoA = FMath::Clamp(FMath::Abs(dot), 0.05f, 1);

					part.DragForce = DensityAir * part.CoefficientOfDrag * 0.5f * part.SurfaceArea * VelocitySquare * direction * PartCosAoA * -FMath::Sign(dot);
				}

				// calculate total resulting force for external use
				DragForce += part.DragForce;
			}
			
		}

		if (GameSettings->bAntiGravity)
		{
			// sharp transit to max force when going near 3000 m/s
			double MappedValue = UKismetMathLibrary::MapRangeClamped(
				VelMag,
				0.0, 3000.0, // InRange A & B
				0.0, 0.975      // OutRange A & B
			);

			double PoweredValue = UKismetMathLibrary::MultiplyMultiply_FloatFloat(MappedValue, 2.0f);

			// up force with mass * g, note: ALL IN SI UNITS HERE!!!
			double FinalZForce = PoweredValue * StartMass * 9.81f;

			DragForce += FVector(0.0, 0.0, FinalZForce);
		}

		// --- Integration (Euler) ---
		// F = ma -> a = F/m
		FVector _Acceleration = FVector(0, 0, GravityZ); // Gravity
		_Acceleration += DragForce / StartMass;          // Drag

		// Update State
		CurrentVel += _Acceleration * dt;
		CurrentPos += CurrentVel * dt;
		CurrentTime += dt;

		// Store Point
		NewPoint.Location = CurrentPos;
		NewPoint.VelocityMag = CurrentVel.Length();
		NewPoint.Time = CurrentTime;
		LocalPath[i] = NewPoint;

		LastTrajectoryPointIndex = i;
		i++;
	}

	// fill rest of pre-allocated array with same point as last one
	// (to be able to just update the procedural mesh)
	while (i < TrajectoryMaxIterations)
	{
		LocalPath[i] = LocalPath[LastTrajectoryPointIndex];

		i++;
	}

	// --- Return to Game Thread ---
	// We use a weak pointer to 'this' in case the actor is destroyed while calculation runs
	TWeakObjectPtr<URocketControllerComponent> WeakThis(this);

	AsyncTask(ENamedThreads::GameThread, [WeakThis, LocalPath]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->PredictedPath = LocalPath;
				WeakThis->bIsPredictionRunning = false; // Unlock for next update

				WeakThis->UpdateTrajectoryMesh();
			}
		});
}

void URocketControllerComponent::UpdateTrajectoryMesh()
{
	if (!TrajectoryMesh) return;

	// we are just going to create a triangle strip here and adjust the visuals and exact vertex positions later in the shader
	for (int32 i = 0; i < TrajectoryMaxIterations; i++)
	{
		FVector TargetPos = PredictedPath[i].Location*100; // Path is in m
		
		// Indices for the two vertices at this path point
		int32 LeftIdx = 2 * i;
		int32 RightIdx = 2 * i + 1;

		MeshVertices[LeftIdx] = TargetPos;
		MeshVertices[RightIdx] = TargetPos;

		if (i <= LastTrajectoryPointIndex)
		{
			FVector Tangent;
			if (i < LastTrajectoryPointIndex) 
				Tangent = (PredictedPath[i + 1].Location - PredictedPath[i].Location).GetSafeNormal();
			else Tangent = (PredictedPath[i].Location - PredictedPath[i - 1].Location).GetSafeNormal(); // last tangent needs to use previous point


			// Store the TANGENT
			// We will use this in the material to calculate the perpendicular vector to the camera
			MeshNormals[LeftIdx] = Tangent;
			MeshNormals[RightIdx] = Tangent;

			// Use UV.X to store the "Side" (-1 for left, 1 for right)
			// Use UV.Y for texture tiling along the path
			float Tiling = (float)i / FMath::Max((float)(LastTrajectoryPointIndex), 1.0f);
		
			MeshUVs[LeftIdx] = (FVector2D(-1.0f, Tiling));
			MeshUVs[RightIdx] = (FVector2D(1.0f, Tiling));

		}
	}

	// Create the Mesh Section
	// We use CreateMeshSection (not Update) because the point count changes. 
	TrajectoryMesh->UpdateMeshSection_LinearColor(0, MeshVertices, MeshNormals, MeshUVs, TArray<FLinearColor>(), TArray<FProcMeshTangent>());
}