/**
 * @file   oo.c
 * @brief  Implements the base class interface.
 * @author Mikey Austin
 * @date   2014
 */

#include "oo.h"
#include "oo_pri.h"
#include "failures.h"

#include <stdlib.h>

void
*new(const void *_class, ...)
{
    const struct Class *class = _class;
    void *obj = calloc(1, class->size);

    if(!obj) {
        I_CRIT("Could not instantiate %s object", class->name);
    }

    return NULL;
}
