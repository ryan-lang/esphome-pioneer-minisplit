from esphome.components import sensor
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import ICON_VIBRATE, ICON_BRIGHTNESS_5
from .. import pioneer_minisplit_ns, CONF_PIONEER_MINISPLIT_ID, PioneerMinisplit

DEPENDENCIES = ["pioneer_minisplit"]
CODEOWNERS = ["@ryan-lang"]

CONF_UNKNOWN_1 = "unknown_1"
CONF_FAN_SPEED_ACTUAL = "fan_speed_actual"
CONF_TEMP_PIPE_OUT = "temp_pipe_out"
CONF_TEMP_PIPE_IN = "temp_pipe_in"
CONF_UNKNOWN_2 = "unknown_2"
CONF_UNKNOWN_3 = "unknown_3"
CONF_COMPRESSOR_CURRENT = "compressor_current"
CONF_MOTOR = "motor"
CONF_SWING_V_POS = "swing_v_pos"
CONF_SWING_H_POS = "swing_h_pos"
CONF_FAULT = "fault"
CONF_CLEAN_FILTER = "clean_filter"
CONF_HEALTH = "health"
CONF_SUPPLY_VOLTAGE = "supply_voltage"

PioneerMinisplitSensor = pioneer_minisplit_ns.class_("PioneerMinisplitSensor", sensor.Sensor, cg.Component)
PioneerMinisplitSensorPurpose = pioneer_minisplit_ns.enum("PioneerMinisplitSensorPurpose")

CONFIG_SCHEMA = (
    cv.Schema({
        cv.GenerateID(CONF_PIONEER_MINISPLIT_ID): cv.use_id(PioneerMinisplit),
        cv.Optional(CONF_UNKNOWN_1): sensor.sensor_schema(PioneerMinisplitSensor, state_class="measurement").extend(cv.COMPONENT_SCHEMA),
        cv.Optional(CONF_FAN_SPEED_ACTUAL): sensor.sensor_schema(PioneerMinisplitSensor, icon="mdi:fan", state_class="measurement").extend(cv.COMPONENT_SCHEMA),
        cv.Optional(CONF_TEMP_PIPE_OUT): sensor.sensor_schema(PioneerMinisplitSensor, icon="mdi:pipe", state_class="measurement", unit_of_measurement="°C").extend(cv.COMPONENT_SCHEMA),
        cv.Optional(CONF_TEMP_PIPE_IN): sensor.sensor_schema(PioneerMinisplitSensor, icon="mdi:pipe", state_class="measurement", unit_of_measurement="°C").extend(cv.COMPONENT_SCHEMA),
        cv.Optional(CONF_UNKNOWN_2): sensor.sensor_schema(PioneerMinisplitSensor, state_class="measurement").extend(cv.COMPONENT_SCHEMA),
        cv.Optional(CONF_UNKNOWN_3): sensor.sensor_schema(PioneerMinisplitSensor, state_class="measurement").extend(cv.COMPONENT_SCHEMA),
        cv.Optional(CONF_COMPRESSOR_CURRENT): sensor.sensor_schema(PioneerMinisplitSensor, icon="mdi:current-ac", state_class="measurement", device_class="current", unit_of_measurement="A", accuracy_decimals=1).extend(cv.COMPONENT_SCHEMA),
        cv.Optional(CONF_MOTOR): sensor.sensor_schema(PioneerMinisplitSensor, icon="mdi:heat-pump", state_class="measurement").extend(cv.COMPONENT_SCHEMA),
        cv.Optional(CONF_SWING_V_POS): sensor.sensor_schema(PioneerMinisplitSensor, icon="mdi:arrow-up-down").extend(cv.COMPONENT_SCHEMA),
        cv.Optional(CONF_SWING_H_POS): sensor.sensor_schema(PioneerMinisplitSensor, icon="mdi:arrow-left-right").extend(cv.COMPONENT_SCHEMA),
        cv.Optional(CONF_FAULT): sensor.sensor_schema(PioneerMinisplitSensor, icon="mdi:alert-circle").extend(cv.COMPONENT_SCHEMA),
        cv.Optional(CONF_CLEAN_FILTER): sensor.sensor_schema(PioneerMinisplitSensor, icon="mdi:air-filter").extend(cv.COMPONENT_SCHEMA),
        cv.Optional(CONF_HEALTH): sensor.sensor_schema(PioneerMinisplitSensor, icon="mdi:medical-bag").extend(cv.COMPONENT_SCHEMA),
        cv.Optional(CONF_SUPPLY_VOLTAGE): sensor.sensor_schema(PioneerMinisplitSensor, state_class="measurement", device_class="voltage", unit_of_measurement="V").extend(cv.COMPONENT_SCHEMA),
    })
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_PIONEER_MINISPLIT_ID])
    if config_unknown_1 := config.get(CONF_UNKNOWN_1):
        var = await sensor.new_sensor(config_unknown_1, parent, PioneerMinisplitSensorPurpose.UNKNOWN_1)
        await cg.register_component(var, config_unknown_1)

    if config_fan_speed_actual := config.get(CONF_FAN_SPEED_ACTUAL):
        var = await sensor.new_sensor(config_fan_speed_actual, parent, PioneerMinisplitSensorPurpose.FAN_SPEED_ACTUAL)
        await cg.register_component(var, config_fan_speed_actual)
    
    if config_temp_pipe_out := config.get(CONF_TEMP_PIPE_OUT):
        var = await sensor.new_sensor(config_temp_pipe_out, parent, PioneerMinisplitSensorPurpose.TEMP_PIPE_OUT)
        await cg.register_component(var, config_temp_pipe_out)

    if config_temp_pipe_in := config.get(CONF_TEMP_PIPE_IN):
        var = await sensor.new_sensor(config_temp_pipe_in, parent, PioneerMinisplitSensorPurpose.TEMP_PIPE_IN)
        await cg.register_component(var, config_temp_pipe_in)

    if config_unknown_2 := config.get(CONF_UNKNOWN_2):
        var = await sensor.new_sensor(config_unknown_2, parent, PioneerMinisplitSensorPurpose.UNKNOWN_2)
        await cg.register_component(var, config_unknown_2)

    if config_unknown_3 := config.get(CONF_UNKNOWN_3):
        var = await sensor.new_sensor(config_unknown_3, parent, PioneerMinisplitSensorPurpose.UNKNOWN_3)
        await cg.register_component(var, config_unknown_3)

    if config_compressor_current := config.get(CONF_COMPRESSOR_CURRENT):
        var = await sensor.new_sensor(config_compressor_current, parent, PioneerMinisplitSensorPurpose.COMPRESSOR_CURRENT)
        await cg.register_component(var, config_compressor_current)

    if config_motor := config.get(CONF_MOTOR):
        var = await sensor.new_sensor(config_motor, parent, PioneerMinisplitSensorPurpose.MOTOR)
        await cg.register_component(var, config_motor)

    if config_swing_v_pos := config.get(CONF_SWING_V_POS):
        var = await sensor.new_sensor(config_swing_v_pos, parent, PioneerMinisplitSensorPurpose.SWING_V_POS)
        await cg.register_component(var, config_swing_v_pos)

    if config_swing_h_pos := config.get(CONF_SWING_H_POS):
        var = await sensor.new_sensor(config_swing_h_pos, parent, PioneerMinisplitSensorPurpose.SWING_H_POS)
        await cg.register_component(var, config_swing_h_pos)

    if config_fault := config.get(CONF_FAULT):
        var = await sensor.new_sensor(config_fault, parent, PioneerMinisplitSensorPurpose.FAULT)
        await cg.register_component(var, config_fault)

    if config_clean_filter := config.get(CONF_CLEAN_FILTER):
        var = await sensor.new_sensor(config_clean_filter, parent, PioneerMinisplitSensorPurpose.CLEAN_FILTER)
        await cg.register_component(var, config_clean_filter)

    if config_health := config.get(CONF_HEALTH):
        var = await sensor.new_sensor(config_health, parent, PioneerMinisplitSensorPurpose.HEALTH)
        await cg.register_component(var, config_health)

    if config_supply_voltage := config.get(CONF_SUPPLY_VOLTAGE):
        var = await sensor.new_sensor(config_supply_voltage, parent, PioneerMinisplitSensorPurpose.SUPPLY_VOLTAGE)
        await cg.register_component(var, config_supply_voltage)
    