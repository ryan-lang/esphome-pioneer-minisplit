#include "switch.h"
#include "esphome/core/log.h"

namespace esphome
{
    namespace pioneer_minisplit
    {

        // state change from parent, update state
        void PioneerMinisplitSwitch::setup()
        {
            // listen for device state changes
            this->parent_->register_listener([this](AcState *state)
                                             {
                if (this->purpose_ == PioneerMinisplitSwitchPurpose::SWITCH_DISPLAY)
                {
                    if (this->state != state->get(AcState::AC_DISPLAY))
                    {
                        this->publish_state(state->get(AcState::AC_DISPLAY));
                    }
                }

                if (this->purpose_ == PioneerMinisplitSwitchPurpose::SWITCH_BEEP)
                {
                    if (this->state != state->get(AcState::AC_BEEP))
                    {
                        this->publish_state(state->get(AcState::AC_BEEP));
                    }
                } });
        }

        // state change from esphome, send to device
        void PioneerMinisplitSwitch::write_state(bool state)
        {
            // clone the state and start building a pending state
            this->parent_->prepare_state_pending();

            if (this->purpose_ == PioneerMinisplitSwitchPurpose::SWITCH_DISPLAY)
            {
                // TODO: block change if power is OFF
                this->parent_->ac_state_pending->set(AcState::AC_DISPLAY, state);
            }

            if (this->purpose_ == PioneerMinisplitSwitchPurpose::SWITCH_BEEP)
            {
                this->parent_->ac_state_pending->set(AcState::AC_BEEP, state);
            }
        }
    }
}
