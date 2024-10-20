from esphome.components import climate
import esphome.config_validation as cv
import esphome.codegen as cg
from .. import pioneer_minisplit_ns, CONF_PIONEER_MINISPLIT_ID, PioneerMinisplit
from esphome.const import CONF_ID

DEPENDENCIES = ["pioneer_minisplit"]
CODEOWNERS = ["@ryan-lang"]

PioneerMinisplitClimate = pioneer_minisplit_ns.class_("PioneerMinisplitClimate", cg.Component, climate.Climate)

CONFIG_SCHEMA = cv.All(
    climate.CLIMATE_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(PioneerMinisplitClimate),
            cv.GenerateID(CONF_PIONEER_MINISPLIT_ID): cv.use_id(PioneerMinisplit),
        }
    ).extend(cv.COMPONENT_SCHEMA)
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_PIONEER_MINISPLIT_ID])
    
    var = cg.new_Pvariable(config[CONF_ID], parent)
    await cg.register_component(var, config)
    await climate.register_climate(var, config)