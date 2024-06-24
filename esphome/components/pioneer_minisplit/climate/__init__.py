from esphome.components import climate
import esphome.config_validation as cv
import esphome.codegen as cg
from .. import pioneer_minisplit_ns, CONF_PIONEER_MINISPLIT_ID, PioneerMinisplit
from esphome.const import (
    CONF_OUTPUT_ID
)

DEPENDENCIES = ["pioneer_minisplit"]
CODEOWNERS = ["@ryan-lang"]

CONF_CLIMATE = "climate"

PioneerMinisplitClimate = pioneer_minisplit_ns.class_("PioneerMinisplitClimate", climate.Climate, cg.Component)

CONFIG_SCHEMA = cv.All(climate.CLIMATE_SCHEMA.extend({
    cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(PioneerMinisplitClimate),
    cv.GenerateID(CONF_PIONEER_MINISPLIT_ID): cv.use_id(PioneerMinisplit),
}).extend(cv.COMPONENT_SCHEMA))


async def to_code(config):
    parent = await cg.get_variable(config[CONF_PIONEER_MINISPLIT_ID])

    var = cg.new_Pvariable(config[CONF_OUTPUT_ID], parent)
    await cg.register_component(var, config)
    await climate.register_climate(var, config)