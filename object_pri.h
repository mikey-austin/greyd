/**
 * @file   object_pri.h
 * @brief  Defines the private Object and Class representations.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef OBJECT_PRI_DEFINED
#define OBJECT_PRI_DEFINED

#include <stdlib.h>
#include <stdarg.h>

/* The trivial base object. */
struct Object {
    struct Class *class;
};

/* The first meta class description is a sub-class of Object. */
struct Class {
    const struct Object  _;
    const char           *name;
    const struct Class   *super;
    size_t                size;
    void *(*__construct) (void *self, va_list *args);
    void *(*__destroy)   (void *self);
    int   (*__equals)    (const void *self, const void *b);
};

#endif
