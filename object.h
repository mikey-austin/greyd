/**
 * @file   object.h
 * @brief  Defines base object.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef OBJECT_DEFINED
#define OBJECT_DEFINED

#include "oo.h"
#include <stdarg.h>

/**
 * The super class of all objects.
 */
extern void *Object;

/**
 * The metaclass of all objects.
 */
extern void *Class;

extern void *Object_construct(void *self, va_list *args);
extern void *Object_destruct(void *self);
extern int   Object_equals(const void *self, const void *b);

#endif
