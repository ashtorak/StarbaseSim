// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

/// @file	AC_P_1D.h
/// @brief	Generic P controller

#include "P_1D.generated.h"

/// @class	AC_P_1D
/// @brief	Object managing one P controller

/**
 * based on ArduCopter class
 */
 
UCLASS(ClassGroup = (Rocket))
class STARBASESIMLIBRARY_API UP_1D : public UObject
{
	GENERATED_BODY()

public:

	UP_1D();

	 // update_all - set target and measured inputs to P controller and calculate outputs
    // target and measurement are filtered
    float update_all(float &target, float measurement);

    // set_limits - sets the maximum error to limit output and first and second derivative of output
    void set_limits(float output_min, float output_max, float D_Out_max = 0.0f, float D2_Out_max = 0.0f);

    // set_error_limits - reduce maximum position error to error_max
    // to be called after setting limits
    void set_error_limits(float error_min, float error_max);

    // get_error_min - return minimum position error
    float get_error_min() const { return _error_min; }

    // get_error_max - return maximum position error
    float get_error_max() const { return _error_max; }


    // accessors
    float get_error() const { return _error; }

	UPROPERTY(BlueprintReadWrite)
		float Kp_default = 1.0f;

	UPROPERTY(BlueprintReadWrite)
		float _kp = 1.0f;

private:

    // internal variables
    float _error;       // time step in seconds
    float _error_min; // error limit in negative direction
    float _error_max; // error limit in positive direction
    float _D1_max;      // maximum first derivative of output

};
