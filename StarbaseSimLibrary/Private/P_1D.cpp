#include "P_1D.h"
#include "Math/UnrealMathUtility.h"
#include "StarbaseSimCommon.h"


UP_1D::UP_1D()
{
}

// update_all - set target and measured inputs to P controller and calculate outputs
// target and measurement are filtered
float UP_1D::update_all(float &target, float measurement)
{
    // calculate distance _error
    _error = target - measurement;

    if (is_negative(_error_min) && (_error < _error_min)) {
        _error = _error_min;
        target = measurement + _error;
    } else if (is_positive(_error_max) && (_error > _error_max)) {
        _error = _error_max;
        target = measurement + _error;
    }

    // MIN(_Dxy_max, _D2xy_max / _kxy_P) limits the max accel to the point where max jerk is exceeded
    return sqrt_controller(_error, _kp, _D1_max, 0.0);
}

// set_limits - sets the maximum error to limit output and first and second derivative of output
// when using for a position controller, lim_err will be position error, lim_out will be correction velocity, lim_D will be acceleration, lim_D2 will be jerk
void UP_1D::set_limits(float output_min, float output_max, float D_Out_max, float D2_Out_max)
{
    _D1_max = 0.0f;
    _error_min = 0.0f;
    _error_max = 0.0f;

    if (is_positive(D_Out_max)) {
        _D1_max = D_Out_max;
    }

    if (is_positive(D2_Out_max) && is_positive(_kp)) {
        // limit the first derivative so as not to exceed the second derivative
        _D1_max = FMath::Min(_D1_max, D2_Out_max / _kp);
    }

    if (is_negative(output_min) && is_positive(_kp)) {
        _error_min = inv_sqrt_controller(output_min, _kp, _D1_max);
    }

    if (is_positive(output_max) && is_positive(_kp)) {
        _error_max = inv_sqrt_controller(output_max, _kp, _D1_max);
    }
}


// set_error_limits - reduce maximum error to error_max
// to be called after setting limits
void UP_1D::set_error_limits(float error_min, float error_max)
{
    if (is_negative(error_min)) {
        if (!is_zero(_error_min)) {
            _error_min = FMath::Max(_error_min, error_min);
        } else {
            _error_min = error_min;
        }
    }

    if (is_positive(error_max)) {
        if (!is_zero(_error_max)) {
            _error_max = FMath::Min(_error_max, error_max);
        } else {
            _error_max = error_max;
        }
    }
}
