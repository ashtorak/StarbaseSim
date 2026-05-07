#pragma once

/// @file	PID_2D.h
/// @brief	Generic PID algorithm, 2-dimensional.

#include <stdlib.h>
#include <cmath>
#include "PID_2D.generated.h"

/**
 * based on ArduCopter class
 */
 
UCLASS(ClassGroup = (Rocket))
class STARBASESIMLIBRARY_API UPID_2D : public UObject
{
	GENERATED_BODY()

public:

    // Constructor for PID
    UPID_2D();

    // update_all - set target and measured inputs to PID controller and calculate outputs
    // target and error are filtered
    // the derivative is then calculated and filtered
    // the integral is then updated if it does not increase in the direction of the limit vector
    FVector2f update_all(const FVector2f &target, const FVector2f &measurement, float dt, const FVector2f &limit);
    //FVector2f update_all(const Vector3f &target, const Vector3f &measurement, float dt, const Vector3f &limit);

    // update the integral
    // if the limit flag is set the integral is only allowed to shrink
    void update_i(float dt, const FVector2f &limit);

    // get results from pid controller
    FVector2f get_p() const;
    const FVector2f& get_i() const;
    FVector2f get_d() const;
    FVector2f get_ff();
    const FVector2f& get_error() const { return _error; }

    // reset the integrator
    void reset_I();

    // reset_filter - input and D term filter will be reset to the next value provided to set_input()
    void reset_filter() { _reset_filter = true; }

    // integrator setting functions
    void set_integrator(const FVector2f& target, const FVector2f& measurement, const FVector2f& i);
    void set_integrator(const FVector2f& error, const FVector2f& i);
    void set_integrator(const FVector3f& i) { set_integrator(FVector2f{i.X, i.Y}); }
    void set_integrator(const FVector2f& i);

	float get_filt_E_alpha(float dt) const;
	float get_filt_D_alpha(float dt) const;

    // parameters
	UPROPERTY(BlueprintReadWrite)
		float _kp = 2.0f;
	UPROPERTY(BlueprintReadWrite)
		float _ki = 1.0;
	UPROPERTY(BlueprintReadWrite)
		float _kd = 0.5f;
    float _kff = 0.0f;
	UPROPERTY(BlueprintReadWrite)
		float _kimax = 10.0f;
    float _filt_E_hz = 5.0f;         // PID error filter frequency in Hz
    float _filt_D_hz = 5.0f;         // PID derivative filter frequency in Hz

	UPROPERTY(BlueprintReadOnly)
		FVector2f    _error;         // error value to enable filtering
	UPROPERTY(BlueprintReadOnly)
		FVector2f    _derivative;    // last derivative from low-pass filter
	UPROPERTY(BlueprintReadOnly)
		FVector2f    _integrator;    // integrator value

	UPROPERTY(BlueprintReadOnly)
		float Px;
	UPROPERTY(BlueprintReadOnly)
		float Ix;
	UPROPERTY(BlueprintReadOnly)
		float Dx;
	UPROPERTY(BlueprintReadOnly)
		float Py;
	UPROPERTY(BlueprintReadOnly)
		float Iy;
	UPROPERTY(BlueprintReadOnly)
		float Dy;

protected:

    // internal variables
    FVector2f    _target;        // target value to enable filtering
    bool        _reset_filter;  // true when input filter should be reset during next call to update_all

private:
    /*const float default_kp;
    const float default_ki;
    const float default_kd;
    const float default_kff;
    const float default_kimax;
    const float default_filt_E_hz;
    const float default_filt_D_hz;*/
};
