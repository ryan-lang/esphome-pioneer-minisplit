#pragma once

#include "esphome/components/pioneer_minisplit/pioneer_minisplit.h"
#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"

namespace esphome
{

    namespace pioneer_minisplit
    {

        class PioneerMinisplitClimate : public Component, public climate::Climate
        {
        public:
            PioneerMinisplitClimate(PioneerMinisplit *parent) : parent_(parent) {}

        protected:
            void setup() override;
            void control(const climate::ClimateCall &call) override;
            climate::ClimateTraits traits() override;

            void switch_to_action_(climate::ClimateAction action);

            climate::ClimateAction compute_action_();

            /// Check if cooling/fanning/heating actions are required; returns true if so
            bool cooling_required_();
            bool heating_required_();

            climate::ClimateMode ac_mode_to_esphome_mode_(uint8_t ac_mode, bool ac_power);
            void esphome_mode_to_ac_mode_(climate::ClimateMode mode, uint8_t &ac_mode, bool &ac_power);
            climate::ClimateFanMode ac_fan_mode_to_esphome_fan_mode_(uint8_t ac_fan);
            uint8_t esphome_fan_mode_to_ac_fan_mode_(climate::ClimateFanMode fan_mode);
            climate::ClimateSwingMode ac_swing_mode_to_esphome_swing_mode_(uint8_t ac_h_swing, uint8_t ac_v_swing);
            void esphome_swing_mode_to_ac_swing_mode_(climate::ClimateSwingMode swing_mode, uint8_t &ac_h_swing, uint8_t &ac_v_swing);
            climate::ClimateAction ac_action_to_esphome_action_(uint8_t ac_action);
            climate::ClimatePreset ac_preset_to_esphome_preset_(bool ac_eco, bool ac_turbo, bool ac_sleep);
            void esphome_preset_to_ac_preset_(climate::ClimatePreset preset, bool &ac_eco, bool &ac_turbo, bool &ac_sleep);

            PioneerMinisplit *parent_;

            /// Hysteresis values used for computing climate actions
            float cooling_deadband_{0};
            float cooling_overrun_{0};
            float heating_deadband_{0};
            float heating_overrun_{0};

            bool use_advanced_heat_cool_{true};
        };
    }
}