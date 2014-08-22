/**
 * @file   list.c
 * @brief  Implements an abstract list data structure.
 * @author Mikey Austin
 * @date   2014
 */

#include "failures.h"
#include "list.h"

#include <stdlib.h>

#define T List_T
#define E List_entry_T

static struct E *List_create_element(T list, void *value);
static void      List_destroy_element(T list, struct E *element);

extern T
List_create(void (*destroy)(void *value))
{
    T list;

    if((list = (T) malloc(sizeof(*list))) == NULL) {
        I_CRIT("Could not create an empty list");
    }

    /* Initialize list pointers. */
    list->destroy = destroy;
    list->size    = 0;
    list->head    = NULL;

    return list;
}

extern void
List_destroy(T list)
{
    struct E *element, *curr;

    for(curr = list->head; curr != NULL; ) {
        element = curr;
        curr = curr->next;

        /* Destroy the element's value. */
        if(element->v && list->destroy) {
            list->destroy(element->v);
        }

        List_destroy_element(list, element);
    }

    free(list);
}

extern void
List_insert_after(T list, void *value)
{
    struct E *element, *curr;

    element = List_create_element(list, value);
    list->size++;

    if(list->head) {
        for(curr = list->head; curr != NULL; curr = curr->next) {
            if(curr->next == NULL) {
                curr->next = element;
                break;
            }
        }
    }
    else {
        list->head = element;
    }
}

extern void
List_insert_head(T list, void *value)
{
    struct E *element, *curr;

    element = List_create_element(list, value);
    list->size++;

    element->next = list->head;
    list->head = element;
}

extern void
*List_remove_head(T list)
{
    struct E *element;
    void *value;

    if((element = list->head) == NULL) {
        return NULL;
    }

    list->head = element->next;

    value = element->v;
    List_destroy_element(list, element);
    list->size--;

    return value;
}

extern int
List_size(T list)
{
    if(list) {
        return list->size;
    }

    return 0;
}

static struct E
*List_create_element(T list, void *value)
{
    struct E *element;

    if((element = (struct E *) malloc(sizeof(*element))) == NULL) {
        I_CRIT("Could not initialize list element");
    }

    element->next = NULL;
    element->v    = value;

    return element;
}

static void
List_destroy_element(T list, struct E *element)
{
    if(element) {
        free(element);
    }
}

#undef T
#undef E
