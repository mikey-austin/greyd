/**
 * @file   oo.h
 * @brief  Defines the base object selector functions.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef OO_DEFINED
#define OO_DEFINED

#include <stdlib.h>
#include <stdarg.h>

extern void       *OO_new(const void *class, ...);
extern void        OO_delete(void *self);
extern int         OO_equals(void *self, void *b);
extern const void *OO_class_of(const void *_self);
extern size_t      OO_size_of(const void *self);
extern const void *OO_super(const void *class);

#endif
