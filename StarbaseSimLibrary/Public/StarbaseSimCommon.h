#pragma once

#include <cmath>
#include <limits>
#include <stdint.h>
#include <type_traits>


inline float getAmbientTemperature()
{
	return 25.0f + 273.15f;
}

// the following are for pos controller

/*
 * @brief: Check whether a float is zero
 */
inline bool is_zero(const float x) {
	return fabsf(x) < FLT_EPSILON;
}

/*
 * @brief: Check whether a double is zero
 */
inline bool is_zero(const double x) {
	return fabs(x) < FLT_EPSILON;
}


/* 
 * @brief: Check whether a float is greater than zero
 */
template <typename T>
inline bool is_positive(const float fVal1) {
    return ((fVal1) >= FLT_EPSILON);
}


/* 
 * @brief: Check whether a float is less than zero
 */
template <typename T>
inline bool is_negative(const float fVal1) {
    return ((fVal1) <= (-1.0 * FLT_EPSILON));
}

/*
 * @brief: Check whether a double is greater than zero
 */
inline bool is_positive(const double fVal1) {
    return (fVal1 >= static_cast<double>(FLT_EPSILON));
}

/*
 * @brief: Check whether a double is less than zero
 */
inline bool is_negative(const double fVal1) {
	return (fVal1 <= static_cast<double>(-1.0 * FLT_EPSILON));
}


/*
 * A variant of sqrt() that checks the input ranges and ensures a valid value
 * as output. If a negative number is given then 0 is returned.  The reasoning
 * is that a negative number for sqrt() in our code is usually caused by small
 * numerical rounding errors, so the real input should have been zero
 */
template <typename T>
float safe_sqrt(const T v);

template<typename T>
inline float safe_sqrt(const T v)
{
	float ret = sqrtf(static_cast<float>(v));
	if (isnan(ret)) {
		return 0;
	}
	return ret;
}

static inline constexpr float sq(const float val)
{
	return val * val;
}

// sqrt_controller calculates the correction based on a proportional controller with piecewise sqrt sections to constrain second derivative.
inline float sqrt_controller(float error, float p, float second_ord_lim, float dt)
{
	float correction_rate;
	if (is_negative(second_ord_lim) || is_zero(second_ord_lim)) {
		// second order limit is zero or negative.
		correction_rate = error * p;
	}
	else if (is_zero(p)) {
		// P term is zero but we have a second order limit.
		if (is_positive(error)) {
			correction_rate = safe_sqrt(2.0 * second_ord_lim * (error));
		}
		else if (is_negative(error)) {
			correction_rate = -safe_sqrt(2.0 * second_ord_lim * (-error));
		}
		else {
			correction_rate = 0.0;
		}
	}
	else {
		// Both the P and second order limit have been defined.
		const float linear_dist = second_ord_lim / FMath::Square(p);
		if (error > linear_dist) {
			correction_rate = safe_sqrt(2.0 * second_ord_lim * (error - (linear_dist / 2.0)));
		}
		else if (error < -linear_dist) {
			correction_rate = -safe_sqrt(2.0 * second_ord_lim * (-error - (linear_dist / 2.0)));
		}
		else {
			correction_rate = error * p;
		}
	}
	if (is_positive(dt)) {
		// this ensures we do not get small oscillations by over shooting the error correction in the last time step.
		return FMath::Clamp(correction_rate, -fabsf(error) / dt, fabsf(error) / dt);
	}
	else {
		return correction_rate;
	}
}

// sqrt_controller calculates the correction based on a proportional controller with piecewise sqrt sections to constrain second derivative.
inline FVector2f sqrt_controller(const FVector2f& error, float p, float second_ord_lim, float dt)
{
	const float error_length = error.Length();
	if (!is_positive(error_length)) {
		return FVector2f{};
	}

	const float correction_length = sqrt_controller(error_length, p, second_ord_lim, dt);
	return error * (correction_length / error_length);
}

// inv_sqrt_controller calculates the inverse of the sqrt controller.
// This function calculates the input (aka error) to the sqrt_controller required to achieve a given output.
inline float inv_sqrt_controller(float output, float p, float D_max)
{
	if (is_positive(D_max) && is_zero(p)) {
		return (output * output) / (2.0 * D_max);
	}
	if ((is_negative(D_max) || is_zero(D_max)) && !is_zero(p)) {
		return output / p;
	}
	if ((is_negative(D_max) || is_zero(D_max)) && is_zero(p)) {
		return 0.0;
	}

	// calculate the velocity at which we switch from calculating the stopping point using a linear function to a sqrt function.
	const float linear_velocity = D_max / p;

	if (fabsf(output) < linear_velocity) {
		// if our current velocity is below the cross-over point we use a linear function
		return output / p;
	}

	const float linear_dist = D_max / sq(p);
	const float stopping_dist = (linear_dist * 0.5f) + sq(output) / (2.0 * D_max);
	return is_positive(output) ? stopping_dist : -stopping_dist;
}

// kinematic_limit calculates the maximum acceleration or velocity in a given direction.
// based on horizontal and vertical limits.
inline float kinematic_limit(FVector3f direction, float max_xy, float max_z_pos, float max_z_neg)
{
	if (is_zero(direction.SquaredLength()) || is_zero(max_xy) || is_zero(max_z_pos) || is_zero(max_z_neg)) {
		return 0.0;
	}

	max_xy = fabsf(max_xy);
	max_z_pos = fabsf(max_z_pos);
	max_z_neg = fabsf(max_z_neg);

	direction.Normalize();
	const float xy_length = FVector2f{ direction.X, direction.Y }.Length();

	if (is_zero(xy_length)) {
		return is_positive(direction.Z) ? max_z_pos : max_z_neg;
	}

	if (is_zero(direction.Z)) {
		return max_xy;
	}

	const float slope = direction.Z / xy_length;
	if (is_positive(slope)) {
		if (fabsf(slope) < max_z_pos / max_xy) {
			return max_xy / xy_length;
		}
		return fabsf(max_z_pos / direction.Z);
	}

	if (fabsf(slope) < max_z_neg / max_xy) {
		return max_xy / xy_length;
	}
	return fabsf(max_z_neg / direction.Z);
}


// limit vector to a given length. returns true if vector was limited
inline bool limit_length(FVector2f& vec, float max_length)
{
	const float len = vec.Length();
	if ((len > max_length) && is_positive(len)) {
		vec.X *= (max_length / len);
		vec.Y *= (max_length / len);
		return true;
	}
	return false;
}

/*
  calculate a low pass filter alpha value
 */
inline float calc_lowpass_alpha_dt(float dt, float cutoff_freq)
{
	if (is_negative(dt) || is_negative(cutoff_freq)) {
		return 1.0;
	}
	if (is_zero(cutoff_freq)) {
		return 1.0;
	}
	if (is_zero(dt)) {
		return 0.0;
	}
	float rc = 1.0f / (TWO_PI * cutoff_freq);
	return dt / (dt + rc);
}

/* limit_accel_xy limits the acceleration to prioritise acceleration perpendicular to the provided velocity vector.
 Input parameters are:
	vel is the velocity vector used to define the direction acceleration limit is biased in.
	accel is the acceleration vector to be limited.
	accel_max is the maximum length of the acceleration vector after being limited.
 Returns true when accel vector has been limited.
*/
inline bool limit_accel_xy(FVector2f vel, FVector3f& accel3, float accel_max)
{
	// check accel_max is defined
	if (!is_positive(accel_max)) {
		return false;
	}

	FVector2f accel = FVector2f(accel3.X, accel3.Y);

	// limit acceleration to accel_max while prioritizing cross track acceleration
	if (accel.SquaredLength() > sq(accel_max)) {

		if (vel.IsZero()) {
			// We do not have a direction of travel so do a simple vector length limit
			limit_length(accel, accel_max);
		}
		else {
			// calculate acceleration in the direction of and perpendicular to the velocity input
			const FVector2f vel_input_unit = vel.GetSafeNormal();
			// acceleration in the direction of travel
			float accel_dir = vel_input_unit.Dot(accel);
			// cross track acceleration
			FVector2f accel_cross = accel - (vel_input_unit * accel_dir);
			if (limit_length(accel_cross, accel_max)) {
				accel_dir = 0.0;
			}
			else {
				float accel_max_dir = safe_sqrt(sq(accel_max) - accel_cross.SquaredLength());
				accel_dir = FMath::Clamp(accel_dir, -accel_max_dir, accel_max_dir);
			}
			accel = accel_cross + vel_input_unit * accel_dir;
		}

		accel3.X = accel.X;
		accel3.Y = accel.Y;

		return true;
	}
	return false;
}