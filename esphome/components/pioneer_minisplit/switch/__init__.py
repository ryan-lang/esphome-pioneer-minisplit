from esphome.components import switch
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import ICON_VIBRATE, ICON_BRIGHTNESS_5
from .. import pioneer_minisplit_ns, CONF_PIONEER_MINISPLIT_ID, PioneerMinisplit

DEPENDENCIES = ["pioneer_minisplit"]
CODEOWNERS = ["@ryan-lang"]

CONF_DISPLAY = "display"
CONF_BEEP = "beep"

PioneerMinisplitSwitch = pioneer_minisplit_ns.class_("PioneerMinisplitSwitch", switch.Switch, cg.Component)
PioneerMinisplitSwitchPurpose = pioneer_minisplit_ns.enum("PioneerMinisplitSwitchPurpose")

CONFIG_SCHEMA = (
    cv.Schema({
        cv.GenerateID(CONF_PIONEER_MINISPLIT_ID): cv.use_id(PioneerMinisplit),
        cv.Optional(CONF_DISPLAY): switch.switch_schema(PioneerMinisplitSwitch, icon=ICON_BRIGHTNESS_5).extend(cv.COMPONENT_SCHEMA),
        cv.Optional(CONF_BEEP): switch.switch_schema(PioneerMinisplitSwitch, icon=ICON_VIBRATE, default_restore_mode="RESTORE_DEFAULT_ON").extend(cv.COMPONENT_SCHEMA),
    })
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_PIONEER_MINISPLIT_ID])
    if config_display := config.get(CONF_DISPLAY):
        var = await switch.new_switch(config_display, parent, PioneerMinisplitSwitchPurpose.DISPLAY)
        await cg.register_component(var, config_display)

    if config_beep := config.get(CONF_BEEP):
        var = await switch.new_switch(config_beep, parent, PioneerMinisplitSwitchPurpose.BEEP)
        await cg.register_component(var, config_beep)
    