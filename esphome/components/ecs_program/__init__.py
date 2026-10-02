import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, time, udp
from esphome.const import CONF_ID, CONF_TIME_ID

ecs_program_ns = cg.esphome_ns.namespace("ecs_program")
ECSProgram = ecs_program_ns.class_("ECSProgram", cg.Component)

CONF_UDP_ID = "udp_id"
CONF_CLIMATE_ID = "climate_id"
CONF_TIMEZONE = "timezone"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(ECSProgram),
    cv.Required(CONF_UDP_ID): cv.use_id(udp.UDPComponent),
    cv.Required(CONF_TIME_ID): cv.use_id(time.RealTimeClock),
    cv.Required(CONF_CLIMATE_ID): cv.use_id(climate.Climate),
    cv.Optional(CONF_TIMEZONE, default=""): cv.string,
}).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    udp_var = await cg.get_variable(config[CONF_UDP_ID])
    time_var = await cg.get_variable(config[CONF_TIME_ID])
    climate_var = await cg.get_variable(config[CONF_CLIMATE_ID])
    cg.add(var.set_udp(udp_var))
    cg.add(var.set_time(time_var))
    cg.add(var.set_climate(climate_var))
    if config[CONF_TIMEZONE]:
        cg.add(var.set_timezone(config[CONF_TIMEZONE]))
