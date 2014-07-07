/**
 * @file   config_value.c
 * @brief  Implements configuration variable value.
 * @author Mikey Austin
 * @date   2014
 */

#include "failures.h"
#include "config_value.h"

#include <stdlib.h>
#include <string.h>

#define T Config_value_T

extern T
Config_value_create(short type)
{
    T new;

    new = (T) malloc(sizeof(*new));
    if(new == NULL) {
        I_CRIT("Could not create new config value");
    }

    new->type = type;

    return new;
}

extern void
Config_value_set_int(T value, int data)
{
    value->type = CONFIG_VAL_TYPE_INT;
    value->v.i  = data;
}

extern void
Config_value_set_str(T value, const char *data)
{
    value->type = CONFIG_VAL_TYPE_STR;
    value->v.s = (char *) malloc(strlen(data) + 1);
    if(value->v.s == NULL) {
        I_CRIT("Could not create string config value");
    }

    strncpy(value->v.s, data, strlen(data));
    *(value->v.s + strlen(data)) = '\0';
}

extern void
Config_value_destroy(T value)
{
    if(!value)
        return;

    switch(value->type) {
    case CONFIG_VAL_TYPE_STR:
        if(value->v.s != NULL) {
            free(value->v.s);
            value->v.s = NULL;
        }
        break;
    }

    free(value);
    value = NULL;
}

#undef T
