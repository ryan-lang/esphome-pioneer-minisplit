#pragma once

#include "esphome/components/pioneer_minisplit/pioneer_minisplit.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome
{

    namespace pioneer_minisplit
    {

        class PioneerMinisplitClimate : public Component, public climate::Climate
        {
        public:
            PioneerMinisplitClimate(PioneerMinisplit *parent) : parent_(parent) {}

            void set_cool_deadband(float deadband) { this->cooling_deadband_ = deadband; }
            void set_cool_overrun(float overrun) { this->cooling_overrun_ = overrun; }
            void set_heat_deadband(float deadband) { this->heating_deadband_ = deadband; }
            void set_heat_overrun(float overrun) { this->heating_overrun_ = overrun; }

            void set_remote_sensor(sensor::Sensor *sensor) { this->remote_sensor_ = sensor; }
            void set_sensor_timeout(uint32_t timeout) { this->sensor_timeout_ = timeout; }
            void set_min_command_interval(uint32_t interval) { this->min_command_interval_ = interval; }
            void set_throttle_gain(float gain) { this->throttle_gain_ = gain; }
            void set_max_throttle(uint8_t max_throttle) { this->max_throttle_ = max_throttle; }
            void set_settle_time(uint32_t settle_time) { this->settle_time_ = settle_time; }

            /// Diagnostics, for template sensors in YAML.
            bool remote_sensor_active() { return this->remote_valid_(); }
            uint8_t commanded_throttle() { return this->throttle_; }
            float internal_temperature() { return this->internal_temp_; }
            float internal_temperature_avg() { return this->internal_temp_avg_; }

            /// The four thresholds the hysteresis actually switches on, so they can be plotted
            /// alongside the temperature traces. The displayed target temperatures are rounded
            /// and the deadband/overrun are invisible, which makes a graph hard to read without
            /// these. NaN until the targets have been set.
            float cool_start_temperature() { return this->target_temperature_high + this->cooling_deadband_; }
            float cool_stop_temperature() { return this->target_temperature_high - this->cooling_overrun_; }
            float heat_start_temperature() { return this->target_temperature_low - this->heating_deadband_; }
            float heat_stop_temperature() { return this->target_temperature_low + this->heating_overrun_; }

        protected:
            void setup() override;
            void control(const climate::ClimateCall &call) override;
            climate::ClimateTraits traits() override;

            void switch_to_action_(climate::ClimateAction action);

            climate::ClimateAction compute_action_();

            /// Check if cooling/fanning/heating actions are required; returns true if so
            bool cooling_required_();
            bool heating_required_();

            /// True while the remote sensor has produced a sane reading recently enough to
            /// trust. Everything below falls back to the unit's own sensor when this is false.
            bool remote_valid_();

            /// The temperature the control loop runs on: remote when trusted, else internal.
            float control_temperature_();

            /// Whether we know enough to command the unit at all. A configured remote sensor
            /// that has never reported is not the same as one that has gone stale: at boot we
            /// have no trustworthy reading, and deciding from the unit's own sensor is the very
            /// thing this component exists to avoid.
            bool control_ready_();

            /// The value we put in the unit's setpoint field. With a trusted remote reading
            /// this is a capacity request expressed relative to the unit's own sensor, not a
            /// setpoint - see the comment on the definition.
            uint8_t compute_setpoint_(bool cooling);

            /// Re-run the control decision and republish; called on a timer so that a remote
            /// sensor going stale is noticed even when nothing else changes.
            void evaluate_();

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
            climate::ClimateMode mode_internal_;
            uint8_t stmp_internal_;

            /// The action we last asked the unit for. Used to latch hysteresis, since the
            /// action the unit reports drops to idle during its compressor cooldown, which
            /// would otherwise read as "stop cooling" and bounce the unit off and back on.
            climate::ClimateAction target_action_{climate::CLIMATE_ACTION_IDLE};

            /// Optional remote room thermometer and its freshness bookkeeping.
            sensor::Sensor *remote_sensor_{nullptr};
            uint32_t sensor_timeout_{300000};
            uint32_t remote_last_ms_{0};
            bool remote_has_value_{false};
            bool remote_active_last_{false};
            float remote_temp_{NAN};

            /// The unit's own return-air reading. Kept separate from current_temperature,
            /// which now carries whichever reading the loop is actually running on.
            float internal_temp_{NAN};

            /// Slow average of the above, for the diagnostic sensor only. Was briefly used to
            /// derive the setpoint, on the theory that the raw sensor's 0.8C steps between
            /// discrete levels were noise worth filtering. They are, but following them is also
            /// what holds the commanded error at the requested throttle, so smoothing here cost
            /// 1.8x the energy. Useful to plot against the raw trace; not for control.
            float internal_temp_avg_{NAN};

            /// Capacity request currently being asked of the unit, in degrees C of
            /// deliberate setpoint offset from its own reading. 0 when not throttling.
            uint8_t throttle_{0};
            float throttle_gain_{2.5f};
            uint8_t max_throttle_{4};

            uint32_t min_command_interval_{60000};
            uint32_t last_stmp_command_ms_{0};

            /// The setpoint we have decided on, held across recomputes so that a wobbling
            /// internal sensor cannot toggle it. 0 means "nothing decided yet, compute fresh".
            uint8_t committed_stmp_{0};

            /// When we last switched the unit into an active mode. For the first settle_time_
            /// after that the unit's sensor is still reacting to its own fan starting, so the
            /// setpoint is held rather than chased.
            uint32_t action_started_ms_{0};
            uint32_t settle_time_{180000};


            /// The mode we last queued, and when. mode_internal_ only updates once the unit
            /// acknowledges, so two calls inside that gap would both queue the same command.
            climate::ClimateMode last_mode_sent_{climate::CLIMATE_MODE_OFF};
            uint32_t last_mode_command_ms_{0};
        };
    }
}