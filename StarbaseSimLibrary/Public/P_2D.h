// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

/// @file	P_2D.h
/// @brief	Generic P controller, 2-dimensional

#include "P_2D.generated.h"

/// @class	P_2D
/// @brief	Object managing one P controller

/**
 * based on ArduCopter class
 */
 
UCLASS(ClassGroup = (Rocket))
class STARBASESIMLIBRARY_API UP_2D : public UObject
{
	GENERATED_BODY()

public:

	UP_2D();

	 // set target and measured inputs to P controller and calculate outputs
	FVector2f update_all(double& target_x, double& target_y, const FVector2d& measurement);

    // set target and measured inputs to P controller and calculate outputs
    // measurement is provided as 3-axis vector but only x and y are used
	FVector2f update_all(double& target_x, double& target_y, const FVector &measurement) {
        return update_all(target_x, target_y, FVector2d{measurement});
    }

    // set_limits - sets the maximum error to limit output and first and second derivative of output
    void set_limits(float output_max, float D_Out_max = 0.0f, float D2_Out_max = 0.0f);

    // set_error_max - reduce maximum position error to error_max
    // to be called after setting limits
    void set_error_max(float error_max);

    // get_error_max - return maximum position error
    float get_error_max() { return _error_max; }


	UPROPERTY(BlueprintReadWrite)
		float Kp_default = 0.5f;

	UPROPERTY(BlueprintReadWrite)
		float _kp = 0.5f;

    FVector2f _error;       // time step in seconds

private:

    // internal variables
    float _error_max; // error limit in positive direction
    float _D1_max;      // maximum first derivative of output

};
