/**
 * @file   object.c
 * @brief  Implements the base object functionality.
 * @author Mikey Austin
 * @date   2014
 */

#include "object.h"
#include "object_pri.h"

extern void
*Object_construct(void *self, va_list *args)
{
    /*
     * For the base object, just return self, as this trivial object
     * is completely created by OO_new.
     */
    return self;
}

extern void
*Object_destruct(void *self)
{
    /*
     * In a similar fashion to construct, OO_delete takes care so
     * just return self.
     */
    return self;
}

extern int
Object_equals(const void *self, const void *b)
{
    /* The objects are equal if the two argument pointers are the same */
    return (self == b);
}
