#include "switch.h"

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
                if (this->purpose_ == DISPLAY)
                {
                    if (this->state != state->get(AcState::AC_DISPLAY))
                    {
                        this->publish_state(state->get(AcState::AC_DISPLAY));
                    }
                }

                if (this->purpose_ == BEEP)
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
            ESP_LOGD("pioneer", "SWITCH WRITE STATE: %s, STATE: %d", switch_name.c_str(), state);

            // clone the state and start building a pending state
            this->parent_->prepare_state_pending();

            if (this->purpose_ == DISPLAY)
            {
                // TODO: block change if power is OFF
                this->parent_->ac_state_pending->set(AcState::AC_DISPLAY, state);
            }

            if (this->purpose_ == BEEP)
            {
                this->parent_->ac_state_pending->set(AcState::AC_BEEP, state);
            }
        }
    }
}
