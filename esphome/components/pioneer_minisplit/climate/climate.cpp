#include "climate.h"

namespace esphome
{
    namespace pioneer_minisplit
    {

        void PioneerMinisplitClimate::setup()
        {

            // listen for device state changes
            this->parent_->register_listener([this](AcState *state)
                                             {
                                                 bool climate_changed = false;

                                                // sync MODE
                                                climate::ClimateMode new_mode = ac_mode_to_esphome_mode_(state->get(AcState::AC_MODE), state->get(AcState::AC_POWER));
                                                if (this->mode != new_mode){
                                                    if (this->use_advanced_heat_cool_)
                                                    {
                                                        // if we are use_advanced_heat_cool_ and in CLIMATE_MODE_HEAT_COOL mode,
                                                        // mode changes will be transparent because we will be switching modes
                                                        // based on the current temperature

                                                        ESP_LOGD("climate", "Ignoring mode change to %d", new_mode);

                                                    }else{
                                                    
                                                        this->mode = new_mode;
                                                        climate_changed = true;
                                                    }
                                                }
                                                
                                                // STMP change
                                                if (this->use_advanced_heat_cool_  && this->mode == climate::CLIMATE_MODE_HEAT_COOL)
                                                {
                                                    // if we are use_advanced_heat_cool_ in CLIMATE_MODE_HEAT_COOL, device's target temp
                                                    // is ignored, because we are tracking a different set of temps internally

                                                    ESP_LOGD("climate", "Ignoring target temperature change to %f", state->get_float(AcState::AC_STMP));
                                                
                                                }else if (this->use_advanced_heat_cool_){
                                                    
                                                    // if we are use_advanced_heat_cool_ and not in CLIMATE_MODE_HEAT_COOL,
                                                    // target temp goes into both high and low slots

                                                    if (!std::isnan(state->get(AcState::AC_STMP)) 
                                                    && (
                                                        (std::abs(this->target_temperature_high - state->get(AcState::AC_STMP)) > 0.01f || std::isnan(this->target_temperature_high)) ||
                                                        (std::abs(this->target_temperature_low - state->get(AcState::AC_STMP)) > 0.01f || std::isnan(this->target_temperature_low))
                                                    ))   
                                                    {
                                                        this->target_temperature_high = state->get(AcState::AC_STMP);
                                                        this->target_temperature_low = state->get(AcState::AC_STMP);
                                                        ESP_LOGD("climate", "Setting target temperature high/low to %f", this->target_temperature_high);
                                                        climate_changed = true;
                                                    }

                                                }else{
                                                    // otherwise target temp goes into single point target temp
                                                    if (!std::isnan(state->get(AcState::AC_STMP)) && (std::abs(this->target_temperature - state->get(AcState::AC_STMP)) > 0.01f || std::isnan(this->target_temperature)))
                                                    {
                                                        this->target_temperature = state->get(AcState::AC_STMP);
                                                        ESP_LOGD("climate", "Setting target temperature to %f", this->target_temperature);
                                                        climate_changed = true;
                                                    }
                                                }

                                                 // Tracking FAN state change
                                                 climate::ClimateFanMode new_fan_mode = ac_fan_mode_to_esphome_fan_mode_(state->get(AcState::AC_FAN));
                                                 if (this->fan_mode != new_fan_mode)
                                                 {
                                                     this->fan_mode = new_fan_mode;
                                                     climate_changed = true;
                                                 }

                                                 // Tracking SWING state change
                                                 climate::ClimateSwingMode new_swing_mode = ac_swing_mode_to_esphome_swing_mode_(state->get(AcState::AC_SWING_H), state->get(AcState::AC_SWING_V));
                                                 if (this->swing_mode != new_swing_mode)
                                                 {
                                                     this->swing_mode = new_swing_mode;
                                                     climate_changed = true;
                                                 }

                                                 // Tracking ACTION change
                                                 climate::ClimateAction new_action = ac_action_to_esphome_action_(state->get(AcState::AC_ACTION));
                                                 if (this->action != new_action)
                                                 {
                                                     this->action = new_action;
                                                     climate_changed = true;
                                                 }

                                                 // tracking PRESET
                                                 climate::ClimatePreset new_preset = ac_preset_to_esphome_preset_(state->get(AcState::AC_ECO), state->get(AcState::AC_TURBO), state->get(AcState::AC_SLEEP));
                                                 if (this->preset != new_preset)
                                                 {
                                                     this->preset = new_preset;
                                                     climate_changed = true;
                                                 }

                                                 // Tracking TEMP change
                                                 if (!std::isnan(state->get_float(AcState::AC_CUR_TEMP)) && (std::abs(this->current_temperature - state->get_float(AcState::AC_CUR_TEMP)) > 0.01f || std::isnan(this->current_temperature)))
                                                 { 
                                                    // Assume a change if difference is more than 0.01
                                                    this->current_temperature = state->get_float(AcState::AC_CUR_TEMP);

                                                    // required action may have changed, recompute, refresh, we'll publish_state() later
                                                    this->switch_to_action_(this->compute_action_());
                                                 
                                                    climate_changed = true;
                                                 }

                                                 if (climate_changed)
                                                 {
                                                     this->publish_state();
                                                 } });

            // restore all climate data, if possible
            auto restore = this->restore_state_();
            if (restore.has_value())
            {
                restore->to_call(this).perform();
            }
            this->switch_to_action_(this->compute_action_());
            this->publish_state();
        }

        // called when esphome issues some device command;
        // updates the in-memory (pending) state with the new desired state
        // does not explicitly send the command to the device, but preps for delayed send
        void PioneerMinisplitClimate::control(const climate::ClimateCall &call)
        {
            // clone the state and start building a pending state
            this->parent_->prepare_state_pending();
            bool state_changed = false;

            // MODE change
            if (call.get_mode().has_value())
            {
                uint8_t new_ac_mode;
                bool new_ac_power;
                esphome_mode_to_ac_mode_(*call.get_mode(), new_ac_mode, new_ac_power);

                this->parent_->ac_state_pending->set(AcState::AC_MODE, new_ac_mode);
                this->parent_->ac_state_pending->set(AcState::AC_POWER, new_ac_power);

                // set mode optimistically
                this->mode = *call.get_mode();
                state_changed = true;
            }

            // PRESET change
            if (call.get_preset().has_value())
            {
                bool new_ac_eco;
                bool new_ac_turbo;
                bool new_ac_sleep;
                esphome_preset_to_ac_preset_(*call.get_preset(), new_ac_eco, new_ac_turbo, new_ac_sleep);

                this->parent_->ac_state_pending->set(AcState::AC_ECO, new_ac_eco);
                this->parent_->ac_state_pending->set(AcState::AC_TURBO, new_ac_turbo);
                this->parent_->ac_state_pending->set(AcState::AC_SLEEP, new_ac_sleep);
            }

            // TARG TEMP change
            if (call.get_target_temperature().has_value())
            {
                this->parent_->ac_state_pending->set(AcState::AC_STMP, *call.get_target_temperature());
            }

            // set TARGET TEMP high/low
            if (this->use_advanced_heat_cool_)
            {
                if (this->mode == climate::CLIMATE_MODE_HEAT_COOL)
                {
                    if (call.get_target_temperature_low().has_value())
                    {
                        this->target_temperature_low = *call.get_target_temperature_low();
                        state_changed = true;
                    }

                    if (call.get_target_temperature_high().has_value())
                    {
                        this->target_temperature_high = *call.get_target_temperature_high();
                        state_changed = true;
                    }
                }
                else
                {
                    // if we are not in CLIMATE_MODE_HEAT_COOL, use whatever is most recently set as the target temp
                    if (call.get_target_temperature_low().has_value())
                    {
                        this->parent_->ac_state_pending->set(AcState::AC_STMP, *call.get_target_temperature_low());
                    }

                    if (call.get_target_temperature_high().has_value())
                    {
                        this->parent_->ac_state_pending->set(AcState::AC_STMP, *call.get_target_temperature_high());
                    }
                }
            }

            // FAN change
            if (call.get_fan_mode().has_value())
            {
                uint8_t new_ac_fan = esphome_fan_mode_to_ac_fan_mode_(*call.get_fan_mode());
                this->parent_->ac_state_pending->set(AcState::AC_FAN, new_ac_fan);
            }

            // SWING change
            if (call.get_swing_mode().has_value())
            {
                u_int8_t new_swing_h;
                u_int8_t new_swing_v;
                esphome_swing_mode_to_ac_swing_mode_(*call.get_swing_mode(), new_swing_h, new_swing_v);

                this->parent_->ac_state_pending->set(AcState::AC_SWING_H, new_swing_h);
                this->parent_->ac_state_pending->set(AcState::AC_SWING_V, new_swing_v);
            }

            if (state_changed)
            {
                this->publish_state();
            }
        }

        climate::ClimateAction PioneerMinisplitClimate::compute_action_()
        {
            if (this->cooling_required_())
            {
                return climate::CLIMATE_ACTION_COOLING;
            }
            else if (this->heating_required_())
            {
                return climate::CLIMATE_ACTION_HEATING;
            }
            else
            {
                return climate::CLIMATE_ACTION_IDLE;
            }
        }

        void PioneerMinisplitClimate::switch_to_action_(climate::ClimateAction action)
        {

            // clone the state and start building a pending state
            this->parent_->prepare_state_pending();

            // ACTION change
            if (action == climate::CLIMATE_ACTION_COOLING)
            {
                // TURN ON & COOL to target_temperature_high
                ESP_LOGD("climate", "Switching to COOLING");

                uint8_t new_ac_mode;
                bool new_ac_power;
                esphome_mode_to_ac_mode_(climate::CLIMATE_MODE_COOL, new_ac_mode, new_ac_power);
                this->parent_->ac_state_pending->set(AcState::AC_MODE, new_ac_mode);
                this->parent_->ac_state_pending->set(AcState::AC_POWER, new_ac_power);
                this->parent_->ac_state_pending->set(AcState::AC_STMP, this->target_temperature_high);
            }
            else if (action == climate::CLIMATE_ACTION_HEATING)
            {
                // TURN ON & HEAT to target_temperature_low
                ESP_LOGD("climate", "Switching to HEATING");
                uint8_t new_ac_mode;
                bool new_ac_power;
                esphome_mode_to_ac_mode_(climate::CLIMATE_MODE_HEAT, new_ac_mode, new_ac_power);
                this->parent_->ac_state_pending->set(AcState::AC_MODE, new_ac_mode);
                this->parent_->ac_state_pending->set(AcState::AC_POWER, new_ac_power);
                this->parent_->ac_state_pending->set(AcState::AC_STMP, this->target_temperature_low);
            }
            else if (action == climate::CLIMATE_ACTION_IDLE)
            {
                // TURN OFF
                ESP_LOGD("climate", "Switching to IDLE");
                uint8_t new_ac_mode;
                bool new_ac_power;
                esphome_mode_to_ac_mode_(climate::CLIMATE_MODE_OFF, new_ac_mode, new_ac_power);
                this->parent_->ac_state_pending->set(AcState::AC_MODE, new_ac_mode);
                this->parent_->ac_state_pending->set(AcState::AC_POWER, new_ac_power);
            }
        }

        bool PioneerMinisplitClimate::cooling_required_()
        {
            auto temperature = this->target_temperature_high;

            if (this->current_temperature > temperature + this->cooling_deadband_)
            {
                // if the current temperature exceeds the target + deadband, cooling is required
                return true;
            }
            else if (this->current_temperature < temperature - this->cooling_overrun_)
            {
                // if the current temperature is less than the target - overrun, cooling should stop
                return false;
            }
            else
            {
                // if we get here, the current temperature is between target + deadband and target - overrun,
                //  so the action should not change unless it conflicts with the current mode
                return (this->action == climate::CLIMATE_ACTION_COOLING) &&
                       ((this->mode == climate::CLIMATE_MODE_HEAT_COOL) || (this->mode == climate::CLIMATE_MODE_COOL));
            }
        }

        bool PioneerMinisplitClimate::heating_required_()
        {
            auto temperature = this->target_temperature_low;

            if (this->current_temperature < temperature - this->heating_deadband_)
            {
                // if the current temperature is below the target - deadband, heating is required
                return true;
            }
            else if (this->current_temperature > temperature + this->heating_overrun_)
            {
                // if the current temperature is above the target + overrun, heating should stop
                return false;
            }
            else
            {
                // if we get here, the current temperature is between target - deadband and target + overrun,
                //  so the action should not change unless it conflicts with the current mode
                return (this->action == climate::CLIMATE_ACTION_HEATING) &&
                       ((this->mode == climate::CLIMATE_MODE_HEAT_COOL) || (this->mode == climate::CLIMATE_MODE_HEAT));
            }
        }

        // define the supported device traits
        climate::ClimateTraits PioneerMinisplitClimate::traits()
        {
            auto traits = climate::ClimateTraits();
            traits.set_supports_current_temperature(true);
            traits.set_supports_action(true);
            traits.set_supported_modes({climate::CLIMATE_MODE_OFF,
                                        climate::CLIMATE_MODE_COOL,
                                        climate::CLIMATE_MODE_FAN_ONLY,
                                        climate::CLIMATE_MODE_DRY,
                                        climate::CLIMATE_MODE_HEAT,
                                        climate::CLIMATE_MODE_HEAT_COOL,
                                        climate::CLIMATE_MODE_AUTO});
            traits.set_supported_fan_modes({climate::CLIMATE_FAN_AUTO,
                                            climate::CLIMATE_FAN_LOW,
                                            climate::CLIMATE_FAN_MEDIUM,
                                            climate::CLIMATE_FAN_HIGH});
            traits.set_supported_swing_modes({climate::CLIMATE_SWING_OFF,
                                              climate::CLIMATE_SWING_BOTH,
                                              climate::CLIMATE_SWING_VERTICAL,
                                              climate::CLIMATE_SWING_HORIZONTAL});
            traits.set_supported_presets({climate::CLIMATE_PRESET_NONE,
                                          climate::CLIMATE_PRESET_BOOST,
                                          climate::CLIMATE_PRESET_ECO,
                                          climate::CLIMATE_PRESET_SLEEP});
            traits.set_visual_min_temperature(16);
            traits.set_visual_max_temperature(31);

            if (this->use_advanced_heat_cool_)
            {
                traits.set_supports_two_point_target_temperature(true);
            }

            return traits;
        }

        // convert ac_mode and ac_power to valid ClimateMode
        // ac_mode: 0=cool 1=fan  2=dry 3=heat 4=auto
        climate::ClimateMode PioneerMinisplitClimate::ac_mode_to_esphome_mode_(uint8_t ac_mode, bool ac_power)
        {
            if (!ac_power)
            {
                return climate::CLIMATE_MODE_OFF;
            }
            else if (ac_mode == 0)
            {
                return climate::CLIMATE_MODE_COOL;
            }
            else if (ac_mode == 1)
            {
                return climate::CLIMATE_MODE_FAN_ONLY;
            }
            else if (ac_mode == 2)
            {
                return climate::CLIMATE_MODE_DRY;
            }
            else if (ac_mode == 3)
            {
                return climate::CLIMATE_MODE_HEAT;
            }
            else if (ac_mode == 4)
            {
                return climate::CLIMATE_MODE_HEAT_COOL;
            }
            else
            {
                return climate::CLIMATE_MODE_HEAT_COOL;
            }
        }

        // convert ClimateMode to ac_mode and ac_power
        void PioneerMinisplitClimate::esphome_mode_to_ac_mode_(climate::ClimateMode mode, uint8_t &ac_mode, bool &ac_power)
        {
            if (mode == climate::CLIMATE_MODE_OFF)
            {
                ac_mode = 0;
                ac_power = false;
            }
            else if (mode == climate::CLIMATE_MODE_COOL)
            {
                ac_mode = 0;
                ac_power = true;
            }
            else if (mode == climate::CLIMATE_MODE_FAN_ONLY)
            {
                ac_mode = 1;
                ac_power = true;
            }
            else if (mode == climate::CLIMATE_MODE_DRY)
            {
                ac_mode = 2;
                ac_power = true;
            }
            else if (mode == climate::CLIMATE_MODE_HEAT)
            {
                ac_mode = 3;
                ac_power = true;
            }
            else if (mode == climate::CLIMATE_MODE_HEAT_COOL)
            {
                ac_mode = 4;
                ac_power = true;
            }
            else
            {
                ac_mode = 0;
                ac_power = false;
            }
        }

        // 0=auto 1=low 2=med 3=high
        climate::ClimateFanMode PioneerMinisplitClimate::ac_fan_mode_to_esphome_fan_mode_(uint8_t ac_fan)
        {
            if (ac_fan == 0)
            {
                return climate::CLIMATE_FAN_AUTO;
            }
            else if (ac_fan == 1)
            {
                return climate::CLIMATE_FAN_LOW;
            }
            else if (ac_fan == 2)
            {
                return climate::CLIMATE_FAN_MEDIUM;
            }
            else if (ac_fan == 3)
            {
                return climate::CLIMATE_FAN_HIGH;
            }
            else
            {
                return climate::CLIMATE_FAN_AUTO;
            }
        }

        uint8_t PioneerMinisplitClimate::esphome_fan_mode_to_ac_fan_mode_(climate::ClimateFanMode fan_mode)
        {
            if (fan_mode == climate::CLIMATE_FAN_AUTO)
            {
                return 0;
            }
            else if (fan_mode == climate::CLIMATE_FAN_LOW)
            {
                return 1;
            }
            else if (fan_mode == climate::CLIMATE_FAN_MEDIUM)
            {
                return 2;
            }
            else if (fan_mode == climate::CLIMATE_FAN_HIGH)
            {
                return 3;
            }
            else
            {
                return 0;
            }
        }

        climate::ClimateSwingMode PioneerMinisplitClimate::ac_swing_mode_to_esphome_swing_mode_(uint8_t ac_h_swing, uint8_t ac_v_swing)
        {
            if (ac_h_swing && ac_v_swing)
            {
                return climate::CLIMATE_SWING_BOTH;
            }
            else if (ac_h_swing)
            {
                return climate::CLIMATE_SWING_HORIZONTAL;
            }
            else if (ac_v_swing)
            {
                return climate::CLIMATE_SWING_VERTICAL;
            }
            else
            {
                return climate::CLIMATE_SWING_OFF;
            }
        }

        void PioneerMinisplitClimate::esphome_swing_mode_to_ac_swing_mode_(climate::ClimateSwingMode swing_mode, uint8_t &ac_h_swing, uint8_t &ac_v_swing)
        {
            if (swing_mode == climate::CLIMATE_SWING_BOTH)
            {
                ac_h_swing = 1;
                ac_v_swing = 1;
            }
            else if (swing_mode == climate::CLIMATE_SWING_HORIZONTAL)
            {
                ac_h_swing = 1;
                ac_v_swing = 0;
            }
            else if (swing_mode == climate::CLIMATE_SWING_VERTICAL)
            {
                ac_h_swing = 0;
                ac_v_swing = 1;
            }
            else
            {
                ac_h_swing = 0;
                ac_v_swing = 0;
            }
        }

        climate::ClimateAction PioneerMinisplitClimate::ac_action_to_esphome_action_(uint8_t ac_action)
        {
            if (ac_action == 0x8A) // 138
            {
                return climate::CLIMATE_ACTION_COOLING;
            }
            else if (ac_action == 0xCA) // 202
            {
                return climate::CLIMATE_ACTION_HEATING;
            }
            else if (ac_action == 0x80) // 128
            {
                return climate::CLIMATE_ACTION_IDLE;
            }
            else if (ac_action == 0xC0) // 192 (2 min Compressor Cooldown Period)
            {
                return climate::CLIMATE_ACTION_IDLE;
            }
            else
            {
                return climate::CLIMATE_ACTION_IDLE;
            }
        }

        climate::ClimatePreset PioneerMinisplitClimate::ac_preset_to_esphome_preset_(bool ac_eco, bool ac_turbo, bool ac_sleep)
        {
            if (ac_eco)
            {
                return climate::CLIMATE_PRESET_ECO;
            }
            else if (ac_turbo)
            {
                return climate::CLIMATE_PRESET_BOOST;
            }
            else if (ac_sleep)
            {
                return climate::CLIMATE_PRESET_SLEEP;
            }
            else
            {
                return climate::CLIMATE_PRESET_NONE;
            }
        }

        void PioneerMinisplitClimate::esphome_preset_to_ac_preset_(climate::ClimatePreset preset, bool &ac_eco, bool &ac_turbo, bool &ac_sleep)
        {
            if (preset == climate::CLIMATE_PRESET_ECO)
            {
                ac_eco = true;
                ac_turbo = false;
                ac_sleep = false;
            }
            else if (preset == climate::CLIMATE_PRESET_BOOST)
            {
                ac_eco = false;
                ac_turbo = true;
                ac_sleep = false;
            }
            else if (preset == climate::CLIMATE_PRESET_SLEEP)
            {
                ac_eco = false;
                ac_turbo = false;
                ac_sleep = true;
            }
            else
            {
                ac_eco = false;
                ac_turbo = false;
                ac_sleep = false;
            }
        }
    }
}
