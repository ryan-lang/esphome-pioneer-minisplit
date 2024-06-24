from esphome.components import binary_sensor
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import ICON_VIBRATE, ICON_BRIGHTNESS_5
from .. import pioneer_minisplit_ns, CONF_PIONEER_MINISPLIT_ID, PioneerMinisplit

DEPENDENCIES = ["pioneer_minisplit"]
CODEOWNERS = ["@ryan-lang"]

CONF_DEEP_SLEEP = "deep_sleep"
CONF_SLEEP = "sleep"
CONF_ECO = "eco"
CONF_TURBO = "turbo"

PioneerMinisplitBinarySensor = pioneer_minisplit_ns.class_("PioneerMinisplitBinarySensor", binary_sensor.BinarySensor, cg.Component)
PioneerMinisplitBinarySensorPurpose = pioneer_minisplit_ns.enum("PioneerMinisplitBinarySensorPurpose")

CONFIG_SCHEMA = (
    cv.Schema({
        cv.GenerateID(CONF_PIONEER_MINISPLIT_ID): cv.use_id(PioneerMinisplit),
        cv.Optional(CONF_DEEP_SLEEP): binary_sensor.binary_sensor_schema(PioneerMinisplitBinarySensor, icon=ICON_BRIGHTNESS_5).extend(cv.COMPONENT_SCHEMA),
        cv.Optional(CONF_SLEEP): binary_sensor.binary_sensor_schema(PioneerMinisplitBinarySensor, icon=ICON_VIBRATE).extend(cv.COMPONENT_SCHEMA),
        cv.Optional(CONF_ECO): binary_sensor.binary_sensor_schema(PioneerMinisplitBinarySensor, icon=ICON_VIBRATE).extend(cv.COMPONENT_SCHEMA),
        cv.Optional(CONF_TURBO): binary_sensor.binary_sensor_schema(PioneerMinisplitBinarySensor, icon=ICON_VIBRATE).extend(cv.COMPONENT_SCHEMA),
    })
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_PIONEER_MINISPLIT_ID])
    if config_deep_sleep := config.get(CONF_DEEP_SLEEP):
        var = await binary_sensor.new_binary_sensor(config_deep_sleep, parent, PioneerMinisplitBinarySensorPurpose.DEEP_SLEEP)
        await cg.register_component(var, config_deep_sleep)

    if config_sleep := config.get(CONF_SLEEP):
        var = await binary_sensor.new_binary_sensor(config_sleep, parent, PioneerMinisplitBinarySensorPurpose.SLEEP)
        await cg.register_component(var, config_sleep)

    if config_eco := config.get(CONF_ECO):
        var = await binary_sensor.new_binary_sensor(config_eco, parent, PioneerMinisplitBinarySensorPurpose.ECO)
        await cg.register_component(var, config_eco)

    if config_turbo := config.get(CONF_TURBO):
        var = await binary_sensor.new_binary_sensor(config_turbo, parent, PioneerMinisplitBinarySensorPurpose.TURBO)
        await cg.register_component(var, config_turbo)