/**
 * @file   oo.h
 * @brief  Defines the base class public interface for object-oriented modules.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef OO_DEFINED
#define OO_DEFINED

#include <stdarg.h>

/**
 * The super class of all objects.
 */
extern void *Object;

/**
 * The metaclass of all objects.
 */
extern void *Class;

extern void *new(const void *Object, ...);
extern void delete(const void *Object);
extern void size_of(const void *Object);
extern void init_object(const void *Object);

#endif
