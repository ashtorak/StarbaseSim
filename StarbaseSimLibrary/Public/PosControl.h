// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "P_1D.h"
#include "P_2D.h"
#include "PID_2D.h"
#include "PIDComponent.h"
#include "RocketControllerComponent.h"
#include "PosControl.generated.h"

/**
 * based on ArduCopter class
 */
UCLASS(ClassGroup = (Rocket))
class STARBASESIMLIBRARY_API UPosControl : public UObject
{
	GENERATED_BODY()

public:

	UPosControl();

	TObjectPtr<class URocketControllerComponent> RocketController;

	void set_max_speed_accel_xy(float speed, float accel);

	///
	/// Vertical position controller
	///

	/// set_max_speed_accel_z - set the maximum vertical speed in cm/s and acceleration in cm/s/s
	///     speed_down can be positive or negative but will always be interpreted as a descent speed
	///     This can be done at any time as changes in these parameters are handled smoothly
	///     by the kinematic shaping.
	void set_max_speed_accel_z(float speed_down, float speed_up, float accel);

	/// update_z_controller - runs the vertical position controller correcting position, velocity and acceleration errors.
///     Position and velocity errors are converted to velocity and acceleration targets using PID objects
///     Desired velocity and accelerations are added to these corrections as they are calculated
///     Kinematically consistent target position and desired velocity and accelerations should be provided before calling this function
	float update_z_controller(float DeltaTime);

	/// update_xy_controller - runs the horizontal position controller correcting position, velocity and acceleration errors.
///     Position and velocity errors are converted to velocity and acceleration targets using PID objects
///     Desired velocity and accelerations are added to these corrections as they are calculated
///     Kinematically consistent target position and desired velocity and accelerations should be provided before calling this function
	void update_xy_controller(float DeltaTime);

	///
/// 3D position shaper
///

/// input_pos_xyz - calculate a jerk limited path from the current position, velocity and acceleration to an input position.
///     The function takes the current position, velocity, and acceleration and calculates the required jerk limited adjustment to the acceleration for the next time dt.
///     The kinematic path is constrained by the maximum acceleration and jerk set using the function set_max_speed_accel_xy.
	void input_pos_xyz(const FVector& pos, float pos_offset_z, float pos_offset_z_buffer, float DeltaTime);

	// update_pos_vel_accel - single axis projection of position and velocity forward in time based on a time step of dt and acceleration of accel.
// the position and velocity is not moved in the direction of limit if limit is not set to zero.
// limit - specifies if the system is unable to continue to accelerate.
// pos_error and vel_error - specifies the direction of the velocity error used in limit handling.
	void update_pos_vel_accel(double& pos, float& vel, float accel, float dt, float limit, float pos_error, float vel_error);

	// update_vel_accel - single axis projection of velocity, vel, forwards in time based on a time step of dt and acceleration of accel.
// the velocity is not moved in the direction of limit if limit is not set to zero.
// limit - specifies if the system is unable to continue to accelerate.
// vel_error - specifies the direction of the velocity error used in limit handling.
	void update_vel_accel(float& vel, float accel, float dt, float limit, float vel_error);


	/* shape_pos_vel_accel calculate a jerk limited path from the current position, velocity and acceleration to an input position and velocity.
 The function takes the current position, velocity, and acceleration and calculates the required jerk limited adjustment to the acceleration for the next time dt.
 The kinematic path is constrained by :
	minimum velocity - vel_min (must be negative),
	maximum velocity - vel_max (must be positive),
	minimum acceleration - accel_min (must be negative),
	maximum acceleration - accel_max (must be positive),
	maximum jerk - jerk_max (must be positive).
 The function alters the variable accel to follow a jerk limited kinematic path to pos_input, vel_input and accel_input.
 The correction velocity is limited to vel_max to vel_min. If limit_total is true the target velocity is limited to vel_max to vel_min.
 The correction acceleration is limited from accel_min to accel_max. If limit_total is true the target acceleration is limited from accel_min to accel_max.
*/
	void shape_pos_vel_accel(const double pos_input, float vel_input, float accel_input,
		const double pos, float vel, float& accel,
		float vel_min, float vel_max,
		float accel_min, float accel_max,
		float jerk_max, float dt, bool limit_total);

	/* shape_vel_accel and shape_vel_xy calculate a jerk limited path from the current position, velocity and acceleration to an input velocity.
	 The function takes the current position, velocity, and acceleration and calculates the required jerk limited adjustment to the acceleration for the next time dt.
	 The kinematic path is constrained by :
		minimum acceleration - accel_min (must be negative),
		maximum acceleration - accel_max (must be positive),
		maximum jerk - jerk_max (must be positive).
	 The function alters the variable accel to follow a jerk limited kinematic path to vel_input and accel_input.
	 The correction acceleration is limited from accel_min to accel_max. If limit_total is true the target acceleration is limited from accel_min to accel_max.
	*/
	void shape_vel_accel(float vel_input, float accel_input,
		float vel, float& accel,
		float accel_min, float accel_max,
		float jerk_max, float dt, bool limit_total_accel);

	/* shape_accel calculates a jerk limited path from the current acceleration to an input acceleration.
 The function takes the current acceleration and calculates the required jerk limited adjustment to the acceleration for the next time dt.
 The kinematic path is constrained by :
	maximum jerk - jerk_max (must be positive).
 The function alters the variable accel to follow a jerk limited kinematic path to accel_input.
*/
	void shape_accel(float accel_input, float& accel,
		float jerk_max, float dt);

	float calculate_overspeed_gain();

	// update_pos_vel_accel - dual axis projection of position and velocity, pos and vel, forwards in time based on a time step of dt and acceleration of accel.
// the position and velocity is not moved in the direction of limit if limit is not set to zero.
// limit - specifies if the system is unable to continue to accelerate.
// pos_error and vel_error - specifies the direction of the velocity error used in limit handling.
	void update_pos_vel_accel_xy(FVector& pos3, FVector3f& vel3, FVector2f accel, float dt, FVector2f limit, FVector2f pos_error, FVector2f vel_error);

	// update_vel_accel - dual axis projection of position and velocity, pos and vel, forwards in time based on a time step of dt and acceleration of accel.
// the velocity is not moved in the direction of limit if limit is not set to zero.
// limit - specifies if the system is unable to continue to accelerate.
// pos_error and vel_error - specifies the direction of the velocity error used in limit handling.
	//void update_vel_accel_xy(FVector2f& vel, FVector2f& accel, float dt, FVector2f& limit, FVector2f& vel_error);

	// 2D version
	void shape_pos_vel_accel_xy(FVector2d pos_input, FVector2f vel_input, FVector2f accel_input,
		FVector2d pos, FVector2f vel, FVector3f& accel,
		float vel_max, float accel_max,
		float jerk_max, float dt, bool limit_total);

	// 2D version
	void shape_vel_accel_xy(FVector2f vel_input, FVector2f accel_input,
		FVector2f vel, FVector3f& accel,
		float accel_max, float jerk_max, float dt, bool limit_total_accel);

	void shape_accel_xy(FVector2f accel_input, FVector3f& accel3, float jerk_max, float dt);

	const float CORNER_ACCELERATION_RATIO = 0.707f;

	FVector3f get_thrust_vector();


	// parameters
	//AP_Float        _lean_angle_max;    // Maximum autopilot commanded angle (in degrees). Set to zero for Angle Max
	//AP_Float        _shaping_jerk_xy;   // Jerk limit of the xy kinematic path generation in m/s^3 used to determine how quickly the aircraft varies the acceleration target
	//AP_Float        _shaping_jerk_z;    // Jerk limit of the z kinematic path generation in m/s^3 used to determine how quickly the aircraft varies the acceleration target
	TObjectPtr<class UP_2D>       _p_pos_xy;          // XY axis position controller to convert distance error to desired velocity
	TObjectPtr<class UP_1D>       _p_pos_z;           // Z axis position controller to convert altitude error to desired climb rate
	TObjectPtr<class UPID_2D>      _pid_vel_xy;        // XY axis velocity controller to convert velocity error to desired acceleration
	TObjectPtr<class UPIDComponent>    _pid_vel_z;         // Z axis velocity controller to convert climb rate error to desired acceleration
	TObjectPtr<class UPIDComponent>    _pid_accel_z;       // Z axis acceleration controller to convert desired acceleration to throttle output

	// output from controller
	//float       _roll_target;           // desired roll angle in degrees calculated by position controller
	//float       _pitch_target;          // desired roll pitch in degrees calculated by position controller
	//float       _yaw_target;            // desired yaw in degrees calculated by position controller
	float       _yaw_rate_target = 0;       // desired yaw rate in radians per second calculated by position controller

	bool is_active_z = false;
	bool is_active_xy = false;

	UPROPERTY(BlueprintReadWrite)
		float angle_max = 33.0f;

	UPROPERTY(BlueprintReadWrite)
		float       _jerk_max_xy = 5.0f;       // Jerk limit of the xy kinematic path generation in cm/s^3 used to determine how quickly the aircraft varies the acceleration target
	UPROPERTY(BlueprintReadWrite)
		float       _jerk_max_z = 5.0f;        // Jerk limit of the z kinematic path generation in cm/s^3 used to determine how quickly the aircraft varies the acceleration target

	UPROPERTY(BlueprintReadWrite)
		float       VelocityTarget = 0.0f;
	UPROPERTY(BlueprintReadWrite)
		bool		IsFarFromTarget = false;
	UPROPERTY(BlueprintReadWrite)
		FVector3f    _vel_desired;           // desired velocity in NEU cm/s
	UPROPERTY(BlueprintReadWrite)
		FVector3f    _accel_desired;         // desired acceleration in NEU cm/s/s (feed forward)
	UPROPERTY(BlueprintReadWrite)
		float	RocketXYControlRefOffset = -2000.0f;

protected:

	// internal variables
	uint32_t    _last_update_xy_ticks;  // ticks of last last update_xy_controller call
	uint32_t    _last_update_z_ticks;   // ticks of last update_z_controller call
	float       _vel_max_xy;        // max horizontal speed in cm/s used for kinematic shaping
	float       _vel_max_up;        // max climb rate in cm/s used for kinematic shaping
	float       _vel_max_down;      // max descent rate in cm/s used for kinematic shaping
	float       _accel_max_xy = 1.0f;     // max horizontal acceleration in cm/s/s used for kinematic shaping
	float       _accel_max_z = 10.0f;      // max vertical acceleration in cm/s/s used for kinematic shaping
	float       _vel_z_control_ratio = 2.0f;    // confidence that we have control in the vertical axis

	// position controller internal variables
	FVector    _pos_target;            // target location, frame NEU in cm relative to the EKF origin
	FVector3f    _vel_target;            // velocity target in NEU cm/s calculated by pos_to_rate step
	FVector3f    _accel_target;          // acceleration target in NEU cm/s/s
	FVector3f    _limit_vector;          // the direction that the position controller is limited, zero when not limited


	void accel_to_lean_angles(float accel_x, float accel_y, float& roll_target, float& pitch_target);

	// calculate_yaw_and_rate_yaw - update the calculated the vehicle yaw and rate of yaw.
	bool calculate_yaw_rate();



private:


	// angle_to_accel converts a maximum lean angle in degrees to an accel limit in m/s/s
	float angle_to_accel(float angle_deg);

	// accel_to_angle converts a maximum accel in m/s/s to a lean angle in degrees
	float accel_to_angle(float accel);

	void init_xy_controller();
	void init_z_controller();
};
