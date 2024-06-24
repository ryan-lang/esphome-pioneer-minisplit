#include "pioneer_minisplit.h"

namespace esphome
{
    namespace pioneer_minisplit
    {

        void PioneerMinisplit::setup()
        {
            this->ac_state = new AcState();
        }

        // main loop
        void PioneerMinisplit::loop()
        {
            this->read_serial_data_();
            if (this->is_time_elapsed_(this->minidelay, 100))
            {
                this->process_serial_data_();
                this->send_pending_state_();
                this->send_heartbeat_if_required_();
            }
        }

        void PioneerMinisplit::register_listener(const std::function<void(AcState *state)> &func)
        {
            auto listener = AcStateListener{
                .func = func,
            };
            this->listeners_.push_back(listener);
        }

        // takes whatever the state is at this moment in time, and clones it into
        // the ac_state_pending object, which will eventually be sent to the device
        void PioneerMinisplit::prepare_state_pending()
        {
            this->ac_state_pending = this->ac_state->clone();
        }

        // read serial data into rx_line buffer
        void PioneerMinisplit::read_serial_data_()
        {
            while (this->available())
            {
                if (this->rx_pos < sizeof(this->rx_line) - 1)
                {
                    this->rx_line[this->rx_pos++] = this->read();
                }
                else
                {
                    this->rx_pos = 0;
                }
                this->minidelay = millis();
            }
        }

        // check if interval has elapsed since last_time, if so, update last_time
        bool PioneerMinisplit::is_time_elapsed_(unsigned long &last_time, unsigned long interval)
        {
            unsigned long current = millis();
            if (current - last_time > interval)
            {
                last_time = current;
                return true;
            }
            return false;
        }

        // receive a message from device then process if valid
        void PioneerMinisplit::process_serial_data_()
        {
            if (this->rx_pos <= 1)
                return;

            char checksum = 0;
            for (char i = 0; i < this->rx_pos - 1; i++)
            {
                checksum ^= this->rx_line[i];
            }

            if (this->rx_line[0] == 0xBB && checksum == this->rx_line[rx_pos - 1])
            {
                this->process_valid_data_();
            }
            else
            {
                ESP_LOGD("pioneer", ">>>> INVALID CHECKSUM");
            }

            this->rx_pos = 0;
        }

        // process data received from device, which has already been determined to be valid
        // then update the in-memory state, then trigger state listeners
        void PioneerMinisplit::process_valid_data_()
        {
            if (this->rx_pos != 61)
                return;

            // LOG each received byte as hex
            for (char i = 0; i < this->rx_pos; i++)
            {
                ESP_LOGVV("pioneer", ">>>> %d: %02X (%d)", i, rx_line[i], rx_line[i]);
            }

            // compare the current rx_line to the last rx_line, logging any differences and updating as we go
            for (char i = 0; i < this->rx_pos; i++)
            {
                if (this->rx_line[i] != this->rx_line_last[i])
                {
                    // skip checksum
                    if (i == 60)
                    {
                        continue;
                    }

                    // voltage
                    if (i == 45)
                    {
                        continue;
                    }

                    // temp is known
                    if (i == 17 || i == 18)
                    {
                        continue;
                    }

                    // ignore unknown_1
                    if (i == 30)
                    {
                        continue;
                    }

                    // ignore fan speed
                    if (i == 34)
                    {
                        continue;
                    }

                    // ignore pipe temp, compressor & motor_raw
                    if (i == 35 || i == 36 || i == 37 || i == 38 || i == 46)
                    {
                        continue;
                    }

                    ESP_LOGD("pioneer", "<> %d: 0x%02X (%d) -> 0x%02X (%d)", i, this->rx_line_last[i], this->rx_line_last[i], this->rx_line[i], this->rx_line[i]);
                    this->rx_line_last[i] = this->rx_line[i];
                }
            }

            this->has_state_ack = true;

            this->ac_state->set(AcState::AC_ECO, (this->rx_line[7] >> 6) & 1);
            this->ac_state->set(AcState::AC_DISPLAY, (this->rx_line[7] >> 5) & 1);
            this->ac_state->set(AcState::AC_POWER, (this->rx_line[7] >> 4) & 1);
            this->ac_state->set(AcState::AC_TURBO, (this->rx_line[7] >> 7) & 1);
            this->ac_state->set(AcState::AC_MODE, (this->rx_line[7] & 0x0F) - 1);
            this->ac_state->set(AcState::AC_STMP, (this->rx_line[8] & 0x0F) + 16);
            this->ac_state->set(AcState::AC_FAN, (this->rx_line[8] >> 4) - 8);
            this->ac_state->set(AcState::AC_HEALTH, this->rx_line[9]);
            this->ac_state->set(AcState::AC_SWING_H, (this->rx_line[10] & 0x20) != 0);
            this->ac_state->set(AcState::AC_SWING_V, (this->rx_line[10] & 0x40) != 0);
            this->ac_state->set_float(AcState::AC_CUR_TEMP, (((this->rx_line[17] << 8) | this->rx_line[18]) / 374.0f - 32.0f) / 1.8f);
            this->ac_state->set(AcState::AC_SLEEP, this->rx_line[19] & 1);
            this->ac_state->set(AcState::AC_SLEEP_EXT, this->rx_line[19]);
            this->ac_state->set(AcState::AC_UNKNOWN_1, this->rx_line[30]);
            this->ac_state->set(AcState::AC_FAN_SPEED_ACTUAL, this->rx_line[34]);
            this->ac_state->set(AcState::AC_TEMP_PIPE_OUT, this->rx_line[35]);
            this->ac_state->set(AcState::AC_TEMP_PIPE_IN, this->rx_line[36]);
            this->ac_state->set(AcState::AC_UNKNOWN_2, this->rx_line[37]);
            this->ac_state->set(AcState::AC_UNKNOWN_3, this->rx_line[38]);
            this->ac_state->set_float(AcState::AC_COMPRESSOR_CURRENT, this->rx_line[39] / 10.0f);
            this->ac_state->set(AcState::AC_ACTION, this->rx_line[40]);
            this->ac_state->set(AcState::AC_FAULT, this->rx_line[44]);
            this->ac_state->set(AcState::AC_SUPPLY_VOLTAGE, this->rx_line[45]);
            this->ac_state->set(AcState::AC_MOTOR, this->rx_line[46]);
            this->ac_state->set(AcState::AC_CLEAN_FILTER, this->rx_line[50]);
            this->ac_state->set(AcState::AC_SWING_V_POS, this->rx_line[51]);
            this->ac_state->set(AcState::AC_SWING_H_POS, this->rx_line[52]);

            // trigger listeners
            for (auto &listener : this->listeners_)
            {
                listener.func(this->ac_state);
            }
        }

        // send the pending state to the device
        void PioneerMinisplit::send_pending_state_()
        {
            if (!this->has_state_ack || !this->ac_state_pending)
                return;

            // prevents sending a 2nd command while waiting for response from first
            // also don't send a command before we've received the first state response
            this->has_state_ack = 0;

            // copy and reset the pending state; ac_state_pending will now be null
            AcState *state_pending = this->ac_state_pending.release();

            // special handling for BEEP, which is not tracked in device state payload
            if (state_pending->get(AcState::AC_BEEP) != this->ac_state->get(AcState::AC_BEEP))
            {
                this->ac_state->set(AcState::AC_BEEP, state_pending->get(AcState::AC_BEEP));
            }

            uint8_t AcCmd[this->COMMAND_LENGTH] = {};
            this->populate_command_data_(state_pending, AcCmd);
            this->calculate_and_set_checksum_(AcCmd);

            this->write_array(AcCmd, this->COMMAND_LENGTH);
            this->hbeat = millis();
        }

        // populate the AcCmd buffer with the current state
        void populate_command_data_(AcState *state, uint8_t *AcCmd)
        {
            const char modeMap[] = {3, 7, 2, 1, 8};
            const char fanMap[] = {0, 2, 3, 5, 0};

            AcCmd[0] = 0xBB;
            AcCmd[1] = 0x00;
            AcCmd[2] = 0x01;
            AcCmd[3] = 0x03;
            AcCmd[4] = 0x19;
            AcCmd[5] = 0x01;

            AcCmd[7] = 0;
            AcCmd[7] |= (state->get(AcState::AC_POWER) ? 1 : 0) << 2;
            AcCmd[7] |= (state->get(AcState::AC_BEEP) ? 1 : 0) << 5;
            AcCmd[7] |= (state->get(AcState::AC_DISPLAY) ? 1 : 0) << 6;
            AcCmd[7] |= (state->get(AcState::AC_ECO) ? 1 : 0) << 7;
            ESP_LOGD("pioneer", ">>> [7]: 0x%02X", AcCmd[7]);

            AcCmd[8] = 0;
            AcCmd[8] = modeMap[state->get(AcState::AC_MODE)];
            AcCmd[8] |= (state->get(AcState::AC_TURBO) ? 1 : 0) << 6;
            ESP_LOGD("pioneer", ">>> [8]: 0x%02X", AcCmd[8]);

            // enforce min/max
            if (state->get(AcState::AC_STMP) < 16)
            {
                state->set(AcState::AC_STMP, 16);
            }
            else if (state->get(AcState::AC_STMP) > 31)
            {
                state->set(AcState::AC_STMP, 31);
            }
            AcCmd[9] = 31 - state->get(AcState::AC_STMP);
            ESP_LOGD("pioneer", ">>> [9]: 0x%02X", AcCmd[9]);

            AcCmd[10] = 0;
            AcCmd[10] = fanMap[state->get(AcState::AC_FAN)];
            if (state->get(AcState::AC_SWING_V))
            {
                AcCmd[10] |= (1 << 3) | (1 << 4) | (1 << 5);
            }
            ESP_LOGD("pioneer", ">>> [10]: 0x%02X", AcCmd[10]);

            AcCmd[14] = 0;
            if (state->get(AcState::AC_SWING_H))
            {
                AcCmd[14] |= (1 << 3) | (1 << 4) | (1 << 5);
            }
            ESP_LOGD("pioneer", ">>> [14]: 0x%02X", AcCmd[14]);

            AcCmd[19] = state->get(AcState::AC_SLEEP) ? 1 : 0;
        }

        void PioneerMinisplit::calculate_and_set_checksum_(uint8_t *AcCmd)
        {
            char checksum = 0;
            for (char i = 0; i < this->COMMAND_LENGTH - 1; i++)
            {
                checksum ^= AcCmd[i];
            }
            AcCmd[this->COMMAND_LENGTH - 1] = checksum;
        }

        void PioneerMinisplit::send_heartbeat_if_required_()
        {
            if (is_time_elapsed_(this->hbeat, 2000))
            {
                const uint8_t heartbeat[] = {0xBB, 0x00, 0x01, 0x04, 0x02, 0x01, 0x00, 0xBD};
                this->write_array(heartbeat, sizeof(heartbeat));
            }
        }
    }
}