#pragma once

#include "esphome/components/pioneer_minisplit/pioneer_minisplit.h"
#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome
{

    namespace pioneer_minisplit
    {

        enum PioneerMinisplitSensorPurpose : uint8_t
        {
            UNKNOWN_1,
            FAN_SPEED_ACTUAL,
            TEMP_PIPE_OUT,
            TEMP_PIPE_IN,
            UNKNOWN_2,
            UNKNOWN_3,
            COMPRESSOR_CURRENT,
            MOTOR,
            SWING_V_POS,
            SWING_H_POS,
            FAULT,
            CLEAN_FILTER,
            HEALTH,
            SUPPLY_VOLTAGE
        };

        class PioneerMinisplitSensor : public Component, public sensor::Sensor
        {
        public:
            PioneerMinisplitSensor(PioneerMinisplit *parent, PioneerMinisplitSensorPurpose purpose) : parent_(parent), purpose_(purpose) {}
            void setup() override;

        protected:
            PioneerMinisplit *parent_;
            PioneerMinisplitSensorPurpose purpose_;
        };

    }

}