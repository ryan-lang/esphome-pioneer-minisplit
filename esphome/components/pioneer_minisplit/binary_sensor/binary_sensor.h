#pragma once

#include "esphome/components/pioneer_minisplit/pioneer_minisplit.h"
#include "esphome/core/component.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

namespace esphome
{
    namespace pioneer_minisplit
    {
        enum PioneerMinisplitBinarySensorPurpose : uint8_t
        {
            DEEP_SLEEP,
            SLEEP,
            ECO,
            TURBO,
        };

        class PioneerMinisplitBinarySensor : public Component, public binary_sensor::BinarySensor
        {
        public:
            PioneerMinisplitBinarySensor(PioneerMinisplit *parent, PioneerMinisplitBinarySensorPurpose purpose) : parent_(parent), purpose_(purpose) {}

        protected:
            void setup() override;
            PioneerMinisplit *parent_;
            PioneerMinisplitBinarySensorPurpose purpose_;
        };
    }
}