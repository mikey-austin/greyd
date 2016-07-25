#include "failures.h"
#include "plugin.h"
#include "greyd_config.h"

static void load(Config_T);
static void unload();

static void
load(Config_T config)
{
    i_info("in dummy load");
}

static void
unload()
{
    i_info("in dummy unload");
}
