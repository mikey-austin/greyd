/**
 * @file   config_value.c
 * @brief  Implements configuration variable value.
 * @author Mikey Austin
 * @date   2014
 */

#include "utils.h"
#include "failures.h"
#include "config_value.h"

#include <stdlib.h>
#include <string.h>

/**
 * The destructor for config value lists.
 */
static void Config_value_list_entry_destroy(void *value);

extern Config_value_T
Config_value_create(short type)
{
    Config_value_T new;

    new = malloc(sizeof(*new));
    if(new == NULL) {
        I_CRIT("Could not create new config value");
    }
    else {
        new->type = type;
        switch(new->type) {
        case CONFIG_VAL_TYPE_LIST:
            new->v.l = List_create(Config_value_list_entry_destroy);
            break;
        }
    }

    return new;
}

extern void
Config_value_set_int(Config_value_T value, int data)
{
    value->type = CONFIG_VAL_TYPE_INT;
    value->v.i  = data;
}

extern void
Config_value_set_str(Config_value_T value, const char *data)
{
    int slen = strlen(data) + 1;

    value->type = CONFIG_VAL_TYPE_STR;
    value->v.s = malloc(slen);
    if(value->v.s == NULL) {
        I_CRIT("Could not create string config value");
    }
    sstrncpy(value->v.s, data, slen);
}

extern void
Config_value_destroy(Config_value_T *value)
{
    if(value == NULL || *value == NULL)
        return;

    switch((*value)->type) {
    case CONFIG_VAL_TYPE_STR:
        if((*value)->v.s != NULL) {
            free((*value)->v.s);
            (*value)->v.s = NULL;
        }
        break;

    case CONFIG_VAL_TYPE_LIST:
        if((*value)->v.l != NULL) {
            List_destroy((List_T *) &((*value)->v.l));
            (*value)->v.l = NULL;
        }
        break;
    }

    free(*value);
    *value = NULL;
}

extern char
*cv_str(Config_value_T value)
{
    return (value && value->type == CONFIG_VAL_TYPE_STR
            ? value->v.s : NULL);
}

extern int
cv_int(Config_value_T value)
{
    return (value && value->type == CONFIG_VAL_TYPE_INT
            ? value->v.i : -1);
}

extern List_T
cv_list(Config_value_T value)
{
    return (value && value->type == CONFIG_VAL_TYPE_LIST
            ? value->v.l : NULL);
}

extern int
cv_type(Config_value_T value)
{
    return value->type;
}

static void
Config_value_list_entry_destroy(void *value)
{
    Config_value_destroy((Config_value_T *) &value);
}
