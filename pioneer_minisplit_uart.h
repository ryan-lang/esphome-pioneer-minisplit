#include "esphome.h"

#define get_pioneer_minisplit(constructor) static_cast<PioneerMinisplit *>(constructor.get_component(0))

#define pioneer_minisplit_climate(p) get_pioneer_minisplit(p)
#define pioneer_minisplit_switch_display(p) get_pioneer_minisplit(p)->switch_display
#define pioneer_minisplit_switch_beep(p) get_pioneer_minisplit(p)->switch_beep
#define pioneer_minisplit_sensor_unknown_1(p) get_pioneer_minisplit(p)->sensor_unknown_1
#define pioneer_minisplit_sensor_fan_speed_actual(p) get_pioneer_minisplit(p)->sensor_fan_speed_actual
#define pioneer_minisplit_sensor_temp_pipe_out(p) get_pioneer_minisplit(p)->sensor_temp_pipe_out
#define pioneer_minisplit_sensor_temp_pipe_in(p) get_pioneer_minisplit(p)->sensor_temp_pipe_in
#define pioneer_minisplit_sensor_unknown_2(p) get_pioneer_minisplit(p)->sensor_unknown_2
#define pioneer_minisplit_sensor_unknown_3(p) get_pioneer_minisplit(p)->sensor_unknown_3
#define pioneer_minisplit_sensor_compressor_current(p) get_pioneer_minisplit(p)->sensor_compressor_current
#define pioneer_minisplit_sensor_motor(p) get_pioneer_minisplit(p)->sensor_motor
#define pioneer_minisplit_sensor_motor_raw(p) get_pioneer_minisplit(p)->sensor_motor_raw
#define pioneer_minisplit_sensor_swing_v_pos(p) get_pioneer_minisplit(p)->sensor_swing_v_pos
#define pioneer_minisplit_sensor_swing_h_pos(p) get_pioneer_minisplit(p)->sensor_swing_h_pos
#define pioneer_minisplit_sensor_fault(p) get_pioneer_minisplit(p)->sensor_fault
#define pioneer_minisplit_sensor_clean_filter(p) get_pioneer_minisplit(p)->sensor_clean_filter
#define pioneer_minisplit_sensor_health(p) get_pioneer_minisplit(p)->sensor_health
#define pioneer_minisplit_sensor_deep_sleep(p) get_pioneer_minisplit(p)->sensor_deep_sleep
#define pioneer_minisplit_sensor_supply_voltage(p) get_pioneer_minisplit(p)->sensor_supply_voltage

enum PioneerMotorSpeed : uint8_t
{
    MOTOR_SPEED_OFF = 0,
    MOTOR_SPEED_LOW = 1,
    MOTOR_SPEED_HIGH = 2
};

class PioneerMinisplit;

class PioneerMinisplitSwitch : public Switch
{
public:
    PioneerMinisplitSwitch(PioneerMinisplit *parent, const std::string &name) : parent_(parent), name_(name) {}
    void write_state(bool state) override;

private:
    PioneerMinisplit *parent_;
    std::string name_;
};

class PioneerMinisplitSensor : public Sensor
{
public:
    PioneerMinisplitSensor(PioneerMinisplit *parent, const std::string &name) : parent_(parent), name_(name) {}

private:
    PioneerMinisplit *parent_;
    std::string name_;
};

class PioneerMinisplitTextSensor : public TextSensor
{
public:
    PioneerMinisplitTextSensor(PioneerMinisplit *parent, const std::string &name) : parent_(parent), name_(name) {}

private:
    PioneerMinisplit *parent_;
    std::string name_;
};

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
        state[command] = value;
    }

    void set_float(ParameterType command, float value)
    {
        state_float[command] = value;
    }

    uint8_t get(ParameterType command)
    {
        return state[command];
    }

    float get_float(ParameterType command)
    {
        return state_float[command];
    }

    std::unique_ptr<AcState> clone()
    {
        return std::make_unique<AcState>(*this);
    }

private:
    std::map<ParameterType, uint8_t> state;
    std::map<ParameterType, float> state_float;
};

class PioneerMinisplit : public Component, public Climate, public UARTDevice
{
public:
    static const uint8_t TOTAL_COMMANDS = 13;
    static const uint8_t COMMAND_LENGTH = 31;

    PioneerMinisplitSwitch *switch_display;
    PioneerMinisplitSwitch *switch_beep;

    PioneerMinisplitSensor *sensor_unknown_1;
    PioneerMinisplitSensor *sensor_fan_speed_actual;
    PioneerMinisplitSensor *sensor_temp_pipe_out;
    PioneerMinisplitSensor *sensor_temp_pipe_in;
    PioneerMinisplitSensor *sensor_unknown_2;
    PioneerMinisplitSensor *sensor_unknown_3;
    PioneerMinisplitSensor *sensor_compressor_current;
    PioneerMinisplitSensor *sensor_motor_raw;
    PioneerMinisplitSensor *sensor_swing_v_pos;
    PioneerMinisplitSensor *sensor_swing_h_pos;
    PioneerMinisplitSensor *sensor_fault;
    PioneerMinisplitSensor *sensor_clean_filter;
    PioneerMinisplitSensor *sensor_health;
    PioneerMinisplitSensor *sensor_supply_voltage;
    PioneerMinisplitTextSensor *sensor_motor;
    BinarySensor *sensor_deep_sleep;

    PioneerMinisplit(UARTComponent *parent) : Component(), Climate(), UARTDevice(parent), ac_state(new AcState())
    {
        switch_display = new PioneerMinisplitSwitch(this, "display");
        switch_beep = new PioneerMinisplitSwitch(this, "beep");

        sensor_unknown_1 = new PioneerMinisplitSensor(this, "unknown_1");
        sensor_fan_speed_actual = new PioneerMinisplitSensor(this, "fan_speed_actual");
        sensor_temp_pipe_out = new PioneerMinisplitSensor(this, "temp_pipe_out");
        sensor_temp_pipe_in = new PioneerMinisplitSensor(this, "temp_pipe_in");
        sensor_unknown_2 = new PioneerMinisplitSensor(this, "unknown_2");
        sensor_unknown_3 = new PioneerMinisplitSensor(this, "unknown_3");
        sensor_compressor_current = new PioneerMinisplitSensor(this, "compressor_current");
        sensor_motor_raw = new PioneerMinisplitSensor(this, "motor_raw");
        sensor_swing_v_pos = new PioneerMinisplitSensor(this, "swing_v_pos");
        sensor_swing_h_pos = new PioneerMinisplitSensor(this, "swing_h_pos");
        sensor_fault = new PioneerMinisplitSensor(this, "fault");
        sensor_clean_filter = new PioneerMinisplitSensor(this, "clean_filter");
        sensor_health = new PioneerMinisplitSensor(this, "health");
        sensor_supply_voltage = new PioneerMinisplitSensor(this, "supply_volrage");

        sensor_motor = new PioneerMinisplitTextSensor(this, "motor");

        sensor_deep_sleep = new BinarySensor();
    }

    void setup() override {}

    void control(const ClimateCall &call) override
    {
        // clone the state and start building a pending state
        this->ac_state_pending = this->ac_state->clone();

        // MODE change
        if (call.get_mode().has_value())
        {
            uint8_t new_ac_mode;
            bool new_ac_power;
            esphome_mode_to_ac_mode(*call.get_mode(), new_ac_mode, new_ac_power);

            this->ac_state_pending->set(AcState::AC_MODE, new_ac_mode);
            this->ac_state_pending->set(AcState::AC_POWER, new_ac_power);
        }

        // PRESET change
        if (call.get_preset().has_value())
        {
            bool new_ac_eco;
            bool new_ac_turbo;
            bool new_ac_sleep;
            esphome_preset_to_ac_preset(*call.get_preset(), new_ac_eco, new_ac_turbo, new_ac_sleep);

            this->ac_state_pending->set(AcState::AC_ECO, new_ac_eco);
            this->ac_state_pending->set(AcState::AC_TURBO, new_ac_turbo);
            this->ac_state_pending->set(AcState::AC_SLEEP, new_ac_sleep);
        }

        // TARG TEMP change
        if (call.get_target_temperature().has_value())
        {
            this->ac_state_pending->set(AcState::AC_STMP, *call.get_target_temperature());
        }

        // FAN change
        if (call.get_fan_mode().has_value())
        {
            uint8_t new_ac_fan = esphome_fan_mode_to_ac_fan_mode(*call.get_fan_mode());
            this->ac_state_pending->set(AcState::AC_FAN, new_ac_fan);
        }

        // SWING change
        if (call.get_swing_mode().has_value())
        {
            u_int8_t new_swing_h;
            u_int8_t new_swing_v;
            esphome_swing_mode_to_ac_swing_mode(*call.get_swing_mode(), new_swing_h, new_swing_v);

            this->ac_state_pending->set(AcState::AC_SWING_H, new_swing_h);
            this->ac_state_pending->set(AcState::AC_SWING_V, new_swing_v);
        }
    }

    // called when the switch is toggled from esphome
    void on_switch_write_state(const std::string &switch_name, bool state)
    {
        ESP_LOGD("pioneer", "SWITCH WRITE STATE: %s, STATE: %d", switch_name.c_str(), state);

        // clone the state and start building a pending state
        this->ac_state_pending = this->ac_state->clone();

        if (switch_name == "display")
        {
            // TODO: block change if power is OFF
            this->ac_state_pending->set(AcState::AC_DISPLAY, state);
        }
        else if (switch_name == "beep")
        {
            this->ac_state_pending->set(AcState::AC_BEEP, state);

            // BEEP is not reported in state response, so we need to track it separately
            this->ac_state->set(AcState::AC_BEEP, state);
            this->switch_beep->publish_state(state);
        }
    }

    ClimateTraits traits() override
    {
        auto traits = climate::ClimateTraits();
        traits.set_supports_current_temperature(true);
        traits.set_supports_action(true);
        traits.set_supported_modes({CLIMATE_MODE_OFF, CLIMATE_MODE_COOL, CLIMATE_MODE_FAN_ONLY, CLIMATE_MODE_DRY, CLIMATE_MODE_HEAT, CLIMATE_MODE_HEAT_COOL});
        traits.set_supported_fan_modes({CLIMATE_FAN_AUTO, CLIMATE_FAN_LOW, CLIMATE_FAN_MEDIUM, CLIMATE_FAN_HIGH});
        traits.set_supported_swing_modes({CLIMATE_SWING_OFF, CLIMATE_SWING_BOTH, CLIMATE_SWING_VERTICAL, CLIMATE_SWING_HORIZONTAL});
        traits.set_supported_presets({CLIMATE_PRESET_NONE, CLIMATE_PRESET_BOOST, CLIMATE_PRESET_ECO, CLIMATE_PRESET_SLEEP});
        traits.set_visual_min_temperature(16);
        traits.set_visual_max_temperature(31);

        return traits;
    }

    void loop() override
    {
        read_serial_data();
        if (is_time_elapsed(minidelay, 100))
        {
            process_serial_data();
            send_pending_state();
            send_heartbeat_if_required();
        }
    }

private:
    AcState *ac_state;
    std::unique_ptr<AcState> ac_state_pending;

    uint8_t rx_pos = 0;
    uint8_t rx_line[70];
    uint8_t rx_line_last[70];
    unsigned long minidelay = 0;
    unsigned long hbeat = 0;
    bool has_state_ack = true;

    void read_serial_data()
    {
        while (available())
        {
            if (rx_pos < sizeof(rx_line) - 1)
            {
                rx_line[rx_pos++] = read();
            }
            else
            {
                rx_pos = 0;
            }
            minidelay = millis();
        }
    }

    bool is_time_elapsed(unsigned long &last_time, unsigned long interval)
    {
        unsigned long current = millis();
        if (current - last_time > interval)
        {
            last_time = current;
            return true;
        }
        return false;
    }

    void process_serial_data()
    {
        if (rx_pos <= 1)
            return;

        byte checksum = 0;
        for (byte i = 0; i < rx_pos - 1; i++)
        {
            checksum ^= rx_line[i];
        }

        if (rx_line[0] == 0xBB && checksum == rx_line[rx_pos - 1])
        {
            process_valid_data();
        }
        else
        {
            ESP_LOGD("pioneer", ">>>> INVALID CHECKSUM");
        }

        rx_pos = 0;
    }

    void process_valid_data()
    {
        if (rx_pos != 61)
            return;

        // LOG each received byte as hex
        for (byte i = 0; i < rx_pos; i++)
        {
            ESP_LOGVV("pioneer", ">>>> %d: %02X (%d)", i, rx_line[i], rx_line[i]);
        }

        // compare the current rx_line to the last rx_line, logging any differences and updating as we go
        for (byte i = 0; i < rx_pos; i++)
        {
            if (rx_line[i] != rx_line_last[i])
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

                ESP_LOGD("pioneer", "<> %d: 0x%02X (%d) -> 0x%02X (%d)", i, rx_line_last[i], rx_line_last[i], rx_line[i], rx_line[i]);
                rx_line_last[i] = rx_line[i];
            }
        }

        has_state_ack = 1;

        this->ac_state->set(AcState::AC_ECO, (rx_line[7] >> 6) & 1);
        this->ac_state->set(AcState::AC_DISPLAY, (rx_line[7] >> 5) & 1);
        this->ac_state->set(AcState::AC_POWER, (rx_line[7] >> 4) & 1);
        this->ac_state->set(AcState::AC_TURBO, (rx_line[7] >> 7) & 1);
        this->ac_state->set(AcState::AC_MODE, (rx_line[7] & 0x0F) - 1);
        this->ac_state->set(AcState::AC_STMP, (rx_line[8] & 0x0F) + 16);
        this->ac_state->set(AcState::AC_FAN, (rx_line[8] >> 4) - 8);
        this->ac_state->set(AcState::AC_HEALTH, rx_line[9]);
        this->ac_state->set(AcState::AC_SWING_H, (rx_line[10] & 0x20) != 0);
        this->ac_state->set(AcState::AC_SWING_V, (rx_line[10] & 0x40) != 0);
        this->ac_state->set_float(AcState::AC_CUR_TEMP, (((rx_line[17] << 8) | rx_line[18]) / 374.0f - 32.0f) / 1.8f);
        this->ac_state->set(AcState::AC_SLEEP, rx_line[19] & 1);
        this->ac_state->set(AcState::AC_SLEEP_EXT, rx_line[19]);
        this->ac_state->set(AcState::AC_UNKNOWN_1, rx_line[30]);
        this->ac_state->set(AcState::AC_FAN_SPEED_ACTUAL, rx_line[34]);
        this->ac_state->set(AcState::AC_TEMP_PIPE_OUT, rx_line[35]);
        this->ac_state->set(AcState::AC_TEMP_PIPE_IN, rx_line[36]);
        this->ac_state->set(AcState::AC_UNKNOWN_2, rx_line[37]);
        this->ac_state->set(AcState::AC_UNKNOWN_3, rx_line[38]);
        this->ac_state->set_float(AcState::AC_COMPRESSOR_CURRENT, rx_line[39] / 10.0f);
        this->ac_state->set(AcState::AC_ACTION, rx_line[40]);
        this->ac_state->set(AcState::AC_FAULT, rx_line[44]);
        this->ac_state->set(AcState::AC_SUPPLY_VOLTAGE, rx_line[45]);
        this->ac_state->set(AcState::AC_MOTOR, rx_line[46]);
        this->ac_state->set(AcState::AC_CLEAN_FILTER, rx_line[50]);
        this->ac_state->set(AcState::AC_SWING_V_POS, rx_line[51]);
        this->ac_state->set(AcState::AC_SWING_H_POS, rx_line[52]);

        sync_to_esphome(this->ac_state);
    }

    void sync_to_esphome(AcState *state)
    {

        bool climate_changed = false;

        // sync MODE AND POWER
        ClimateMode new_mode = ac_mode_to_esphome_mode(state->get(AcState::AC_MODE), state->get(AcState::AC_POWER));
        if (this->mode != new_mode)
        {
            this->mode = new_mode;
            climate_changed = true;
        }

        // Tracking DISPLAY state change
        if (this->switch_display->state != state->get(AcState::AC_DISPLAY))
        {
            this->switch_display->publish_state(state->get(AcState::AC_DISPLAY));
        }

        // Tracking STMP state change
        if (!std::isnan(state->get(AcState::AC_STMP)) && (std::abs(this->target_temperature - state->get(AcState::AC_STMP)) > 0.01f || std::isnan(this->target_temperature)))
        {
            this->target_temperature = state->get(AcState::AC_STMP);
            climate_changed = true;
        }

        // Tracking FAN state change
        ClimateFanMode new_fan_mode = ac_fan_mode_to_esphome_fan_mode(state->get(AcState::AC_FAN));
        if (this->fan_mode != new_fan_mode)
        {
            this->fan_mode = new_fan_mode;
            climate_changed = true;
        }

        // Tracking HEALTH state change
        if (this->sensor_health->get_state() != state->get(AcState::AC_HEALTH))
        {
            this->sensor_health->publish_state(state->get(AcState::AC_HEALTH));
        }

        // Tracking SWING state change
        ClimateSwingMode new_swing_mode = ac_swing_mode_to_esphome_swing_mode(state->get(AcState::AC_SWING_H), state->get(AcState::AC_SWING_V));
        if (this->swing_mode != new_swing_mode)
        {
            this->swing_mode = new_swing_mode;
            climate_changed = true;
        }

        // Tracking TEMP change
        if (!std::isnan(state->get_float(AcState::AC_CUR_TEMP)) && (std::abs(this->current_temperature - state->get_float(AcState::AC_CUR_TEMP)) > 0.01f || std::isnan(this->current_temperature)))
        { // Assume a change if difference is more than 0.01
            this->current_temperature = state->get_float(AcState::AC_CUR_TEMP);
            climate_changed = true;
        }

        // Tracking SLEEP state change
        // generally in 0x88 (sleep off?)
        // sleep mode 0x89 (sleep on)
        // after 10min 0x08 (deep sleep?)
        if (this->sensor_deep_sleep->state != (state->get(AcState::AC_SLEEP_EXT) == 0x08))
        {
            this->sensor_deep_sleep->publish_state((state->get(AcState::AC_SLEEP_EXT) == 0x08));
        }

        // Tracking AC_UNKNOWN_1 change
        if (this->sensor_unknown_1->get_state() != state->get(AcState::AC_UNKNOWN_1))
        {
            this->sensor_unknown_1->publish_state(state->get(AcState::AC_UNKNOWN_1));
        }

        // Tracking INTERNAL FAN SPEED change
        if (this->sensor_fan_speed_actual->get_state() != state->get(AcState::AC_FAN_SPEED_ACTUAL))
        {
            this->sensor_fan_speed_actual->publish_state(state->get(AcState::AC_FAN_SPEED_ACTUAL));
        }

        // Tracking PIPE OUT TEMP change
        if (this->sensor_temp_pipe_out->get_state() != state->get(AcState::AC_TEMP_PIPE_OUT))
        {
            this->sensor_temp_pipe_out->publish_state(state->get(AcState::AC_TEMP_PIPE_OUT));
        }

        // Tracking PIPE IN TEMP change
        if (this->sensor_temp_pipe_in->get_state() != state->get(AcState::AC_TEMP_PIPE_IN))
        {
            this->sensor_temp_pipe_in->publish_state(state->get(AcState::AC_TEMP_PIPE_IN));
        }

        // Tracking COMPRESSOR 1 change
        if (this->sensor_unknown_2->get_state() != state->get(AcState::AC_UNKNOWN_2))
        {
            this->sensor_unknown_2->publish_state(state->get(AcState::AC_UNKNOWN_2));
        }

        // Tracking COMPRESSOR 2 change
        if (this->sensor_unknown_3->get_state() != state->get(AcState::AC_UNKNOWN_3))
        {
            this->sensor_unknown_3->publish_state(state->get(AcState::AC_UNKNOWN_3));
        }

        // Tracking COMPRESSOR CURRENT change
        if (this->sensor_compressor_current->get_state() != state->get_float(AcState::AC_COMPRESSOR_CURRENT))
        {
            this->sensor_compressor_current->publish_state(state->get_float(AcState::AC_COMPRESSOR_CURRENT));
        }

        // Tracking ACTION change
        ClimateAction new_action = ac_action_to_esphome_action(state->get(AcState::AC_ACTION));
        if (this->action != new_action)
        {
            this->action = new_action;
            climate_changed = true;
        }

        // Tracking FAULT
        if (this->sensor_fault->get_state() != state->get(AcState::AC_FAULT))
        {
            this->sensor_fault->publish_state(state->get(AcState::AC_FAULT));
        }

        // Tracking SUPPLY VOLTAGE
        if (this->sensor_supply_voltage->get_state() != state->get(AcState::AC_SUPPLY_VOLTAGE))
        {
            this->sensor_supply_voltage->publish_state(state->get(AcState::AC_SUPPLY_VOLTAGE));
        }

        // Tracking MOTOR
        if (this->sensor_motor_raw->get_state() != state->get(AcState::AC_MOTOR))
        {
            this->sensor_motor_raw->publish_state(state->get(AcState::AC_MOTOR));
        }
        std::string new_motor_speed = ac_motor_speed_to_string(state->get(AcState::AC_MOTOR));
        if (this->sensor_motor->get_state() != new_motor_speed)
        {
            this->sensor_motor->publish_state(new_motor_speed);
        }

        // Tracking clean filter
        if (this->sensor_clean_filter->get_state() != state->get(AcState::AC_CLEAN_FILTER))
        {
            this->sensor_clean_filter->publish_state(state->get(AcState::AC_CLEAN_FILTER));
        }

        // Tracking SWING V POS
        if (this->sensor_swing_v_pos->get_state() != state->get(AcState::AC_SWING_V_POS))
        {
            this->sensor_swing_v_pos->publish_state(state->get(AcState::AC_SWING_V_POS));
        }

        // Tracking SWING H POS
        if (this->sensor_swing_h_pos->get_state() != state->get(AcState::AC_SWING_H_POS))
        {
            this->sensor_swing_h_pos->publish_state(state->get(AcState::AC_SWING_H_POS));
        }

        // tracking PRESET
        ClimatePreset new_preset = ac_preset_to_esphome_preset(state->get(AcState::AC_ECO), state->get(AcState::AC_TURBO), state->get(AcState::AC_SLEEP));
        if (this->preset != new_preset)
        {
            this->preset = new_preset;
            climate_changed = true;
        }

        if (climate_changed)
        {
            publish_state();
        }
    }

    void send_pending_state()
    {
        if (!this->has_state_ack || !this->ac_state_pending)
            return;

        // prevents sending a 2nd command while waiting for response from first
        this->has_state_ack = 0;

        // copy and reset the pending state; ac_state_pending will now be null
        AcState *state_pending = this->ac_state_pending.release();

        byte AcCmd[COMMAND_LENGTH] = {};
        populate_command_data(state_pending, AcCmd);
        calculate_and_set_checksum(AcCmd);

        write_array(AcCmd, COMMAND_LENGTH);
        hbeat = millis();
    }

    void populate_command_data(AcState *state, byte *AcCmd)
    {
        const byte modeMap[] = {3, 7, 2, 1, 8};
        const byte fanMap[] = {0, 2, 3, 5, 0};

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

    void calculate_and_set_checksum(byte *AcCmd)
    {
        byte checksum = 0;
        for (byte i = 0; i < COMMAND_LENGTH - 1; i++)
        {
            checksum ^= AcCmd[i];
        }
        AcCmd[COMMAND_LENGTH - 1] = checksum;
    }

    void send_heartbeat_if_required()
    {
        if (is_time_elapsed(hbeat, 2000))
        {
            const byte heartbeat[] = {0xBB, 0x00, 0x01, 0x04, 0x02, 0x01, 0x00, 0xBD};
            write_array(heartbeat, sizeof(heartbeat));
        }
    }

    // convert ac_mode and ac_power to valid ClimateMode
    // ac_mode: 0=cool 1=fan  2=dry 3=heat 4=auto
    ClimateMode ac_mode_to_esphome_mode(uint8_t ac_mode, bool ac_power)
    {
        if (!ac_power)
        {
            return CLIMATE_MODE_OFF;
        }
        else if (ac_mode == 0)
        {
            return CLIMATE_MODE_COOL;
        }
        else if (ac_mode == 1)
        {
            return CLIMATE_MODE_FAN_ONLY;
        }
        else if (ac_mode == 2)
        {
            return CLIMATE_MODE_DRY;
        }
        else if (ac_mode == 3)
        {
            return CLIMATE_MODE_HEAT;
        }
        else if (ac_mode == 4)
        {
            return CLIMATE_MODE_HEAT_COOL;
        }
        else
        {
            return CLIMATE_MODE_HEAT_COOL;
        }
    }

    // convert ClimateMode to ac_mode and ac_power
    void esphome_mode_to_ac_mode(ClimateMode mode, uint8_t &ac_mode, bool &ac_power)
    {
        if (mode == CLIMATE_MODE_OFF)
        {
            ac_mode = 0;
            ac_power = false;
        }
        else if (mode == CLIMATE_MODE_COOL)
        {
            ac_mode = 0;
            ac_power = true;
        }
        else if (mode == CLIMATE_MODE_FAN_ONLY)
        {
            ac_mode = 1;
            ac_power = true;
        }
        else if (mode == CLIMATE_MODE_DRY)
        {
            ac_mode = 2;
            ac_power = true;
        }
        else if (mode == CLIMATE_MODE_HEAT)
        {
            ac_mode = 3;
            ac_power = true;
        }
        else if (mode == CLIMATE_MODE_HEAT_COOL)
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
    ClimateFanMode ac_fan_mode_to_esphome_fan_mode(uint8_t ac_fan)
    {
        if (ac_fan == 0)
        {
            return CLIMATE_FAN_AUTO;
        }
        else if (ac_fan == 1)
        {
            return CLIMATE_FAN_LOW;
        }
        else if (ac_fan == 2)
        {
            return CLIMATE_FAN_MEDIUM;
        }
        else if (ac_fan == 3)
        {
            return CLIMATE_FAN_HIGH;
        }
        else
        {
            return CLIMATE_FAN_AUTO;
        }
    }

    uint8_t esphome_fan_mode_to_ac_fan_mode(ClimateFanMode fan_mode)
    {
        if (fan_mode == CLIMATE_FAN_AUTO)
        {
            return 0;
        }
        else if (fan_mode == CLIMATE_FAN_LOW)
        {
            return 1;
        }
        else if (fan_mode == CLIMATE_FAN_MEDIUM)
        {
            return 2;
        }
        else if (fan_mode == CLIMATE_FAN_HIGH)
        {
            return 3;
        }
        else
        {
            return 0;
        }
    }

    ClimateSwingMode ac_swing_mode_to_esphome_swing_mode(uint8_t ac_h_swing, uint8_t ac_v_swing)
    {
        if (ac_h_swing && ac_v_swing)
        {
            return CLIMATE_SWING_BOTH;
        }
        else if (ac_h_swing)
        {
            return CLIMATE_SWING_HORIZONTAL;
        }
        else if (ac_v_swing)
        {
            return CLIMATE_SWING_VERTICAL;
        }
        else
        {
            return CLIMATE_SWING_OFF;
        }
    }

    void esphome_swing_mode_to_ac_swing_mode(ClimateSwingMode swing_mode, uint8_t &ac_h_swing, uint8_t &ac_v_swing)
    {
        if (swing_mode == CLIMATE_SWING_BOTH)
        {
            ac_h_swing = 1;
            ac_v_swing = 1;
        }
        else if (swing_mode == CLIMATE_SWING_HORIZONTAL)
        {
            ac_h_swing = 1;
            ac_v_swing = 0;
        }
        else if (swing_mode == CLIMATE_SWING_VERTICAL)
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

    ClimateAction ac_action_to_esphome_action(uint8_t ac_action)
    {
        if (ac_action == 0x8A) // 138
        {
            return CLIMATE_ACTION_COOLING;
        }
        else if (ac_action == 0xCA) // 202
        {
            return CLIMATE_ACTION_HEATING;
        }
        else if (ac_action == 0x80) // 128
        {
            return CLIMATE_ACTION_IDLE;
        }
        else if (ac_action == 0xC0) // 192 (2 min Compressor Cooldown Period)
        {
            return CLIMATE_ACTION_IDLE;
        }
        else
        {
            return CLIMATE_ACTION_IDLE;
        }
    }

    ClimatePreset ac_preset_to_esphome_preset(bool ac_eco, bool ac_turbo, bool ac_sleep)
    {
        if (ac_eco)
        {
            return CLIMATE_PRESET_ECO;
        }
        else if (ac_turbo)
        {
            return CLIMATE_PRESET_BOOST;
        }
        else if (ac_sleep)
        {
            return CLIMATE_PRESET_SLEEP;
        }
        else
        {
            return CLIMATE_PRESET_NONE;
        }
    }

    void esphome_preset_to_ac_preset(ClimatePreset preset, bool &ac_eco, bool &ac_turbo, bool &ac_sleep)
    {
        if (preset == CLIMATE_PRESET_ECO)
        {
            ac_eco = true;
            ac_turbo = false;
            ac_sleep = false;
        }
        else if (preset == CLIMATE_PRESET_BOOST)
        {
            ac_eco = false;
            ac_turbo = true;
            ac_sleep = false;
        }
        else if (preset == CLIMATE_PRESET_SLEEP)
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

    std::string
    ac_motor_speed_to_string(uint8_t speed)
    {
        switch (speed)
        {
        case MOTOR_SPEED_OFF:
            return "OFF";
        case MOTOR_SPEED_LOW:
            return "LOW";
        case MOTOR_SPEED_HIGH:
            return "HIGH";
        default:
            return "UNKNOWN";
        }
    }
};

void PioneerMinisplitSwitch::write_state(bool state)
{
    // Notify the parent PioneerMinisplit instance about the state change
    parent_->on_switch_write_state(this->name_, state);
}