#include "binary_sensor.h"

namespace esphome
{
    namespace pioneer_minisplit
    {
        void PioneerMinisplitBinarySensor::setup()
        {

            // listen for device state changes
            this->parent_->register_listener([this](AcState *state)
                                             {

            // Tracking SLEEP state change
            // generally in 0x88 (sleep off?)
            // sleep mode 0x89 (sleep on)
            // after 10min 0x08 (deep sleep?)
            if (this->purpose_ == DEEP_SLEEP)
            {
                if (this->state != (state->get(AcState::AC_SLEEP_EXT) == 0x08))
                {
                    this->publish_state((state->get(AcState::AC_SLEEP_EXT) == 0x08));
                }
            }
            if (this->purpose_ == SLEEP)
            {
                if (this->state != state->get(AcState::AC_SLEEP))
                {
                    this->publish_state(state->get(AcState::AC_SLEEP));
                }
            }

            // tracking ECO
            if (this->purpose_ == ECO)
            {
                if (this->state != state->get(AcState::AC_ECO))
                {
                    this->publish_state(state->get(AcState::AC_ECO));
                }
            }

            // tracking TURBO
            if (this->purpose_ == TURBO)
            {
                if (this->state != state->get(AcState::AC_TURBO))
                {
                    this->publish_state(state->get(AcState::AC_TURBO));
                }
            } });
        }
    }
}
