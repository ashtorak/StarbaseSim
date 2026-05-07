// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include "RocketEngine.h"
#include "RocketActor.h"
#include "GameStateBocaBase.h"

#include "RocketControllerComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRocketController, All, All);

class UNiagaraSystem;
class UNiagaraComponent;
class AGameStateBocaBase;
class UProceduralMeshComponent;
class ULyraSettingsLocal;

// Simple struct to store prediction points
USTRUCT(BlueprintType)
struct FTrajectoryPoint
{
	GENERATED_BODY()

	FVector Location;
	float VelocityMag;
	float Time;
};

USTRUCT(BlueprintType)
struct FGridFin
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
		FName BoneName;

	UPROPERTY(BlueprintReadWrite)
		FConstraintInstanceAccessor constraint;
};

UCLASS(BlueprintType, ClassGroup = (Rocket))
class URCSThruster : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite)
		FName bone;

	UPROPERTY(BlueprintReadWrite)
		bool isExisting;

	UPROPERTY(BlueprintReadWrite)
		bool isFiring;

	UPROPERTY(BlueprintReadWrite)
		bool xplus;

	UPROPERTY(BlueprintReadWrite)
		bool xminus;

	UPROPERTY(BlueprintReadWrite)
		bool yplus;

	UPROPERTY(BlueprintReadWrite)
		bool yminus;

	UPROPERTY(BlueprintReadWrite)
		bool zplus;

	UPROPERTY(BlueprintReadWrite)
		bool zminus;

	UPROPERTY(BlueprintReadWrite)
		UNiagaraComponent* vent;
};



UCLASS(Blueprintable, ClassGroup=(Rocket), meta=(BlueprintSpawnableComponent) )
class STARBASESIMLIBRARY_API URocketControllerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	URocketControllerComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void AsyncPhysicsTickComponent(float DeltaTime, float SimTime) override;

	virtual void SlowTickComponent(float DeltaTime);

	void Update(float DeltaTime, float SimTime);

	TObjectPtr<class ARocketActor> Rocket;

	// recalculates engine mass when engine has been removed or so
	bool partsChanged = true;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class USkeletalMeshComponent> MainMeshComponent;

	// This is used for attitude controllers
	UPROPERTY(BlueprintReadWrite)
		FTransform  MainMeshTransform;

	UPROPERTY(BlueprintReadWrite)
		bool IsOnAttitudeControl;

	UPROPERTY(BlueprintReadWrite, replicated)
		bool IsOnPosControlXY;

	UPROPERTY(BlueprintReadWrite, replicated)
		bool IsOnPosControlZ;

	// to not have any controller and gimbal update
	UPROPERTY(BlueprintReadWrite, replicated)
		bool isFullySwitchedOff = false;

	// set this via blueprint at begin
	UPROPERTY(BlueprintReadWrite)
		bool isBooster = false;

	// in kg/m³
	UPROPERTY(BlueprintReadWrite)
		float DensityAir;

	// in m/s
	UPROPERTY(BlueprintReadWrite)
		FVector Velocity;

	// in m/s
	UPROPERTY(BlueprintReadWrite)
		float VelocityMagnitude;

	FVector VelocityPrevious;

	// in m/s²
	UPROPERTY(BlueprintReadWrite)
		FVector Acceleration;

	UPROPERTY(Category = "Physics", EditAnywhere, BlueprintReadWrite)
		float AccelerationSmoothingFactor = 0.33f;

	// in m/s²
	//UPROPERTY(BlueprintReadWrite)
		FVector Gravity = FVector(0,0,-9.81);

	// in m/s² - body acceleration = acceleration - gravity in body frame
	UPROPERTY(Category = "Physics", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "BodyAcceleration - gravity in body frame", ForceUnits = "m/s²"))
		FVector BodyAcceleration;

	// in m/s² - body acceleration = acceleration - gravity in body frame
	UPROPERTY(BlueprintReadWrite)
		float BodyAccelerationMagnitude;

	// simple estimate that is calculated in game thread, do not use for physics calcs
	//UPROPERTY(BlueprintReadWrite)
	UPROPERTY(BlueprintReadOnly)
		FVector OverallCenterOfMass;

	//UPROPERTY(BlueprintReadWrite)
		FVector posCoM;

	//UPROPERTY(BlueprintReadWrite)
		FVector posEngines;

		// simple estimate that is calculated in game thread, do not use for physics calcs
		UFUNCTION(BlueprintCallable)
		FVector GetOverallCenterOfMass();

	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class UPIDComponent> PIDThrottle;

	// in m above sea level
	UPROPERTY(BlueprintReadWrite)
		float Altitude;

	// total mass in kg is calculated in slow update
	UPROPERTY(BlueprintReadWrite)
		float TotalMass =10.0;

	// in kg
	UPROPERTY(BlueprintReadWrite)
		float TotalMassEngines = 0.0f;

	// in kg, needs to be set via blueprint
	UPROPERTY(BlueprintReadWrite)
	float TotalMassAeroParts = 0.0f;

	// in kg, set via blueprint according to if it is attached or not
	UPROPERTY(BlueprintReadWrite)
		float TotalMassHotStageRing = 0.0f;

	// in kg, main body mass is set to match this
	UPROPERTY(BlueprintReadWrite)
		float DryMass = 230000.0f;

	// total prop mass in kg
	UPROPERTY(BlueprintReadWrite, replicated)
		float PropMassTotal = 0.0f;

	// total prop mass in kg
	UPROPERTY(BlueprintReadWrite, replicated)
		float PropMassLiquid = 0.0f;

	// total prop mass in kg
	UPROPERTY(BlueprintReadWrite, replicated)
		float PropMassGas = 0.0f;

	// prop capacity in kg
	UPROPERTY(BlueprintReadWrite)
		float PropCapacity;

	// this just for moving gimbals outwards
	UPROPERTY(BlueprintReadWrite, replicated)
		bool isShipHotStaging;

	// this just for moving outer gimbals outwards and giving inner more room
	UPROPERTY(BlueprintReadWrite, replicated)
		bool isBoosterLanding;

	UPROPERTY(BlueprintReadWrite)
		float TotalThrust;

	UFUNCTION(BlueprintCallable)
		void SetBoneCollision(FName BoneName, bool state);

	UPROPERTY(BlueprintReadWrite, replicated)
		bool sitsOnOLM;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
		void ReleaseOLMClamps();

	UPROPERTY(BlueprintReadWrite)
		bool isStoppingEngines = false;


	ULyraSettingsLocal* GameSettings;

	///////////////////////////////////////////////////////////////////////////////////////////////
	// prediction stuff
	///////////////////////////////////////////////////////////////////////////////////////////////
	
	// The calculated path in m (safe to read from Blueprint)
	UPROPERTY(BlueprintReadOnly, Category = "Trajectory")
	TArray<FTrajectoryPoint> PredictedPath;

	int TrajectoryMaxIterations = 2000; // to change during runtime disable/enable trajectories
	int LastTrajectoryPointIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trajectory")
	float TrajectoryUpdateInterval = 0.1f;

	// Call this from Tick/SlowTick to trigger an update
	void UpdateTrajectoryPrediction();

	float LastTrajectoryUpdateTime = 0.0f;

	// Parameters for aerodynamics (Mirroring AeroDynamicsComponent)
	// You might want to get these dynamically or set them to match your AeroComponent values
	
	float Sim_Cd = 1.0f;             // Coefficient of Drag

	UPROPERTY(BlueprintReadWrite)
	float Prediction_DragFactor = 1.0f;     // Tuning factor

	// Thread safety flag
	std::atomic<bool> bIsPredictionRunning = false;

	// The heavy lifting function (runs on background thread)
	void RunAsyncPrediction(FVector StartPosMeters, FVector StartVelMeters, FVector UpVector, float StartMass, float GravityZ);


		// The component that actually renders the line
		UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trajectory")
		TObjectPtr<UProceduralMeshComponent> TrajectoryMesh;

		// The material for the line (set this in the Blueprint defaults)
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trajectory")
		TObjectPtr<UMaterialInterface> TrajectoryMaterial;

		// Local-space data buffers to avoid reallocating memory every frame
		TArray<FVector> MeshVertices;
		TArray<int> MeshTriangles;
		TArray<FVector> MeshNormals;
		TArray<FVector2D> MeshUVs;

		void UpdateTrajectoryMesh();

		// Toggles the trajectory prediction on/off
		UFUNCTION(BlueprintCallable, Category = "Trajectory")
		void SetTrajectoryEnabled(bool bEnable);

	UPROPERTY(BlueprintReadOnly, Category = "Trajectory")
	bool bTrajectoryEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trajectory")
	int GuidanceMode = 0;




	///////////////////////////////////////////////////////////////////////////////////////////////
	// engine stuff
	///////////////////////////////////////////////////////////////////////////////////////////////

	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class URocketEngines> Engines;

	UPROPERTY(BlueprintReadWrite)
		int NumberOfEnginesRunning;

	UPROPERTY(BlueprintReadWrite, Replicated)
		int NumberOfEnginesRequested = 3;

	int64 EnginesThatAreRunningBitmask;

	bool AreEnginesReleased = false;

	// adjust pid parameters based on active gimbal engines and prop mass
	float CountActiveGimbalEngines;
	float K_factor;

	UPROPERTY(BlueprintReadWrite)
		float NumberOfGimbalEngines = 13;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
		void StopEnginesCommandFromCPP();

	// thrust to weight ratio
	UPROPERTY(BlueprintReadOnly)
		float TWR = 0;

	// thrust to weight ratio with current mass and selected engines at full throttle
	UPROPERTY(BlueprintReadOnly)
		float TWR_max = 0;

	// for TWR calc
	UPROPERTY(BlueprintReadWrite)
		float MassOfShipOnTop = 0;

	// nominal thrust at full throttle in N
	UPROPERTY(BlueprintReadWrite, replicated)
		float EngineThrust = 2300000;

	// nominal mass flow through one engine in kg/s
	UPROPERTY(BlueprintReadWrite, replicated)
		float EngineMassFlow = 666;

	// total mass flow through all running engines in kg/s
	UPROPERTY(BlueprintReadWrite)
		float EngineMassFlowTotal;

	void CalculateThrottle(float DeltaTime);

	UPROPERTY(BlueprintReadWrite, replicated)
		float ThrottleRequest;

	///////////////////////////////////////////////////////////////////////////////////////////////
	// gimbal/attitude stuff
	///////////////////////////////////////////////////////////////////////////////////////////////

	// this is for sliders and controllers
	UPROPERTY(BlueprintReadWrite, replicated)
		float GimbalAngleMax = 15.0f;

	// hard limit (default, for ship)
	float hardGimbalMax = 15.0f;
	// hard limit for booster
	float hardGimbalMaxBooster = 12.0f;
	// hard limit for inner three during landing
	float hardGimbalMaxBoosterLanding = 15.0f;
	float _GimbalAngleMax;

	UPROPERTY(BlueprintReadWrite, replicated)
		bool useEngineGimbals = true;

	UPROPERTY(BlueprintReadWrite)
		float GimbalRate = 99.0f;

	void CalculateAngularErrorWithVelocity();
	
	// This represents a quaternion rotation in NED frame to the body
	FQuat AttitudeBody;

	UPROPERTY(BlueprintReadOnly)
		FVector AngularVelocityBody_Degree = FVector(0,0,0);

	// to add rotation around long axis when using pos control
	UPROPERTY(BlueprintReadWrite)
		float YawTargetRate = 0;

	// final position request for orientation target of gimbals in x,y,z coming from controllers
	UPROPERTY(BlueprintReadWrite)
		FVector GimbalPos;

	UPROPERTY(BlueprintReadWrite)
		float GimbalDirectionAngVel = -1;

	// The gimbal controller tries to keep the value below these angular velocities (actually below somewhat the double of it due to how it is set up currently)
	// This has to be >0 to have any effect on GimbalPos!
	UPROPERTY(BlueprintReadWrite, Replicated)
		FVector GimbalAngVelMaxSetpoint = FVector(11,11,4);

	// When the angle between target and body vector is larger than this, don't roll (Default = 15°)
	UPROPERTY(BlueprintReadWrite)
	float ThrustVectorErrorRollLimit = FMath::DegreesToRadians(15);

	UPROPERTY(BlueprintReadWrite)
	FVector GimbalErrorExtraKp = FVector(1, 1, 1);

	// don't control x and y when it's rotating very fast around it's up axis
	UPROPERTY(BlueprintReadWrite)
	float AngularVelocityZLimit_Degree = 55;

	// prevent large corrections when suddenly there is a larger Z angle due to overall rotation of rocket
	UPROPERTY(BlueprintReadWrite)
		float AngularErrorZLimit_Degree = 33;

	// due to the scuffedness of the current implentation we get the angular velocity close to the setpoint just by using this additional factor
	UPROPERTY(BlueprintReadWrite)
		float scaleDivider = 2;

	// This represents the angular velocity of the target (setpoint) attitude used in
	// the attitude controller as an angular velocity vector, in radians per second in
	// the target attitude frame.
	UPROPERTY(BlueprintReadWrite)
	FVector            _ang_vel_target;

	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class UPIDComponent> PIDGimbalXcpp;

	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class UPIDComponent> PIDGimbalYcpp;

	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class UPIDComponent> PIDGimbalZcpp;

	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class UPIDComponent> PIDGimbalAngVelX;

	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class UPIDComponent> PIDGimbalAngVelY;

	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class UPIDComponent> PIDGimbalAngVelZ;

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Drag
	///////////////////////////////////////////////////////////////////////////////////////////////

	UPROPERTY(BlueprintReadWrite, Replicated)
		FVector AngVelDragMaxSetpoint = FVector(22, 22, 22);

	// final position request for grid fins or flaps in x,y,z coming from controllers
	UPROPERTY(BlueprintReadWrite)
		FVector DragPos;

	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class UPIDComponent> PIDDragX;

	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class UPIDComponent> PIDDragY;

	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class UPIDComponent> PIDDragZ;

	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class UPIDComponent> PIDDragAngVelX;

	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class UPIDComponent> PIDDragAngVelY;

	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class UPIDComponent> PIDDragAngVelZ;

	// Grid Fins

	UPROPERTY(BlueprintReadWrite, replicated)
		bool useGridFins = false;

	UPROPERTY(BlueprintReadWrite)
		float GridFinMax = 45;

	UPROPERTY(BlueprintReadWrite)
		float GridFinRate = 99.0f;

	UPROPERTY(BlueprintReadWrite)
		FConstraintInstanceAccessor GridFinLeftQD;

	UPROPERTY(BlueprintReadWrite)
		FConstraintInstanceAccessor GridFinLeft;

	UPROPERTY(BlueprintReadWrite)
		FConstraintInstanceAccessor GridFinRightQD;

	UPROPERTY(BlueprintReadWrite)
		FConstraintInstanceAccessor GridFinRight;

	// Flaps

	UPROPERTY(BlueprintReadWrite, replicated)
		bool useFlaps = false;

	UPROPERTY(BlueprintReadWrite, replicated)
		bool foldFlaps = false;

	UPROPERTY(BlueprintReadWrite)
		float FlapAftMaxTarget = 80.0f;

	UPROPERTY(BlueprintReadWrite)
		float FlapFrontMaxTarget = 70.0f;

	UPROPERTY(BlueprintReadWrite)
		float FlapsRate = 99.0f;

	UPROPERTY(BlueprintReadWrite)
		FConstraintInstanceAccessor FlapAftLeft;

	UPROPERTY(BlueprintReadWrite)
		FConstraintInstanceAccessor FlapAftRight;

	UPROPERTY(BlueprintReadWrite)
		FConstraintInstanceAccessor FlapFrontLeft;

	UPROPERTY(BlueprintReadWrite)
		FConstraintInstanceAccessor FlapFrontRight;

	float flapFolder = -1.0f;


	///////////////////////////////////////////////////////////////////////////////////////////////
	// RCS
	///////////////////////////////////////////////////////////////////////////////////////////////


	UPROPERTY(BlueprintReadWrite)
		float ShipCowBellReductionFactor =0.45f;

	UPROPERTY(BlueprintReadWrite)
		FTransform tBoneLocal;

	UPROPERTY(BlueprintReadWrite)
		FVector RCSForceVectorCalc;

	UPROPERTY(BlueprintReadWrite)
		FVector RCSForceVectorOverride;

	UPROPERTY(BlueprintReadWrite)
		FVector RCSForcePos;

	UPROPERTY(BlueprintReadWrite, replicated)
		FVector AngVelRCSMaxSetpoint = FVector(22, 22, 22);

	// final position request for RCS in x,y,z coming from controllers
	UPROPERTY(BlueprintReadWrite)
		FVector RCSPos;

	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class UNiagaraSystem> VentingEffect;

	UPROPERTY(BlueprintReadWrite, replicated)
		bool useRCS = false;

	UPROPERTY(BlueprintReadWrite)
		float RCSThreshold = 0.05f;

	UPROPERTY(BlueprintReadWrite, replicated)
		float RCSThrust = 100000;

	UPROPERTY(BlueprintReadWrite)
		TArray<URCSThruster*> RCSThrusters;

	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class UPIDComponent> PIDRCSX;

	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class UPIDComponent> PIDRCSY;

	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class UPIDComponent> PIDRCSZ;

	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class UPIDComponent> PIDRCSAngVelX;

	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class UPIDComponent> PIDRCSAngVelY;

	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class UPIDComponent> PIDRCSAngVelZ;

	bool xplus, xminus, yplus, yminus, zplus, zminus;

	float RCS_X_lastTime, RCS_Y_lastTime, RCS_Z_lastTime;

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Position Controller
	///////////////////////////////////////////////////////////////////////////////////////////////

	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class UPosControl> pos_control;

	// in m above sea level
	UPROPERTY(BlueprintReadWrite)
		float AltitudeTarget;

	// north, east, up in m
	UPROPERTY(BlueprintReadWrite, replicated)
		FVector PositionTarget;

	// north, east, up in m
	UPROPERTY(BlueprintReadWrite)
		FVector CurrentPosition;

	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class UP_1D >       P_pos_z;           // Z axis position controller to convert altitude error to desired climb rate

	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class UPIDComponent> PID_vel_z;
	
	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class UPIDComponent> PID_accel_z;

	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class UP_2D >       P_pos_xy;           // Z axis position controller to convert altitude error to desired climb rate

	UPROPERTY(BlueprintReadWrite)
		TObjectPtr<class UPID_2D>		PID_vel_xy;

	UPROPERTY(BlueprintReadWrite)
		float max_speed_up = 222.0f;

	UPROPERTY(BlueprintReadWrite)
		float max_speed_down = -222.0f;

	UPROPERTY(BlueprintReadWrite)
		float max_accel_z = 22.0f;

	UPROPERTY(BlueprintReadWrite)
		float HoverThrottle;
	
	UPROPERTY(BlueprintReadWrite)
		bool useHoverThrottle = true;

	UFUNCTION(BlueprintCallable)
		void UpdateZParams();

	UFUNCTION(BlueprintCallable)
		void setXYMaxSpeedAccel(float MaxSpeed, float MaxAccel);

	// This represents a quaternion rotation in NED frame to the target (setpoint)
	// attitude used in the attitude controller.
	UPROPERTY(BlueprintReadWrite, replicated)
		FQuat _attitude_target;

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Spawn and Scripting
	///////////////////////////////////////////////////////////////////////////////////////////////

	UPROPERTY(BlueprintReadWrite)
		double SpawnPosX = 0;

	UPROPERTY(BlueprintReadWrite)
		double SpawnPosY = 7600;

	UPROPERTY(BlueprintReadWrite)
		double SpawnPosZ = 10000;

	UPROPERTY(BlueprintReadWrite)
		double SpawnRotX = 45;

	UPROPERTY(BlueprintReadWrite)
		double SpawnRotY = 0;

	UPROPERTY(BlueprintReadWrite)
		double SpawnRotZ = 0;

	UPROPERTY(BlueprintReadWrite)
		double SpawnVelocityX = 0;

	UPROPERTY(BlueprintReadWrite)
		double SpawnVelocityY = -565;

	UPROPERTY(BlueprintReadWrite)
		double SpawnVelocityZ = -565;

	// propellant on spawn in tons
	UPROPERTY(BlueprintReadWrite)
		float SpawnProp = 0;

	TObjectPtr<class AGameStateBocaBase> GSBoca;

	UPROPERTY(BlueprintReadWrite)
	TArray<FGameCommandData> ScriptCommands;

	int ScriptCommandsIterator = 0;

	UPROPERTY(BlueprintReadWrite, replicated)
	bool RunScriptArray = false;

	UPROPERTY(BlueprintReadWrite)
	float ScriptStartTime = 0;

	UPROPERTY(BlueprintReadWrite)
	int ScriptType = 0;

protected:

	// Update rate_target_ang_vel using attitude_error_rot_vec_rad
	FVector update_attitude_target_from_att_error(const FVector& attitude_error_rot_vec_rad);


	TObjectPtr<class USceneComponent> AttitudeTargetComponent;


	// This represents the Gimbal error in degree per second in the body frame, used in the angular
	// velocity controller.
	FVector            _attitude_error_body_Degree;

	// The angle between the target thrust vector and the current thrust vector.
	float               _thrust_angle;

	// The angle between the target thrust vector and the current thrust vector.
	float               _thrust_error_angle;

private: 
	const float slowTickSeconds = 0.5f;
	float slowTickAccumulator;

	FQuat heading_vec_correction_quat;

	// thrust_heading_rotation_angles - calculates two ordered rotations to move the attitude_body quaternion to the attitude_target quaternion.
	// The maximum error in the yaw axis is limited based on the angle yaw P value and acceleration.
	void thrust_heading_rotation_angles(FQuat& attitude_target, const FQuat& attitude_body, FVector& attitude_error, float& thrust_angle, float& thrust_error_angle);

	// thrust_vector_rotation_angles - calculates two ordered rotations to move the attitude_body quaternion to the attitude_target quaternion.
	// The first rotation corrects the thrust vector and the second rotation corrects the heading vector.
	void thrust_vector_rotation_angles(const FQuat& attitude_target, const FQuat& attitude_body, FQuat& thrust_vector_correction, FVector& attitude_error, float& thrust_angle, float& thrust_error_angle);

	// translates body frame acceleration limits to the euler axis
	void ang_vel_limit_Degree(FVector& euler, float ang_vel_roll_max, float ang_vel_pitch_max, float ang_vel_yaw_max) const;

	FQuat Attitude_from_thrust_vector_rate_heading(const FVector3f& thrust_vector, float heading_rate, bool slew_yaw, float DeltaTime);

	FQuat attitude_from_thrust_vector(FVector3f thrust_vector, float heading_angle);

	FQuat attitude_from_thrust_vector_no_yaw(FVector3f thrust_vector);

	// pidNumber is 0,1,2 for X,Y,Z
	void CalculateGimbalRequest(int pidNumber, TObjectPtr<class UPIDComponent> pidGimbal, TObjectPtr<class UPIDComponent> pidAngVel, float DeltaTime);

	// pidNumber is 0,1,2 for X,Y,Z
	void CalculateDragRequest(int pidNumber, TObjectPtr<class UPIDComponent> pidAng, TObjectPtr<class UPIDComponent> pidAngVel, float DeltaTime);

	// pidNumber is 0,1,2 for X,Y,Z
	void CalculateRCSRequest(int pidNumber, TObjectPtr<class UPIDComponent> pidAng, TObjectPtr<class UPIDComponent> pidAngVel, float DeltaTime);


	void SetGridFinTarget(FConstraintInstanceAccessor& GridFin, float newTarget);

	void SetFlapTarget(FConstraintInstanceAccessor& Flap, float newTarget, float maxTarget);

	float deltaRate;

	// used to reset firing state when useRCS is set to false, default true to disable effects on spawn
	bool RCSWasOn = true;

	float rot_z;
	float att_err_z;
	float ZexpAng;
	float ZexpAngVel;

	
};
