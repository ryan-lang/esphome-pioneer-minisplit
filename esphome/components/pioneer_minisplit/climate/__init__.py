from esphome.components import climate, sensor
import esphome.config_validation as cv
import esphome.codegen as cg
from .. import pioneer_minisplit_ns, CONF_PIONEER_MINISPLIT_ID, PioneerMinisplit
from esphome.const import (
    CONF_COOL_DEADBAND,
    CONF_COOL_OVERRUN,
    CONF_HEAT_DEADBAND,
    CONF_HEAT_OVERRUN,
    CONF_ID,
    CONF_SENSOR,
)

CONF_SENSOR_TIMEOUT = "sensor_timeout"
CONF_MIN_COMMAND_INTERVAL = "min_command_interval"
CONF_THROTTLE_GAIN = "throttle_gain"
CONF_MAX_THROTTLE = "max_throttle"
CONF_SETTLE_TIME = "settle_time"

DEPENDENCIES = ["pioneer_minisplit"]
CODEOWNERS = ["@ryan-lang"]

PioneerMinisplitClimate = pioneer_minisplit_ns.class_("PioneerMinisplitClimate", cg.Component, climate.Climate)

CONFIG_SCHEMA = cv.All(
    climate.climate_schema(PioneerMinisplitClimate).extend(
        {
            cv.GenerateID(CONF_PIONEER_MINISPLIT_ID): cv.use_id(PioneerMinisplit),
            # Hysteresis for the advanced heat/cool mode, where this component drives the
            # unit's mode and setpoint itself. Wider bands mean fewer commands to the unit,
            # which means fewer display flashes and less compressor cycling.
            cv.Optional(CONF_COOL_DEADBAND, default=0.5): cv.temperature_delta,
            cv.Optional(CONF_COOL_OVERRUN, default=0.5): cv.temperature_delta,
            cv.Optional(CONF_HEAT_DEADBAND, default=0.5): cv.temperature_delta,
            cv.Optional(CONF_HEAT_OVERRUN, default=0.5): cv.temperature_delta,
            # Remote room thermometer. When present and fresh it replaces the unit's own
            # return-air reading as the control input, and the setpoint we send the unit stops
            # being a setpoint and becomes a capacity request (see compute_setpoint_).
            cv.Optional(CONF_SENSOR): cv.use_id(sensor.Sensor),
            # How long a remote reading stays trustworthy. On expiry we fall back to the
            # unit's internal sensor and hand the setpoint back to its own thermostat.
            cv.Optional(
                CONF_SENSOR_TIMEOUT, default="5min"
            ): cv.positive_time_period_milliseconds,
            # Every setpoint command lights the unit's display, so rate-limit them. Mode
            # changes (the actual on/off decision) are never rate-limited.
            cv.Optional(
                CONF_MIN_COMMAND_INTERVAL, default="60s"
            ): cv.positive_time_period_milliseconds,
            # Capacity request per degree of room error, and its ceiling. delta =
            # clamp(round(error * gain), 1, max). Defaults give 1C of throttle at ~0.3C of
            # error, 2C at ~0.8C, saturating at 4C by ~1.5C of error.
            cv.Optional(CONF_THROTTLE_GAIN, default=2.5): cv.positive_float,
            cv.Optional(CONF_MAX_THROTTLE, default=4): cv.int_range(min=1, max=15),
            # After the unit starts, its own sensor spends a few minutes reacting to its fan
            # rather than to the room. The setpoint is held over that window instead of
            # chasing a reading that is measuring the fan.
            cv.Optional(
                CONF_SETTLE_TIME, default="180s"
            ): cv.positive_time_period_milliseconds,
        }
    ).extend(cv.COMPONENT_SCHEMA)
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_PIONEER_MINISPLIT_ID])

    var = cg.new_Pvariable(config[CONF_ID], parent)
    await cg.register_component(var, config)
    await climate.register_climate(var, config)

    cg.add(var.set_cool_deadband(config[CONF_COOL_DEADBAND]))
    cg.add(var.set_cool_overrun(config[CONF_COOL_OVERRUN]))
    cg.add(var.set_heat_deadband(config[CONF_HEAT_DEADBAND]))
    cg.add(var.set_heat_overrun(config[CONF_HEAT_OVERRUN]))

    if sensor_id := config.get(CONF_SENSOR):
        cg.add(var.set_remote_sensor(await cg.get_variable(sensor_id)))
    cg.add(var.set_sensor_timeout(config[CONF_SENSOR_TIMEOUT]))
    cg.add(var.set_min_command_interval(config[CONF_MIN_COMMAND_INTERVAL]))
    cg.add(var.set_throttle_gain(config[CONF_THROTTLE_GAIN]))
    cg.add(var.set_max_throttle(config[CONF_MAX_THROTTLE]))
    cg.add(var.set_settle_time(config[CONF_SETTLE_TIME]))
