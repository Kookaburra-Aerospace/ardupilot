#include "Plane.h"

#include "RC_Channel.h"

// defining these two macros and including the RC_Channels_VarInfo
// header defines the parameter information common to all vehicle
// types
#define RC_CHANNELS_SUBCLASS RC_Channels_Plane
#define RC_CHANNEL_SUBCLASS RC_Channel_Plane

#include <RC_Channel/RC_Channels_VarInfo.h>

// note that this callback is not presently used on Plane:
int8_t RC_Channels_Plane::flight_mode_channel_number() const
{
    return plane.g.flight_mode_channel.get();
}

bool RC_Channels_Plane::has_valid_input() const
{
    if (plane.failsafe.rc_failsafe) {
        return false;
    }
    if (plane.failsafe.throttle_counter != 0) {
        return false;
    }
    return true;
}

void RC_Channel_Plane::do_aux_function_change_mode(const Mode::Number number,
                                                   const aux_switch_pos_t ch_flag)
{
    switch(ch_flag) {
    case HIGH: {
        // engage mode (if not possible we remain in current flight mode)
        const bool success = plane.set_mode_by_number(number, MODE_REASON_TX_COMMAND);
        if (plane.control_mode != &plane.mode_initializing) {
            if (success) {
                AP_Notify::events.user_mode_change = 1;
            } else {
                AP_Notify::events.user_mode_change_failed = 1;
            }
        }
        break;
    }
    default:
        // return to flight mode switch's flight mode if we are currently
        // in this mode
        if (plane.control_mode->mode_number() == number) {
// TODO:           rc().reset_mode_switch();
            plane.reset_control_switch();
        }
    }
}

void RC_Channel_Plane::init_aux_function(const RC_Channel::aux_func_t ch_option,
                                         const RC_Channel::aux_switch_pos_t ch_flag)
{
    switch(ch_option) {
    // the following functions do not need to be initialised:
    case AUX_FUNC::ARMDISARM:
    case AUX_FUNC::INVERTED:
    case AUX_FUNC::RTL:
        break;

    case AUX_FUNC::REVERSE_THROTTLE:
        plane.have_reverse_throttle_rc_option = true;
        // setup input throttle as a range. This is needed as init_aux_function is called
        // after set_control_channels()
        if (plane.channel_throttle) {
            plane.channel_throttle->set_range(100);
        }
        // note that we don't call do_aux_function() here as we don't
        // want to startup with reverse thrust
        break;

    case AUX_FUNC::PRECISION_LOITER:
        do_aux_function(ch_option, ch_flag);
        break;

    default:
        // handle in parent class
        RC_Channel::init_aux_function(ch_option, ch_flag);
        break;
}
}

// do_aux_function - implement the function invoked by auxillary switches
void RC_Channel_Plane::do_aux_function(const aux_func_t ch_option, const aux_switch_pos_t ch_flag)
{
    switch(ch_option) {
    case AUX_FUNC::INVERTED:
        plane.inverted_flight = (ch_flag == HIGH);
        break;

    case AUX_FUNC::REVERSE_THROTTLE:
        plane.reversed_throttle = (ch_flag == HIGH);
        gcs().send_text(MAV_SEVERITY_INFO, "RevThrottle: %s", plane.reversed_throttle?"ENABLE":"DISABLE");
        break;

    case AUX_FUNC::AUTO:
        do_aux_function_change_mode(Mode::Number::AUTO, ch_flag);
        break;

    case AUX_FUNC::CIRCLE:
        do_aux_function_change_mode(Mode::Number::CIRCLE, ch_flag);
        break;

    case AUX_FUNC::GUIDED:
        do_aux_function_change_mode(Mode::Number::GUIDED, ch_flag);
        break;

    case AUX_FUNC::MANUAL:
        do_aux_function_change_mode(Mode::Number::MANUAL, ch_flag);
        break;

    case AUX_FUNC::RTL:
        do_aux_function_change_mode(Mode::Number::RTL, ch_flag);

    case AUX_FUNC::PRECISION_LOITER:
#if PRECISION_LANDING == ENABLED
        switch (ch_flag) {
        case HIGH:
            plane.quadplane.set_precision_loiter_enabled(true);
            break;
        case MIDDLE:
            // nothing
            break;
        case LOW:
            plane.quadplane.set_precision_loiter_enabled(false);
            break;
        }
#endif
        break;

    default:
        RC_Channel::do_aux_function(ch_option, ch_flag);
        break;
    }
}
