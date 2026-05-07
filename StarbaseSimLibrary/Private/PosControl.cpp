// Fill out your copyright notice in the Description page of Project Settings.


#include "PosControl.h"
#include <Net/UnrealNetwork.h>
#include "Math/UnrealMathUtility.h"
#include "Kismet/KismetMathLibrary.h"
#include "StarbaseSimCommon.h"

UPosControl::UPosControl()
{
	//  needed for replication
	//SetIsReplicatedByDefault(true);
}

//  needed for replication
//void URocketEngine::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
//{
//	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
//
//	DOREPLIFETIME(URocketEngine, isFiring);
//}

/// set_max_speed_accel_xy - set the maximum horizontal speed in cm/s and acceleration in cm/s/s
///     This function only needs to be called if using the kinematic shaping.
///     This can be done at any time as changes in these parameters are handled smoothly
///     by the kinematic shaping.
void UPosControl::set_max_speed_accel_xy(float speed, float accel)
{
	_vel_max_xy = speed;
	_accel_max_xy = accel;

	// ensure the horizontal jerk is less than the vehicle is capable of
	/*const float jerk_max_cmsss = MIN(_attitude_control.get_ang_vel_roll_max_rads(), _attitude_control.get_ang_vel_pitch_max_rads()) * GRAVITY_MSS * 100.0;
	const float snap_max_cmssss = MIN(_attitude_control.get_accel_roll_max_radss(), _attitude_control.get_accel_pitch_max_radss()) * GRAVITY_MSS * 100.0;*/

	// get specified jerk limit
	//_jerk_max_xy = _shaping_jerk_xy * 100.0;

	// limit maximum jerk based on maximum angular rate
	//if (is_positive(jerk_max_cmsss) && _attitude_control.get_bf_feedforward()) {
	//	_jerk_max_xy_cmsss = MIN(_jerk_max_xy_cmsss, jerk_max_cmsss);
	//}

	//// limit maximum jerk to maximum possible average jerk based on angular acceleration
	//if (is_positive(snap_max_cmssss) && _attitude_control.get_bf_feedforward()) {
	//	_jerk_max_xy_cmsss = MIN(0.5 * safe_sqrt(_accel_max_xy_cmss * snap_max_cmssss), _jerk_max_xy_cmsss);
	//}
}

///
/// Vertical position controller
///

/// set_max_speed_accel_z - set the maximum vertical speed in cm/s and acceleration in cm/s/s
///     speed_down can be positive or negative but will always be interpreted as a descent speed.
///     This function only needs to be called if using the kinematic shaping.
///     This can be done at any time as changes in these parameters are handled smoothly
///     by the kinematic shaping.
void UPosControl::set_max_speed_accel_z(float speed_down, float speed_up, float accel)
{
	// ensure speed_down is always negative
	speed_down = -fabsf(speed_down);

	// sanity check and update
	if (speed_down < 0) {
		_vel_max_down = speed_down;
	}
	if (speed_up > 0) {
		_vel_max_up = speed_up;
	}
	if (accel > 0) {
		_accel_max_z = accel;
		//	_jerk_max_z = _accel_max_z = accel;
	}

	// ensure the vertical Jerk is not limited by the filters in the Z accel PID object
	/*_jerk_max_zss = _shaping_jerk_z * 100.0;
	if (is_positive(_pid_accel_z.filt_T_hz())) {
		_jerk_max_zss = MIN(_jerk_max_zss, MIN(GRAVITY_MSS * 100.0, _accel_max_zs) * (M_2PI * _pid_accel_z.filt_T_hz()) / 5.0);
	}
	if (is_positive(_pid_accel_z.filt_E_hz())) {
		_jerk_max_zss = MIN(_jerk_max_zss, MIN(GRAVITY_MSS * 100.0, _accel_max_zs) * (M_2PI * _pid_accel_z.filt_E_hz()) / 5.0);
	}*/
}

/// update_z_controller - runs the vertical position controller correcting position, velocity and acceleration errors.
///     Position and velocity errors are converted to velocity and acceleration targets using PID objects
///     Desired velocity and accelerations are added to these corrections as they are calculated
///     Kinematically consistent target position and desired velocity and accelerations should be provided before calling this function
float UPosControl::update_z_controller(float DeltaTime)
{
	if (!is_active_z && !IsFarFromTarget) {
		init_z_controller();
	}

	float throttle = 0;

	// calculate the target velocity correction
	float pos_target_zf = _pos_target.Z;

	_vel_target.Z = _p_pos_z->update_all(pos_target_zf, RocketController->Altitude);


	// add feed forward component
	if (IsFarFromTarget)
	{
		_vel_target.Z = VelocityTarget; // from blueprint
		is_active_z = false;
		_pos_target.Z = RocketController->PositionTarget.Z;
	}
	else _pos_target.Z = pos_target_zf;
	//else _vel_target.Z += _vel_desired.Z;

	// Velocity Controller

	const float curr_vel_z = RocketController->Velocity.Z;
	_pid_vel_z->ProcessValue = curr_vel_z;
	_pid_vel_z->Setpoint = _vel_target.Z;
	_pid_vel_z->Limit = _limit_vector.Z;
	_pid_vel_z->CalculateOutput(DeltaTime);
	_accel_target.Z = _pid_vel_z->Output;
		//update_all(_vel_target.z, curr_vel_z, _dt, _motors.limit.throttle_lower, _motors.limit.throttle_upper);
	//_accel_target.Z *= AP::ahrs().getControlScaleZ();

	// add feed forward component
	if (!IsFarFromTarget) _accel_target.Z += _accel_desired.Z;

	// Acceleration Controller

	// Calculate vertical acceleration
	const float z_accel_meas = RocketController->Acceleration.Z;

	// ensure imax is always large enough to overpower hover throttle
	/*if (_motors.get_throttle_hover() * 1000.0f > _pid_accel_z.imax()) {
		_pid_accel_z.imax(_motors.get_throttle_hover() * 1000.0f);
	}

		thr_out = _pid_accel_z.update_all(_accel_target.z, z_accel_meas, _dt, (_motors.limit.throttle_lower || _motors.limit.throttle_upper)) * 0.001f;
		thr_out += _pid_accel_z.get_ff() * 0.001f;
	
	thr_out += _motors.get_throttle_hover();*/

	_pid_accel_z->ProcessValue = z_accel_meas;
	_pid_accel_z->Setpoint = _accel_target.Z;
	_pid_accel_z->CalculateOutput(DeltaTime);
	throttle = UKismetMathLibrary::SafeDivide(_pid_accel_z->Output, _pid_accel_z->OutputMax);
	//throttle = _pid_accel_z->Output;
	throttle += (RocketController->HoverThrottle);

	// Actuator commands

	// Check for vertical controller health

	// _speed_down is checked to be non-zero when set - NOT USED IN ARDUCOPTER LIBRARY
	/*float error_ratio = _pid_vel_z->Error / _vel_max_down;
	_vel_z_control_ratio += _dt * 0.1f * (0.5 - error_ratio);
	_vel_z_control_ratio = FMath::Clamp(_vel_z_control_ratio, 0.0f, 2.0f);*/

	// set vertical component of the limit vector
	//if (IsFarFromTarget)
	//{
	//	if (throttle > 0.5f) {
	//		_limit_vector.Z = 1.0f;
	//	}
	//	else if (throttle < 0.5f) {
	//		_limit_vector.Z = -1.0f;
	//	}
	//}
	//else 
	//{
		if (throttle >= 1.f) {
			_limit_vector.Z = 1.0f;
		}
		else if (throttle <= -0.2f) {
			_limit_vector.Z = -1.0f;
		}
		else {
			_limit_vector.Z = 0.0f;
		}
	//}

	return throttle;
}

/// update_xy_controller - runs the horizontal position controller correcting position, velocity and acceleration errors.
///     Position and velocity errors are converted to velocity and acceleration targets using PID objects
///     Desired velocity and accelerations are added to these corrections as they are calculated
///     Kinematically consistent target position and desired velocity and accelerations should be provided before calling this function
void UPosControl::update_xy_controller(float DeltaTime)
{
	if (!is_active_xy) {
		init_xy_controller();
	}

	// Position Controller

	FVector RocketXYControlRef = (RocketController->MainMeshTransform.GetLocation() + RocketXYControlRefOffset * RocketController->MainMeshTransform.GetRotation().GetUpVector()) * 0.01f;
	FVector2f vel_target = _p_pos_xy->update_all(_pos_target.X, _pos_target.Y, RocketXYControlRef);

	vel_target.X += _vel_desired.X;
	vel_target.Y += _vel_desired.Y;

	// Velocity Controller

	const FVector2f& curr_vel = FVector2f(RocketController->Velocity.X, RocketController->Velocity.Y);
	FVector2f accel_target = _pid_vel_xy->update_all(vel_target, curr_vel, DeltaTime, FVector2f(_limit_vector));

	_vel_target.X = vel_target.X;
	_vel_target.Y = vel_target.Y;

	// pass the correction acceleration to the target acceleration output
	_accel_target.X = accel_target.X;
	_accel_target.X += _accel_desired.X;
	_accel_target.Y = accel_target.Y;
	_accel_target.Y += _accel_desired.Y;

	// Acceleration Controller

	// limit acceleration using maximum lean angles
	float accel_max = angle_to_accel(angle_max);
	// Define the limit vector before we constrain _accel_target 
	_limit_vector.X = _accel_target.X;
	_limit_vector.Y = _accel_target.Y;
	if (!limit_accel_xy(FVector2f(_vel_desired), _accel_target, accel_max)) {
		// _accel_target was not limited so we can zero the xy limit vector
		_limit_vector.X = _limit_vector.Y = 0.0f;
	}

	// update angle targets that will be passed to stabilize controller
	//accel_to_lean_angles(_accel_target.x, _accel_target.y, _roll_target, _pitch_target);
	//calculate_yaw_rate();
	// yaw rate and thrust vector (from accel target) will be used for attitude controller
}


///
/// 3D position shaper
///

/// input_pos_xyz - calculate a jerk limited path from the current position, velocity and acceleration to an input position.
///     The function takes the current position, velocity, and acceleration and calculates the required jerk limited adjustment to the acceleration for the next time dt.
///     The kinematic path is constrained by the maximum jerk parameter and the velocity and acceleration limits set using the function set_max_speed_accel_xy.
///     The jerk limit defines the acceleration error decay in the kinematic path as the system approaches constant acceleration.
///     The jerk limit also defines the time taken to achieve the maximum acceleration.
///     The function alters the input velocity to be the velocity that the system could reach zero acceleration in the minimum time.
void UPosControl::input_pos_xyz(const FVector& pos, float pos_offset_z, float pos_offset_z_buffer, float DeltaTime)
{
	// Terrain following velocity scalar must be calculated before we remove the position offset
	//const float offset_z_scaler = pos_offset_z_scaler(pos_offset_z, pos_offset_z_buffer);

	// remove terrain offsets for flat earth assumption
	/*_pos_target.z -= _pos_offset_z;
	_vel_desired.z -= _vel_offset_z;
	_accel_desired.z -= _accel_offset_z;*/

	// calculated increased maximum acceleration and jerk if over speed
	const float overspeed_gain = calculate_overspeed_gain();
	const float accel_max_z = _accel_max_z * overspeed_gain;
	const float jerk_max_z = _jerk_max_z * overspeed_gain;

	update_pos_vel_accel_xy(_pos_target, _vel_desired, FVector2f(_accel_desired), DeltaTime, FVector2f(_limit_vector), _p_pos_xy->_error, _pid_vel_xy->_error);

	// adjust desired altitude if motors have not hit their limits
	update_pos_vel_accel(_pos_target.Z, _vel_desired.Z, _accel_desired.Z, DeltaTime, _limit_vector.Z, _p_pos_z->get_error(), _pid_vel_z->Error);

	// calculate the horizontal and vertical velocity limits to travel directly to the destination defined by pos
	float vel_max_xy = 0.0f;
	float vel_max_z = 0.0f;
	FVector3f dest_vector = FVector3f(pos - _pos_target);
	if (is_positive(dest_vector.SquaredLength())) {
		dest_vector.Normalize();
		float dest_vector_xy_length = dest_vector.Size2D();

		float vel_max = kinematic_limit(dest_vector, _vel_max_xy, _vel_max_up, _vel_max_down);
		vel_max_xy = vel_max * dest_vector_xy_length;
		vel_max_z = fabsf(vel_max * dest_vector.Z);
	}

	// reduce speed if we are reaching the edge of our vertical buffer
	//vel_max_xy *= offset_z_scaler;

	FVector2f vel = vel.ZeroVector;
	FVector2f accel = vel;
	shape_pos_vel_accel_xy(FVector2d(pos), vel, accel, FVector2d(_pos_target), FVector2f(_vel_desired), _accel_desired,
		vel_max_xy, _accel_max_xy, _jerk_max_xy, DeltaTime, false);

	float posz = pos.Z;
	shape_pos_vel_accel(posz, 0, 0,
		_pos_target.Z, _vel_desired.Z, _accel_desired.Z,
		-vel_max_z, vel_max_z,
		-FMath::Clamp(accel_max_z, 0.0f, 1750.0f), accel_max_z,
		jerk_max_z, DeltaTime, false);

	// update the vertical position, velocity and acceleration offsets
	//update_pos_offset_z(pos_offset_z);

	// add terrain offsets
	/*_pos_target.z += _pos_offset_z;
	_vel_desired.z += _vel_offset_z;
	_accel_desired.z += _accel_offset_z;*/
}

// update_pos_vel_accel - single axis projection of position and velocity forward in time based on a time step of dt and acceleration of accel.
// the position and velocity is not moved in the direction of limit if limit is not set to zero.
// limit - specifies if the system is unable to continue to accelerate.
// pos_error and vel_error - specifies the direction of the velocity error used in limit handling.
void UPosControl::update_pos_vel_accel(double& pos, float& vel, float accel, float dt, float limit, float pos_error, float vel_error)
{
	// move position and velocity forward by dt if it does not increase error when limited.
	float delta_pos = vel * dt + accel * 0.5f * FMath::Square(dt);
	// do not add delta_pos if it will increase the velocity error in the direction of limit
	if (is_positive(delta_pos * limit) && is_positive(pos_error * limit)) {
		delta_pos = 0.0;
	}
	pos += delta_pos;

	update_vel_accel(vel, accel, dt, limit, vel_error);
}

// update_vel_accel - single axis projection of velocity, vel, forwards in time based on a time step of dt and acceleration of accel.
// the velocity is not moved in the direction of limit if limit is not set to zero.
// limit - specifies if the system is unable to continue to accelerate.
// vel_error - specifies the direction of the velocity error used in limit handling.
void UPosControl::update_vel_accel(float& vel, float accel, float dt, float limit, float vel_error)
{
	float delta_vel = accel * dt;
	// do not add delta_vel if it will increase the velocity error in the direction of limit
	// unless adding delta_vel will reduce vel towards zero
	if (is_positive(delta_vel * limit) && is_positive(vel_error * limit)) {
		if (is_negative(vel * limit)) {
			delta_vel = FMath::Clamp(delta_vel, -fabsf(vel), fabsf(vel));
		}
		else {
			delta_vel = 0.0;
		}
	}
	vel += delta_vel;
}

/* shape_pos_vel_accel calculate a jerk limited path from the current position, velocity and acceleration to an input position and velocity.
 The function takes the current position, velocity, and acceleration and calculates the required jerk limited adjustment to the acceleration for the next time dt.
 The kinematic path is constrained by :
	minimum velocity - vel_min (must not be positive),
	maximum velocity - vel_max (must not be negative),
	minimum acceleration - accel_min (must be negative),
	maximum acceleration - accel_max (must be positive),
	maximum jerk - jerk_max (must be positive).
 The function alters the variable accel to follow a jerk limited kinematic path to pos_input, vel_input and accel_input.
 The correction velocity is limited to vel_max to vel_min. If limit_total is true the target velocity is limited to vel_max to vel_min.
 The correction acceleration is limited from accel_min to accel_max. If limit_total is true the target acceleration is limited from accel_min to accel_max.
*/
void UPosControl::shape_pos_vel_accel(double pos_input, float vel_input, float accel_input,
	double pos, float vel, float& accel,
	float vel_min, float vel_max,
	float accel_min, float accel_max,
	float jerk_max, float dt, bool limit_total)
{
	// sanity check vel_min, vel_max, accel_min, accel_max and jerk_max.
	if (is_positive(vel_min) || is_negative(vel_max) || !is_negative(accel_min) || !is_positive(accel_max) || !is_positive(jerk_max)) {
		
		return;
	}

	// position error to be corrected
	float pos_error = pos_input - pos;

	// Calculate time constants and limits to ensure stable operation
	// The negative acceleration limit is used here because the square root controller
	// manages the approach to the setpoint. Therefore the acceleration is in the opposite
	// direction to the position error.
	float accel_tc_max;
	float KPv;
	if (is_positive(pos_error)) {
		accel_tc_max = -0.5 * accel_min;
		KPv = 0.5 * jerk_max / (-accel_min);
	}
	else {
		accel_tc_max = 0.5 * accel_max;
		KPv = 0.5 * jerk_max / accel_max;
	}

	// velocity to correct position
	float vel_target = sqrt_controller(pos_error, KPv, accel_tc_max, dt);

	// limit velocity between vel_min and vel_max
	if (is_negative(vel_min) || is_positive(vel_max)) {
		vel_target = FMath::Clamp(vel_target, vel_min, vel_max);
	}

	// velocity correction with input velocity
	vel_target += vel_input;

	// limit velocity between vel_min and vel_max
	if (limit_total) {
		vel_target = FMath::Clamp(vel_target, vel_min, vel_max);
	}

	shape_vel_accel(vel_target, accel_input, vel, accel, accel_min, accel_max, jerk_max, dt, limit_total);
}

/* shape_vel_accel and shape_vel_xy calculate a jerk limited path from the current position, velocity and acceleration to an input velocity.
 The function takes the current position, velocity, and acceleration and calculates the required jerk limited adjustment to the acceleration for the next time dt.
 The kinematic path is constrained by :
	minimum acceleration - accel_min (must be negative),
	maximum acceleration - accel_max (must be positive),
	maximum jerk - jerk_max (must be positive).
 The function alters the variable accel to follow a jerk limited kinematic path to vel_input and accel_input.
 The correction acceleration is limited from accel_min to accel_max. If limit_total is true the target acceleration is limited from accel_min to accel_max.
*/
void UPosControl::shape_vel_accel(float vel_input, float accel_input,
	float vel, float& accel,
	float accel_min, float accel_max,
	float jerk_max, float dt, bool limit_total_accel)
{
	// sanity check accel_min, accel_max and jerk_max.
	if (!is_negative(accel_min) || !is_positive(accel_max) || !is_positive(jerk_max)) {
		return;
	}

	// velocity error to be corrected
	float vel_error = vel_input - vel;

	// Calculate time constants and limits to ensure stable operation
	// The direction of acceleration limit is the same as the velocity error.
	// This is because the velocity error is negative when slowing down while
	// closing a positive position error.
	float KPa;
	if (is_positive(vel_error)) {
		KPa = jerk_max / accel_max;
	}
	else {
		KPa = jerk_max / (-accel_min);
	}

	// acceleration to correct velocity
	float accel_target = sqrt_controller(vel_error, KPa, jerk_max, dt);

	// constrain correction acceleration from accel_min to accel_max
	accel_target = FMath::Clamp(accel_target, accel_min, accel_max);

	// velocity correction with input velocity
	accel_target += accel_input;

	// constrain total acceleration from accel_min to accel_max
	if (limit_total_accel) {
		accel_target = FMath::Clamp(accel_target, accel_min, accel_max);
	}

	shape_accel(accel_target, accel, jerk_max, dt);
}


/* shape_accel calculates a jerk limited path from the current acceleration to an input acceleration.
 The function takes the current acceleration and calculates the required jerk limited adjustment to the acceleration for the next time dt.
 The kinematic path is constrained by :
	maximum jerk - jerk_max (must be positive).
 The function alters the variable accel to follow a jerk limited kinematic path to accel_input.
*/
void UPosControl::shape_accel(float accel_input, float& accel,
	float jerk_max, float dt)
{
	// sanity check jerk_max
	if (!is_positive(jerk_max)) {
		return;
	}

	// jerk limit acceleration change
	if (is_positive(dt)) {
		float accel_delta = accel_input - accel;
		accel_delta = FMath::Clamp(accel_delta, -jerk_max * dt, jerk_max * dt);
		accel += accel_delta;
	}
}

// calculate_overspeed_gain - calculated increased maximum acceleration and jerk if over speed condition is detected
float UPosControl::calculate_overspeed_gain()
{
	if (_vel_desired.Z < _vel_max_down && !is_zero(_vel_max_down)) {
		return 2 * _vel_desired.Z / _vel_max_down;
	}
	if (_vel_desired.Z > _vel_max_up && !is_zero(_vel_max_up)) {
		return 2 * _vel_desired.Z / _vel_max_up;
	}
	return 1.0;
}

// update_pos_vel_accel - dual axis projection of position and velocity, pos and vel, forwards in time based on a time step of dt and acceleration of accel.
// the position and velocity is not moved in the direction of limit if limit is not set to zero.
// limit - specifies if the system is unable to continue to accelerate.
// pos_error and vel_error - specifies the direction of the velocity error used in limit handling.
void UPosControl::update_pos_vel_accel_xy(FVector& pos3, FVector3f& vel3, FVector2f accel, float dt, FVector2f limit, FVector2f pos_error, FVector2f vel_error)
{
	FVector2f vel = FVector2f(vel3);

	// move position and velocity forward by dt.
	FVector2f delta_pos = vel * dt + accel * 0.5f * sq(dt);

	if (!is_zero(limit.SquaredLength())) {
		// zero delta_pos if it will increase the velocity error in the direction of limit
		if (is_positive(delta_pos.Dot(limit)) && is_positive(pos_error.Dot(limit))) {
			delta_pos.Zero();
		}
	}

	pos3.X += delta_pos.X;
	pos3.Y += delta_pos.Y;

	/*update_vel_accel_xy(vel, accel, dt, limit, vel_error);
}*/

// update_vel_accel - dual axis projection of position and velocity, pos and vel, forwards in time based on a time step of dt and acceleration of accel.
// the velocity is not moved in the direction of limit if limit is not set to zero.
// limit - specifies if the system is unable to continue to accelerate.
// pos_error and vel_error - specifies the direction of the velocity error used in limit handling.
//void UPosControl::update_vel_accel_xy(FVector& vel, FVector& accel, float dt, FVector2f limit, FVector2f vel_error)
//{
	// increase velocity by acceleration * dt if it does not increase error when limited.
	// unless adding delta_vel will reduce the magnitude of vel
	FVector2f delta_vel = accel * dt;
	if (!limit.IsZero() && !delta_vel.IsZero()) {
		// check if delta_vel will increase the velocity error in the direction of limit
		if (is_positive(delta_vel.Dot(limit)) && is_positive(vel_error.Dot(limit)) && !is_negative(vel.Dot(limit))) {
			delta_vel.Zero();
		}
	}
	vel3.X += delta_vel.X;
	vel3.Y += delta_vel.Y;
}

// 2D version
void UPosControl::shape_pos_vel_accel_xy(FVector2d pos_input, FVector2f vel_input, FVector2f accel_input,
	FVector2d pos, FVector2f vel, FVector3f& accel,
	float vel_max, float accel_max,
	float jerk_max, float dt, bool limit_total)
{
	// sanity check vel_max, accel_max and jerk_max.
	if (is_negative(vel_max) || !is_positive(accel_max) || !is_positive(jerk_max)) {
		return;
	}

	// Calculate time constants and limits to ensure stable operation
	const float KPv = 0.5 * jerk_max / accel_max;
	// reduce breaking acceleration to support cornering without overshooting the stopping point
	const float accel_tc_max = 0.5 * accel_max;

	// position error to be corrected
	FVector2f pos_error = FVector2f(pos_input - pos);

	// velocity to correct position
	FVector2f vel_target = sqrt_controller(pos_error, KPv, accel_tc_max, dt);

	// limit velocity to vel_max
	if (is_positive(vel_max)) {
		limit_length(vel_target, vel_max);
	}

	// velocity correction with input velocity
	vel_target = vel_target + vel_input;

	// limit velocity to vel_max
	if (limit_total) {
		limit_length(vel_target, vel_max);
	}

	shape_vel_accel_xy(vel_target, accel_input, vel, accel, accel_max, jerk_max, dt, limit_total);
}

// 2D version
void UPosControl::shape_vel_accel_xy(FVector2f vel_input, FVector2f accel_input,
	FVector2f vel, FVector3f& accel,
	float accel_max, float jerk_max, float dt, bool limit_total_accel)
{
	// sanity check accel_max and jerk_max.
	if (!is_positive(accel_max) || !is_positive(jerk_max)) {
		return;
	}

	// Calculate time constants and limits to ensure stable operation
	const float KPa = jerk_max / accel_max;

	// velocity error to be corrected
	const FVector2f vel_error = vel_input - vel;

	// acceleration to correct velocity
	FVector2f accel_target = sqrt_controller(vel_error, KPa, jerk_max, dt);

	// limit correction acceleration to accel_max
	if (vel_input.IsNearlyZero()) {
		limit_length(accel_target, accel_max);
	}
	else {
		// calculate acceleration in the direction of and perpendicular to the velocity input
		const FVector2f vel_input_unit = vel_input.GetSafeNormal();
		float accel_dir = vel_input_unit.Dot(accel_target);
		FVector2f accel_cross = accel_target - (vel_input_unit * accel_dir);

		// ensure 1/sqrt(2) of maximum acceleration is available to correct cross component 
		// relative to vel_input
		if (sq(accel_dir) <= accel_cross.SquaredLength()) {
			// accel_target can be simply limited in magnitude
			limit_length(accel_target, accel_max);
		}
		else {
			// limiting the length of the vector will reduce the lateral acceleration below 1/sqrt(2)
			// limit the lateral acceleration to 1/sqrt(2) and retain as much of the remaining
			// acceleration as possible.
			limit_length(accel_cross, CORNER_ACCELERATION_RATIO * accel_max);
			float accel_max_dir = safe_sqrt(sq(accel_max) - accel_cross.SquaredLength());
			accel_dir = FMath::Clamp(accel_dir, -accel_max_dir, accel_max_dir);
			accel_target = accel_cross + vel_input_unit * accel_dir;
		}
	}

	accel_target += accel_input;

	// limit total acceleration to accel_max
	if (limit_total_accel) {
		limit_length(accel_target, accel_max);
	}

	shape_accel_xy(accel_target, accel, jerk_max, dt);
}

// 2D version
void UPosControl::shape_accel_xy(FVector2f accel_input, FVector3f& accel3, float jerk_max, float dt)
{
	// sanity check jerk_max
	if (!is_positive(jerk_max)) {
		return;
	}

	FVector2f accel = FVector2f(accel3);
	// jerk limit acceleration change
	if (is_positive(dt)) {
		FVector2f accel_delta = accel_input - accel;
		limit_length(accel_delta, jerk_max * dt);
		accel = accel + accel_delta;
	}
	accel3.X = accel.X;
	accel3.Y = accel.Y;
}

void UPosControl::accel_to_lean_angles(float accel_x, float accel_y, float& roll_target, float& pitch_target)
{
	//// rotate accelerations into body forward-right frame
	//const float accel_forward = accel_x * _ahrs.cos_yaw() + accel_y * _ahrs.sin_yaw();
	//const float accel_right = -accel_x * _ahrs.sin_yaw() + accel_y * _ahrs.cos_yaw();

	//// update angle targets that will be passed to stabilize controller
	//pitch_target = accel_to_angle(-accel_forward);
	//float cos_pitch_target = cosf(pitch_target * PI / 180.0f);
	//roll_target = accel_to_angle(accel_right * cos_pitch_target);
}

// calculate_yaw_and_rate_yaw - update the calculated the vehicle yaw and rate of yaw.
bool UPosControl::calculate_yaw_rate()
{
	const float vel_desired_xy_len = FVector2f(_vel_desired).Length();

	// update the target yaw if velocity is greater than 5% _vel_max_xy
	if (vel_desired_xy_len > _vel_max_xy * 0.05f) {
		//_yaw_target = degrees(_vel_desired.xy().angle()) * 100.0f;

		// Calculate the turn rate
		float turn_rate = 0.0f;
		if (is_positive(vel_desired_xy_len)) {
			const float accel_forward = (_accel_desired.X * _vel_desired.X + _accel_desired.Y * _vel_desired.Y) / vel_desired_xy_len;
			const FVector2f accel_turn = FVector2f(_accel_desired) - FVector2f(_vel_desired) * accel_forward / vel_desired_xy_len;
			const float accel_turn_xy_len = accel_turn.Length();
			turn_rate = accel_turn_xy_len / vel_desired_xy_len;
			if ((accel_turn.Y * _vel_desired.X - accel_turn.X * _vel_desired.Y) < 0.0) {
				turn_rate = -turn_rate;
			}
		}
		_yaw_rate_target = turn_rate*100;
		return true;
	}
	return false;
}

// returns the NED target acceleration vector for attitude control
FVector3f UPosControl::get_thrust_vector()
{
	FVector3f accel_target = _accel_target;
	accel_target.Z = -RocketController->Gravity.Z;
	return accel_target;
}

// angle_to_accel converts a maximum lean angle in degrees to an accel limit in m/s/s
float UPosControl::angle_to_accel(float angle_deg)
{
	return FMath::Abs(RocketController->Gravity.Z) * tanf(FMath::DegreesToRadians(angle_deg));
}

// accel_to_angle converts a maximum accel in m/s/s to a lean angle in degrees
float UPosControl::accel_to_angle(float accel)
{
	return FMath::RadiansToDegrees(atanf((accel / FMath::Abs(RocketController->Gravity.Z))));
}

/// init_xy_controller - initialise the position controller to the current position, velocity, acceleration and attitude.
///     This function is the default initialisation for any position control that provides position, velocity and acceleration.
void UPosControl::init_xy_controller()
{
	_yaw_rate_target = 0.0f;

	//_pos_target.xy() = _inav.get_position_xy_cm().topostype();
	_pos_target = RocketController->CurrentPosition;

	/*const Vector2f& curr_vel = _inav.get_velocity_xy_cms();
	_vel_desired.xy() = curr_vel;
	_vel_target.xy() = curr_vel;*/
	_vel_desired = _vel_target = FVector3f(RocketController->Velocity);

	// Set desired accel to zero because raw acceleration is prone to noise
	//_accel_desired.xy().zero();
	_accel_desired = FVector3f(RocketController->Acceleration);

	_accel_target.X = RocketController->_attitude_target.X;
	_accel_target.Y = RocketController->_attitude_target.Y;

	// initialise I terms from lean angles
	_pid_vel_xy->reset_filter();
	// initialise the I term to _accel_target - _accel_desired
	// _accel_desired is zero and can be removed from the equation
	_pid_vel_xy->set_integrator(_accel_target);// -_vel_target.xy() * _pid_vel_xy.ff());

	is_active_xy = true;
}

/// init_z_controller - initialise the position controller to the current position, velocity, acceleration and attitude.
///     This function is the default initialisation for any position control that provides position, velocity and acceleration.
///     This function is private and contains all the shared z axis initialisation functions
void UPosControl::init_z_controller()
{
	//_pos_target.z = _inav.get_position_z_up_cm();

	//const float curr_vel_z = _inav.get_velocity_z_up_cms();
	//_vel_desired.z = curr_vel_z;
	//// with zero position error _vel_target = _vel_desired
	//_vel_target.z = curr_vel_z;

	_pid_vel_z->Zero();

	_accel_desired.Z = FMath::Clamp(RocketController->Acceleration.Z, -_accel_max_z, _accel_max_z);
	// with zero position error _accel_target = _accel_desired
	_accel_target.Z = _accel_desired.Z;
	_pid_accel_z->Zero();

	// Set accel PID I term based on the current throttle
	_pid_accel_z->I = RocketController->ThrottleRequest - RocketController->HoverThrottle;

	is_active_z = true;
}
