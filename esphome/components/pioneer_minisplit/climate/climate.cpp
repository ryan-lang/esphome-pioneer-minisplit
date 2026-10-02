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
                                                this->mode_internal_ = new_mode;
                                                if (this->use_advanced_heat_cool_){
                                                    // in advanced mode, we only support modes:
                                                    // - CLIMATE_MODE_OFF
                                                    // - CLIMATE_MODE_FAN_ONLY
                                                    // - CLIMATE_MODE_DRY
                                                    // - CLIMATE_MODE_HEAT_COOL
                                                    // CLIMATE_MODE_HEAT_COOL is overloaded to also mean CLIMATE_MODE_COOL & CLIMATE_MODE_HEAT

                                                    if (new_mode == climate::CLIMATE_MODE_COOL || new_mode == climate::CLIMATE_MODE_HEAT)
                                                    {
                                                        new_mode = climate::CLIMATE_MODE_HEAT_COOL;
                                                    }
                                                }
                                                if (this->mode != new_mode){
                                                    this->mode = new_mode;
                                                    climate_changed = true;
                                                }
                                                
                                                // STMP change
                                                if (!std::isnan(state->get(AcState::AC_STMP)))
                                                {
                                                    this->stmp_internal_ = state->get(AcState::AC_STMP);

                                                    if (!this->use_advanced_heat_cool_ && this->target_temperature != state->get(AcState::AC_STMP)){
                                                        // otherwise target temp goes into single point target temp
                                                        this->target_temperature = state->get(AcState::AC_STMP);
                                                        ESP_LOGD("climate", "Setting target temperature to %d", this->target_temperature);
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
                                                 float ac_temp = state->get_float(AcState::AC_CUR_TEMP);
                                                 if (!std::isnan(ac_temp) && (std::abs(this->internal_temp_ - ac_temp) > 0.01f || std::isnan(this->internal_temp_)))
                                                 {
                                                    // Assume a change if difference is more than 0.01
                                                    this->internal_temp_ = ac_temp;

                                                    // The throttle is computed relative to the unit's own reading, so recompute
                                                    // even when a remote sensor is what the loop is actually tracking.
                                                    if (this->use_advanced_heat_cool_ && this->mode == climate::CLIMATE_MODE_HEAT_COOL){
                                                        this->switch_to_action_(this->compute_action_());
                                                    }

                                                    float control_temp = this->control_temperature_();
                                                    if (!std::isnan(control_temp) && (std::abs(this->current_temperature - control_temp) > 0.01f || std::isnan(this->current_temperature)))
                                                    {
                                                        this->current_temperature = control_temp;
                                                        climate_changed = true;
                                                    }
                                                 }

                                                 if (climate_changed)
                                                 {
                                                     this->publish_state();
                                                 } });

            // listen for remote thermometer updates
            if (this->remote_sensor_ != nullptr)
            {
                this->remote_sensor_->add_on_state_callback([this](float state)
                                                            {
                    // A wildly out-of-range reading is more likely a dead sensor reporting
                    // garbage than a real room, and acting on it would drive the unit hard in
                    // the wrong direction. Drop it and let it go stale instead.
                    if (std::isnan(state) || state < 5.0f || state > 40.0f)
                    {
                        ESP_LOGW("climate", "remote temperature %.2f out of range, ignoring", state);
                        return;
                    }

                    this->remote_temp_ = state;
                    this->remote_last_ms_ = millis();
                    this->remote_has_value_ = true;
                    this->evaluate_(); });
            }

            // A remote sensor going quiet is the absence of an event, so nothing above will
            // notice it. Poll the decision so the fallback actually engages.
            this->set_interval("evaluate", 30000, [this]()
                               { this->evaluate_(); });

            // restore all climate data, if possible
            auto restore = this->restore_state_();
            if (restore.has_value())
            {
                restore->to_call(this).perform();
            }
            else
            {
                ESP_LOGD("climate", "No previous state to restore");
            }

            if (this->use_advanced_heat_cool_ && this->mode == climate::CLIMATE_MODE_HEAT_COOL)
            {
                this->switch_to_action_(this->compute_action_());
            }
            this->refresh_diagnostics_();
            this->publish_state();
        }

        // called when esphome issues some device command;
        // updates the in-memory (pending) state with the new desired state
        // does not explicitly send the command to the device, but preps for delayed send
        void PioneerMinisplitClimate::control(const climate::ClimateCall &call)
        {
            bool state_changed = false;

            // MODE change
            if (call.get_mode().has_value())
            {
                // set mode optimistically
                this->mode = *call.get_mode();
                state_changed = true;

                // In advanced heat/cool we own the unit's mode and power: switch_to_action_()
                // below decides whether it should be cooling, heating or off. Queuing the raw
                // HEAT_COOL mode here would put the unit in its own auto mode first and cost an
                // extra command frame (and display flash) to correct.
                if (!(this->use_advanced_heat_cool_ && this->mode == climate::CLIMATE_MODE_HEAT_COOL))
                {
                    uint8_t new_ac_mode;
                    bool new_ac_power;
                    esphome_mode_to_ac_mode_(*call.get_mode(), new_ac_mode, new_ac_power);

                    this->parent_->set_pending_parameter(AcState::AC_MODE, new_ac_mode);
                    this->parent_->set_pending_parameter(AcState::AC_POWER, new_ac_power);
                }
            }

            // PRESET change
            if (call.get_preset().has_value())
            {
                bool new_ac_eco;
                bool new_ac_turbo;
                bool new_ac_sleep;
                esphome_preset_to_ac_preset_(*call.get_preset(), new_ac_eco, new_ac_turbo, new_ac_sleep);

                this->parent_->set_pending_parameter(AcState::AC_ECO, new_ac_eco);
                this->parent_->set_pending_parameter(AcState::AC_TURBO, new_ac_turbo);
                this->parent_->set_pending_parameter(AcState::AC_SLEEP, new_ac_sleep);
            }

            // set TARGET TEMP change
            if (this->use_advanced_heat_cool_)
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
                if (call.get_target_temperature().has_value())
                {
                    this->parent_->set_pending_parameter(AcState::AC_STMP, *call.get_target_temperature());
                }
            }

            // FAN change
            if (call.get_fan_mode().has_value())
            {
                uint8_t new_ac_fan = esphome_fan_mode_to_ac_fan_mode_(*call.get_fan_mode());
                this->parent_->set_pending_parameter(AcState::AC_FAN, new_ac_fan);
            }

            // SWING change
            if (call.get_swing_mode().has_value())
            {
                u_int8_t new_swing_h;
                u_int8_t new_swing_v;
                esphome_swing_mode_to_ac_swing_mode_(*call.get_swing_mode(), new_swing_h, new_swing_v);

                this->parent_->set_pending_parameter(AcState::AC_SWING_H, new_swing_h);
                this->parent_->set_pending_parameter(AcState::AC_SWING_V, new_swing_v);
            }

            if (state_changed)
            {
                if (this->use_advanced_heat_cool_ && this->mode == climate::CLIMATE_MODE_HEAT_COOL)
                {
                    this->switch_to_action_(this->compute_action_());
                }
                if (this->mode == climate::CLIMATE_MODE_OFF)
                {
                    this->target_action_ = climate::CLIMATE_ACTION_IDLE;
                    this->committed_stmp_ = 0;
                    this->last_commanded_stmp_ = 0;
                }
                this->refresh_diagnostics_();
                this->publish_state();
            }
        }

        bool PioneerMinisplitClimate::remote_valid_()
        {
            if (this->remote_sensor_ == nullptr || !this->remote_has_value_)
            {
                return false;
            }
            return (millis() - this->remote_last_ms_) < this->sensor_timeout_;
        }

        float PioneerMinisplitClimate::control_temperature_()
        {
            return this->remote_valid_() ? this->remote_temp_ : this->internal_temp_;
        }

        bool PioneerMinisplitClimate::control_ready_()
        {
            // Never having heard from a configured remote sensor is different from it having
            // gone stale. On the stale path falling back to the unit's own sensor is the
            // intended degraded behaviour; at boot it would mean latching a cooling or heating
            // decision off the reading we do not trust, before the first packet has arrived.
            if (this->remote_sensor_ != nullptr && !this->remote_has_value_)
            {
                return false;
            }
            return !std::isnan(this->control_temperature_());
        }

        void PioneerMinisplitClimate::evaluate_()
        {
            // Advance the slow average of the unit's own sensor. Driven from this timer rather
            // than from each incoming frame so the time constant is predictable: alpha 0.1 on a
            // 30s tick is roughly a 5 minute constant. Runs while idle too, so it is already
            // warm when a cycle starts.
            if (!std::isnan(this->internal_temp_))
            {
                if (std::isnan(this->internal_temp_avg_))
                {
                    this->internal_temp_avg_ = this->internal_temp_;
                }
                else
                {
                    this->internal_temp_avg_ += 0.1f * (this->internal_temp_ - this->internal_temp_avg_);
                }
            }

            bool active = this->remote_valid_();
            if (active != this->remote_active_last_)
            {
                this->remote_active_last_ = active;
                ESP_LOGW("climate", "remote sensor %s - control temperature now from %s sensor",
                         active ? "available" : "stale", active ? "remote" : "internal");

                // Any setpoint committed while we had no remote reading came from the fallback
                // path and was a guess. Drop it so the settle window cannot protect it now that
                // we can do better - otherwise a boot, or a sender outage that recovers
                // mid-cycle, leaves the unit running on that guess for the whole window.
                if (active)
                {
                    this->committed_stmp_ = 0;
                }
            }

            if (this->use_advanced_heat_cool_ && this->mode == climate::CLIMATE_MODE_HEAT_COOL)
            {
                this->switch_to_action_(this->compute_action_());
            }

            this->refresh_diagnostics_();

            float control_temp = this->control_temperature_();
            if (!std::isnan(control_temp) && (std::abs(this->current_temperature - control_temp) > 0.01f || std::isnan(this->current_temperature)))
            {
                this->current_temperature = control_temp;
                this->publish_state();
            }
        }

        void PioneerMinisplitClimate::refresh_diagnostics_()
        {
            if (this->remote_sensor_ == nullptr)
            {
                this->control_source_ = "internal";
            }
            else if (!this->remote_has_value_)
            {
                this->control_source_ = "waiting";
            }
            else if (this->remote_valid_())
            {
                this->control_source_ = "remote";
            }
            else
            {
                this->control_source_ = "internal_fallback";
            }

            if (this->mode == climate::CLIMATE_MODE_OFF)
            {
                this->control_reason_ = "off";
                return;
            }
            if (this->mode != climate::CLIMATE_MODE_HEAT_COOL)
            {
                this->control_reason_ = "manual_mode";
                return;
            }
            if (!this->control_ready_())
            {
                this->control_reason_ = "waiting_for_sensor";
                return;
            }

            float temperature = this->control_temperature_();
            if (std::isnan(temperature))
            {
                this->control_reason_ = "waiting_for_sensor";
            }
            else if (this->target_action_ == climate::CLIMATE_ACTION_COOLING)
            {
                this->control_reason_ = temperature > this->target_temperature_high + this->cooling_deadband_
                                             ? "cooling_demand"
                                             : "cooling_latched";
            }
            else if (this->target_action_ == climate::CLIMATE_ACTION_HEATING)
            {
                this->control_reason_ = temperature < this->target_temperature_low - this->heating_deadband_
                                             ? "heating_demand"
                                             : "heating_latched";
            }
            else if (this->committed_stmp_ != 0 && this->last_commanded_stmp_ != this->stmp_internal_)
            {
                this->control_reason_ = "setpoint_pending";
            }
            else
            {
                this->control_reason_ = "within_hysteresis";
            }
        }

        // The unit's setpoint field is compared against its own return-air sensor, which is
        // the thing we do not trust. So rather than sending it a setpoint and hoping, we send
        // a value offset from what it currently believes the room to be: the inverter
        // modulates on that difference, which turns the field into a capacity request. How
        // much capacity we ask for is driven by the error the remote sensor reports.
        //
        // With no trustworthy remote reading there is nothing to derive a capacity request
        // from, so we hand the field back to the unit's own thermostat and let it behave as
        // it did before any of this - degraded, but never worse than stock.
        uint8_t PioneerMinisplitClimate::compute_setpoint_(bool cooling)
        {
            float target = cooling ? this->target_temperature_high : this->target_temperature_low;

            if (!this->remote_valid_() || std::isnan(this->internal_temp_))
            {
                this->throttle_ = 0;
                this->committed_stmp_ = (uint8_t)clamp((int)lroundf(target), 16, 31);
                return this->committed_stmp_;
            }

            float error = cooling ? (this->remote_temp_ - target) : (target - this->remote_temp_);
            int delta = clamp((int)lroundf(error * this->throttle_gain_), 1, (int)this->max_throttle_);
            this->throttle_ = (uint8_t)delta;

            // Track the raw reading, deliberately, despite it stepping 0.8C between discrete
            // levels. Following it is what holds the commanded error at delta, and that is the
            // whole point: smoothing it instead unpins capacity, measured on hardware as 1.78C
            // of commanded error and 6.2A where tracking gives 1.0C and 2A. The resulting
            // setpoint commands are not churn to be eliminated - they are the controller
            // re-pinning capacity against a noisy sensor. internal_temp_avg_ is still computed,
            // for the diagnostic sensor only.
            float setpoint = cooling ? (this->internal_temp_ - delta) : (this->internal_temp_ + delta);

            if (this->committed_stmp_ != 0)
            {
                // Starting the unit stirs the air across its own sensor, which moved 2C in the
                // first three minutes of a real cycle. That is the fan, not the room, so there
                // is nothing to be learned by following it - hold until it settles.
                if ((millis() - this->action_started_ms_) < this->settle_time_)
                {
                    return this->committed_stmp_;
                }

                // Enough of a band to stop a value sitting exactly on the rounding boundary from
                // flipping the integer setpoint on every recompute, but deliberately narrower
                // than the sensor's 0.8C level gap so that real movement is still followed.
                // A wider band suppresses the commands, which sounds like an improvement and is
                // not: it stops the controller re-pinning capacity. See the note above.
                if (std::fabs(setpoint - (float)this->committed_stmp_) < 0.75f)
                {
                    return this->committed_stmp_;
                }
            }

            this->committed_stmp_ = (uint8_t)clamp((int)lroundf(setpoint), 16, 31);
            return this->committed_stmp_;
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
            if (action != climate::CLIMATE_ACTION_IDLE && action != climate::CLIMATE_ACTION_COOLING && action != climate::CLIMATE_ACTION_HEATING)
            {
                ESP_LOGD("climate", "Invalid action in switch_to_action_ %d", action);
                return;
            }

            // Leave the unit in whatever state it is already in until we have a reading worth
            // acting on. Deciding here and correcting a second later once the first remote
            // packet lands would cost two commands and could briefly stop a running unit.
            if (!this->control_ready_())
            {
                ESP_LOGD("climate", "no trustworthy reading yet - leaving unit untouched");
                return;
            }

            // Entering an active mode starts a new cycle: the committed setpoint from the last
            // one is meaningless now, and the settle window starts here. Must happen before the
            // setpoint is computed below, or the first command of the cycle would hold a stale
            // value instead of deciding fresh.
            if (action != this->target_action_ &&
                (action == climate::CLIMATE_ACTION_COOLING || action == climate::CLIMATE_ACTION_HEATING))
            {
                this->action_started_ms_ = millis();
                this->committed_stmp_ = 0;
            }

            this->target_action_ = action;

            // determine new desired state
            climate::ClimateMode desired_mode;
            uint8_t desired_stmp;
            if (action == climate::CLIMATE_ACTION_COOLING)
            {
                desired_mode = climate::CLIMATE_MODE_COOL;
                desired_stmp = this->compute_setpoint_(true);
            }
            else if (action == climate::CLIMATE_ACTION_HEATING)
            {
                desired_mode = climate::CLIMATE_MODE_HEAT;
                desired_stmp = this->compute_setpoint_(false);
            }
            else
            {
                desired_mode = climate::CLIMATE_MODE_OFF;
                desired_stmp = 0;
                this->throttle_ = 0;
                this->committed_stmp_ = 0;
            }

            this->last_commanded_stmp_ = desired_stmp;

            // determine if changes are occuring and we need to send a device command
            bool is_state_changing = false;
            // mode_internal_ reflects what the unit has acknowledged, which lags the command by
            // some hundreds of milliseconds. Two calls arriving inside that gap - easy now that
            // both a 30s timer and the remote sensor callback can trigger one - would each
            // queue the same command. Suppress a repeat of the same mode briefly, but still
            // allow a genuine retry if the unit never took it.
            bool mode_recently_sent = this->last_mode_command_ms_ != 0 &&
                                      this->last_mode_sent_ == desired_mode &&
                                      (millis() - this->last_mode_command_ms_) < 10000;

            if (this->mode_internal_ != desired_mode && !mode_recently_sent)
            {
                ESP_LOGI("climate", "adv. heat/cool - switching to mode %s from cur. internal mode %s", climate::climate_mode_to_string(desired_mode), climate::climate_mode_to_string(this->mode_internal_));
                this->last_mode_sent_ = desired_mode;
                this->last_mode_command_ms_ = millis();
                // clone the state and start building a pending state
                // No need to prepare state anymore - just set pending parameters

                uint8_t new_ac_mode;
                bool new_ac_power;
                esphome_mode_to_ac_mode_(desired_mode, new_ac_mode, new_ac_power);
                this->parent_->set_pending_parameter(AcState::AC_MODE, new_ac_mode);
                this->parent_->set_pending_parameter(AcState::AC_POWER, new_ac_power);
            }
            if (this->stmp_internal_ != desired_stmp && desired_stmp > 0)
            {
                // Every setpoint command lights the unit's display. Under throttle control the
                // target moves with the unit's own drifting sensor, so without a floor on the
                // command rate a bedroom would flash all night. A mode change is the actual
                // on/off decision and is never held back.
                uint32_t now = millis();
                // Only bypass the rate limit for a mode change we are actually issuing now. On a
                // duplicate call the mode command is suppressed above, so the setpoint must fall
                // back to the rate limit rather than riding a bypass that no longer applies.
                bool mode_changing = this->mode_internal_ != desired_mode && !mode_recently_sent;
                if (mode_changing || this->last_stmp_command_ms_ == 0 || (now - this->last_stmp_command_ms_) >= this->min_command_interval_)
                {
                    ESP_LOGI("climate", "adv. heat/cool - setting temp to %u from cur. internal temp %u (throttle %u)", desired_stmp, this->stmp_internal_, this->throttle_);
                    this->parent_->set_pending_parameter(AcState::AC_STMP, desired_stmp);
                    this->last_stmp_command_ms_ = now;
                }
                else
                {
                    // evaluate_() runs on a 30s timer and will pick this up once eligible
                    ESP_LOGD("climate", "adv. heat/cool - deferring setpoint %u, rate limited", desired_stmp);
                }
            }
        }

        bool PioneerMinisplitClimate::cooling_required_()
        {
            // if mode is not HEAT_COOL, we don't act
            if (this->mode != climate::CLIMATE_MODE_HEAT_COOL)
            {
                return false;
            }

            auto temperature = this->target_temperature_high;
            float current = this->control_temperature_();

            if (std::isnan(current))
            {
                // no reading to decide on yet; hold whatever we were doing
                return this->target_action_ == climate::CLIMATE_ACTION_COOLING;
            }

            if (current > temperature + this->cooling_deadband_)
            {
                // if the current temperature exceeds the target + deadband, cooling is required
                return true;
            }
            else if (current < temperature - this->cooling_overrun_)
            {
                // if the current temperature is less than the target - overrun, cooling should stop
                return false;
            }
            else
            {
                // if we get here, the current temperature is between target + deadband and target - overrun,
                //  so the action should continue whatever it was already doing
                return this->target_action_ == climate::CLIMATE_ACTION_COOLING;
            }
        }

        bool PioneerMinisplitClimate::heating_required_()
        {
            // if mode is not HEAT_COOL, we don't act
            if (this->mode != climate::CLIMATE_MODE_HEAT_COOL)
            {
                return false;
            }

            auto temperature = this->target_temperature_low;
            float current = this->control_temperature_();

            if (std::isnan(current))
            {
                // no reading to decide on yet; hold whatever we were doing
                return this->target_action_ == climate::CLIMATE_ACTION_HEATING;
            }

            if (current < temperature - this->heating_deadband_)
            {
                // if the current temperature is below the target - deadband, heating is required
                return true;
            }
            else if (current > temperature + this->heating_overrun_)
            {
                // if the current temperature is above the target + overrun, heating should stop
                return false;
            }
            else
            {
                // if we get here, the current temperature is between target - deadband and target + overrun,
                //  so the action should continue whatever it was already doing
                return this->target_action_ == climate::CLIMATE_ACTION_HEATING;
            }
        }

        // define the supported device traits
        climate::ClimateTraits PioneerMinisplitClimate::traits()
        {
            auto traits = climate::ClimateTraits();
            traits.set_supports_current_temperature(true);
            traits.set_supports_action(true);
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
                traits.set_supported_modes({climate::CLIMATE_MODE_FAN_ONLY,
                                            climate::CLIMATE_MODE_OFF,
                                            climate::CLIMATE_MODE_DRY,
                                            climate::CLIMATE_MODE_HEAT_COOL});
            }
            else
            {
                traits.set_supported_modes({climate::CLIMATE_MODE_OFF,
                                            climate::CLIMATE_MODE_COOL,
                                            climate::CLIMATE_MODE_FAN_ONLY,
                                            climate::CLIMATE_MODE_DRY,
                                            climate::CLIMATE_MODE_HEAT,
                                            climate::CLIMATE_MODE_HEAT_COOL});
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
