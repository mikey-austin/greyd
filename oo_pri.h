/**
 * @file   oo_pri.h
 * @brief  Defines the base class protected representation.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef OO_PRI_DEFINED
#define OO_PRI_DEFINED

#include <stdlib.h>

struct Object {
    struct Class *class;
    size_t       size;
};

struct Class {
    size_t             size;
    const void         *super;
    const char         *name;
    void *(*construct) (const void *self, ...);
    void *(*destroy)   (const void *self);
    void *(*equals)    (const void *self, const void *b);
};

#endif
