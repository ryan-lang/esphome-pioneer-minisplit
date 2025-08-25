#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include <vector>
#include <memory>
#include <cstring>

namespace esphome
{
    namespace pioneer_minisplit
    {

        class AcState
        {
        public:
            enum ParameterType
            {
                AC_ECO,
                AC_DISPLAY,
                AC_POWER,
                AC_TURBO,
                AC_MODE,
                AC_STMP,
                AC_FAN,
                AC_SWING_H,
                AC_SWING_V,
                AC_SLEEP,
                AC_HEALTH,
                AC_MUTE,
                AC_BEEP,
                AC_CUR_TEMP,
                AC_UNKNOWN_1,
                AC_FAN_SPEED_ACTUAL,
                AC_TEMP_PIPE_OUT,
                AC_TEMP_PIPE_IN,
                AC_UNKNOWN_2,
                AC_UNKNOWN_3,
                AC_COMPRESSOR_CURRENT,
                AC_ACTION,
                AC_FAULT,
                AC_CLEAN_FILTER,
                AC_SLEEP_EXT,
                AC_SWING_V_POS,
                AC_SWING_H_POS,
                AC_MOTOR,
                AC_SUPPLY_VOLTAGE,
                AC_PARAM_COUNT  // Keep this last - used for array sizing
            };

            AcState() {
                // Initialize arrays with default values
                std::memset(state, 0, sizeof(state));
                std::memset(state_float, 0, sizeof(state_float));
            }

            void set(ParameterType command, uint8_t value)
            {
                if (command < AC_PARAM_COUNT) {
                    this->state[command] = value;
                }
            }

            void set_float(ParameterType command, float value)
            {
                if (command < AC_PARAM_COUNT) {
                    this->state_float[command] = value;
                }
            }

            uint8_t get(ParameterType command)
            {
                if (command < AC_PARAM_COUNT) {
                    return this->state[command];
                }
                return 0;
            }

            float get_float(ParameterType command)
            {
                if (command < AC_PARAM_COUNT) {
                    return this->state_float[command];
                }
                return 0.0f;
            }

            // Copy values from another state
            void copy_from(const AcState* other)
            {
                std::memcpy(this->state, other->state, sizeof(this->state));
                std::memcpy(this->state_float, other->state_float, sizeof(this->state_float));
            }

            uint8_t state[AC_PARAM_COUNT];
            float state_float[AC_PARAM_COUNT];
        };

        struct AcStateListener
        {
            std::function<void(AcState *state)> func;
        };

        class PioneerMinisplit : public Component, public uart::UARTDevice
        {
        public:
            PioneerMinisplit() : Component(), UARTDevice(), ac_state(new AcState()){};
            void loop() override;
            void register_listener(const std::function<void(AcState *state)> &func);
            void set_pending_parameter(AcState::ParameterType param, uint8_t value);
            void set_pending_parameter_float(AcState::ParameterType param, float value);
            void clear_pending_changes();
            bool has_pending_changes() const { return pending_change_count > 0; }
            AcState *get_pending_state();
            AcState *ac_state;

        private:
            void read_serial_data_();
            bool is_time_elapsed_(unsigned long &last_time, unsigned long interval);
            void process_serial_data_();
            void process_valid_data_();
            void send_pending_state_();
            void populate_command_data_(AcState *state, uint8_t *AcCmd);
            void calculate_and_set_checksum_(uint8_t *AcCmd);
            void send_heartbeat_if_required_();

            static const uint8_t TOTAL_COMMANDS = 13;
            static const uint8_t COMMAND_LENGTH = 31;
            std::vector<AcStateListener> listeners_;
            uint8_t rx_pos = 0;
            uint8_t rx_line[70];
            uint8_t rx_line_last[70];
            unsigned long minidelay = 0;
            unsigned long hbeat = 0;
            bool has_state_ack = true;
            
            // Pending state management - only track changed parameters
            static const uint8_t MAX_PENDING_CHANGES = 16;
            uint8_t pending_params[MAX_PENDING_CHANGES];
            uint8_t pending_values[MAX_PENDING_CHANGES];
            float pending_float_values[MAX_PENDING_CHANGES];
            uint8_t pending_change_count = 0;
            AcState pending_state_buffer;  // Reusable buffer for pending state
        };

    }
}