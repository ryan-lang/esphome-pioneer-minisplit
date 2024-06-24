#pragma once

#include "esphome/core/component.h"
#include "esphome/components/pioneer_minisplit/pioneer_minisplit.h"
#include "esphome/components/switch/switch.h"

namespace esphome
{

    namespace pioneer_minisplit
    {

        enum PioneerMinisplitSwitchPurpose : uint8_t
        {
            SWITCH_DISPLAY,
            SWITCH_BEEP
        };

        class PioneerMinisplitSwitch : public Component, public switch_::Switch
        {
        public:
            PioneerMinisplitSwitch(PioneerMinisplit *parent, PioneerMinisplitSwitchPurpose purpose) : parent_(parent), purpose_(purpose) {}

        protected:
            void setup() override;
            void write_state(bool state) override;
            PioneerMinisplit *parent_;
            PioneerMinisplitSwitchPurpose purpose_;
        };

    }

}