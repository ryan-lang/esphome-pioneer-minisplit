#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include <map>
#include <vector>
#include <memory>

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
                AC_SUPPLY_VOLTAGE
            };

            void set(ParameterType command, uint8_t value)
            {
                this->state[command] = value;
            }

            void set_float(ParameterType command, float value)
            {
                this->state_float[command] = value;
            }

            uint8_t get(ParameterType command)
            {
                return this->state[command];
            }

            float get_float(ParameterType command)
            {
                return this->state_float[command];
            }

            std::unique_ptr<AcState> clone()
            {
                return std::make_unique<AcState>(*this);
            }

        private:
            std::map<ParameterType, uint8_t> state;
            std::map<ParameterType, float> state_float;
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
            void prepare_state_pending();
            std::unique_ptr<AcState> ac_state_pending;

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
            AcState *ac_state;
            std::vector<AcStateListener> listeners_;
            uint8_t rx_pos = 0;
            uint8_t rx_line[70];
            uint8_t rx_line_last[70];
            unsigned long minidelay = 0;
            unsigned long hbeat = 0;
            bool has_state_ack = true;
        };

    }
}