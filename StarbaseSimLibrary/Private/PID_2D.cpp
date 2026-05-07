/// @file	AC_PID_2D.cpp
/// @brief	Generic PID algorithm

#include "PID_2D.h"
#include "Math/UnrealMathUtility.h"
#include "StarbaseSimCommon.h"

// Constructor
UPID_2D::UPID_2D()
{
    // reset input filter to first value received
    _reset_filter = true;
}

//  update_all - set target and measured inputs to PID controller and calculate outputs
//  target and error are filtered
//  the derivative is then calculated and filtered
//  the integral is then updated if it does not increase in the direction of the limit vector
FVector2f UPID_2D::update_all(const FVector2f &target, const FVector2f &measurement, float dt, const FVector2f &limit)
{
    // don't process inf or NaN
    if (target.ContainsNaN() || 
        measurement.ContainsNaN()) {
        return FVector2f{};
    }

    _target = target;

    // reset input filter to value received
    if (_reset_filter) {
        _reset_filter = false;
        _error = _target - measurement;
        _derivative.Zero();
    } else {
        FVector2f error_last{_error};
        _error += ((_target - measurement) - _error) * get_filt_E_alpha(dt);

        // calculate and filter derivative
        if (is_positive(dt)) {
            const FVector2f derivative{(_error - error_last) / dt};
            _derivative += (derivative - _derivative) * get_filt_D_alpha(dt);
        }
    }

    // update I term
    update_i(dt, limit);

    /*_pid_info_x.target = _target.x;
    _pid_info_x.actual = measurement.x;
    _pid_info_x.error = _error.x;
    _pid_info_x.P = _error.x * _kp;
    _pid_info_x.I = _integrator.x;
    _pid_info_x.D = _derivative.x * _kd;
    _pid_info_x.FF = _target.x * _kff;

    _pid_info_y.target = _target.y;
    _pid_info_y.actual = measurement.y;
    _pid_info_y.error = _error.y;
    _pid_info_y.P = _error.y * _kp;
    _pid_info_y.I = _integrator.y;
    _pid_info_y.D = _derivative.y * _kd;
    _pid_info_y.FF = _target.y * _kff;*/

	Px = _error.X * _kp;
	Ix = _integrator.X;
	Dx = _derivative.X * _kd;

	Py = _error.Y * _kp;
	Iy = _integrator.Y;
	Dy = _derivative.Y * _kd;

    return _error * _kp + _integrator + _derivative * _kd + _target * _kff;
}

//FVector2f UPID_2D::update_all(const Vector3f &target, const Vector3f &measurement, float dt, const Vector3f &limit)
//{
//    return update_all(FVector2f{target.x, target.y}, FVector2f{measurement.x, measurement.y}, dt, FVector2f{limit.x, limit.y});
//}

//  update_i - update the integral
//  If the limit is set the integral is only allowed to reduce in the direction of the limit
void UPID_2D::update_i(float dt, const FVector2f &limit)
{
    /*_pid_info_x.limit = false;
    _pid_info_y.limit = false;*/

    FVector2f delta_integrator = (_error * _ki) * dt;
    float integrator_length = _integrator.Length();
    _integrator += delta_integrator;
    // do not let integrator increase in length if delta_integrator is in the direction of limit
    if (is_positive(delta_integrator.Dot(limit)) && limit_length(_integrator, integrator_length)) {
        //_pid_info_x.limit = true;
        //_pid_info_y.limit = true;
    }

    limit_length(_integrator, _kimax);
}

FVector2f UPID_2D::get_p() const
{
    return _error * _kp;
}

const FVector2f& UPID_2D::get_i() const
{
    return _integrator;
}

FVector2f UPID_2D::get_d() const
{
    return _derivative * _kd;
}

FVector2f UPID_2D::get_ff()
{
    /*_pid_info_x.FF = _target.x * _kff;
    _pid_info_y.FF = _target.y * _kff;*/
    return _target * _kff;
}

void UPID_2D::reset_I()
{
    _integrator.Zero(); 
}

// get the target filter alpha
float UPID_2D::get_filt_E_alpha(float dt) const
{
    return calc_lowpass_alpha_dt(dt, _filt_E_hz);
}

// get the derivative filter alpha
float UPID_2D::get_filt_D_alpha(float dt) const
{
    return calc_lowpass_alpha_dt(dt, _filt_D_hz);
}

void UPID_2D::set_integrator(const FVector2f& target, const FVector2f& measurement, const FVector2f& i)
{
    set_integrator(target - measurement, i);
}

void UPID_2D::set_integrator(const FVector2f& error, const FVector2f& i)
{
    set_integrator(i - error * _kp);
}

void UPID_2D::set_integrator(const FVector2f& i)
{
    _integrator = i;
    limit_length(_integrator, _kimax);
}

