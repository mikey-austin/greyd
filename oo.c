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
*OO_new(const void *_class, ...)
{
    const struct Class *class = _class;
    void *obj = calloc(1, class->size);

    if(!obj) {
        I_CRIT("Could not instantiate %s object", class->name);
    }

    /* Ensure the class pointer is at the beginning of the new object. */
    *(const struct Class **) obj = class;

    /* If a constructor exists, call it. */
    if(class->__construct) {
        va_list args;
        va_start(args, _class);
        obj = class->__construct(obj, &args);
        va_end(args);
    }

    return NULL;
}

void
OO_delete(void *self)
{
    const struct Class **cp = self;

    /* Call the object's destructor if it exists. */
    if(self && *cp && (*cp)->__destroy) {
        self = (*cp)->__destroy(self);
    }

    free(self);
}

int
OO_equals(void *self, void *b)
{
    const struct Class **cp  = self;

    if(!self || !*cp || !(*cp)->__equals) {
        I_CRIT("Could not call __equals");
    }

    return (*cp)->__equals(self, b);
}

const void
*OO_class_of(const void *_self)
{
    const struct Object *self = _self;

    if(!self || !self->class) {
        I_CRIT("Could not find object's class");
    }

    return self->class;
}

size_t
OO_size_of(const void *_self)
{
    const struct Class *class = OO_class_of(_self);

    return class->size;
}
