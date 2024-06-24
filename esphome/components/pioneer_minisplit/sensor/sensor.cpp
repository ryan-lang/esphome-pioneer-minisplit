#include "sensor.h"

namespace esphome
{
    namespace pioneer_minisplit
    {
        void PioneerMinisplitSensor::setup()
        {
            // listen for device state changes
            this->parent_->register_listener([this](AcState *state)
                                             {
                // HEALTH state change
                if (this->purpose_ == HEALTH)
                {
                    if (this->get_state() != state->get(AcState::AC_HEALTH))
                    {
                        this->publish_state(state->get(AcState::AC_HEALTH));
                    }
                }

                // Tracking AC_UNKNOWN_1 change
                if (this->purpose_ == UNKNOWN_1)
                {
                    if (this->get_state() != state->get(AcState::AC_UNKNOWN_1))
                    {
                        this->publish_state(state->get(AcState::AC_UNKNOWN_1));
                    }
                }

                // Tracking INTERNAL FAN SPEED change
                if (this->purpose_ == FAN_SPEED_ACTUAL)
                {
                    if (this->get_state() != state->get(AcState::AC_FAN_SPEED_ACTUAL))
                    {
                        this->publish_state(state->get(AcState::AC_FAN_SPEED_ACTUAL));
                    }
                }

                // Tracking PIPE OUT TEMP change
                if (this->purpose_ == TEMP_PIPE_OUT)
                {
                    if (this->get_state() != state->get(AcState::AC_TEMP_PIPE_OUT))
                    {
                        this->publish_state(state->get(AcState::AC_TEMP_PIPE_OUT));
                    }
                }

                // Tracking PIPE IN TEMP change
                if (this->purpose_ == TEMP_PIPE_IN)
                {
                    if (this->get_state() != state->get(AcState::AC_TEMP_PIPE_IN))
                    {
                        this->publish_state(state->get(AcState::AC_TEMP_PIPE_IN));
                    }
                }

                // Tracking COMPRESSOR 1 change
                if (this->purpose_ == UNKNOWN_2)
                {
                    if (this->get_state() != state->get(AcState::AC_UNKNOWN_2))
                    {
                        this->publish_state(state->get(AcState::AC_UNKNOWN_2));
                    }
                }

                // Tracking COMPRESSOR 2 change
                if (this->purpose_ == UNKNOWN_3)
                {
                    if (this->get_state() != state->get(AcState::AC_UNKNOWN_3))
                    {
                        this->publish_state(state->get(AcState::AC_UNKNOWN_3));
                    }
                }

                // Tracking COMPRESSOR CURRENT change
                if (this->purpose_ == COMPRESSOR_CURRENT)
                {
                    if (this->get_state() != state->get_float(AcState::AC_COMPRESSOR_CURRENT))
                    {
                        this->publish_state(state->get_float(AcState::AC_COMPRESSOR_CURRENT));
                    }
                }

                // Tracking FAULT
                if (this->purpose_ == FAULT)
                {
                    if (this->get_state() != state->get(AcState::AC_FAULT))
                    {
                        this->publish_state(state->get(AcState::AC_FAULT));
                    }
                }

                // Tracking SUPPLY VOLTAGE
                if (this->purpose_ == SUPPLY_VOLTAGE)
                {
                    if (this->get_state() != state->get(AcState::AC_SUPPLY_VOLTAGE))
                    {
                        this->publish_state(state->get(AcState::AC_SUPPLY_VOLTAGE));
                    }
                }

                // Tracking MOTOR
                if (this->purpose_ == MOTOR)
                {
                    if (this->get_state() != state->get(AcState::AC_MOTOR))
                    {
                        this->publish_state(state->get(AcState::AC_MOTOR));
                    }
                }

                // Tracking clean filter
                if (this->purpose_ == CLEAN_FILTER)
                {
                    if (this->get_state() != state->get(AcState::AC_CLEAN_FILTER))
                    {
                        this->publish_state(state->get(AcState::AC_CLEAN_FILTER));
                    }
                }

                // Tracking SWING V POS
                if (this->purpose_ == SWING_V_POS)
                {
                    if (this->get_state() != state->get(AcState::AC_SWING_V_POS))
                    {
                        this->publish_state(state->get(AcState::AC_SWING_V_POS));
                    }
                }

                // Tracking SWING H POS
                if (this->purpose_ == SWING_H_POS)
                {
                    if (this->get_state() != state->get(AcState::AC_SWING_H_POS))
                    {
                        this->publish_state(state->get(AcState::AC_SWING_H_POS));
                    }
                } });
        }
    }
}