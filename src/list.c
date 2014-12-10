/**
 * @file   list.c
 * @brief  Implements an abstract list data structure.
 * @author Mikey Austin
 * @date   2014
 */

#include "failures.h"
#include "list.h"

#include <err.h>
#include <stdlib.h>

static struct List_entry *List_create_element(List_T list, void *value);
static void List_destroy_element(List_T list, struct List_entry **element);

extern List_T
List_create(void (*destroy)(void *value))
{
    List_T list;

    if((list = malloc(sizeof(*list))) == NULL)
        errx(1, "Could not create an empty list");

    /* Initialize list pointers. */
    list->destroy = destroy;
    list->size    = 0;
    list->head    = NULL;

    return list;
}

extern void
List_destroy(List_T *list)
{
    if(list == NULL || *list == NULL)
        return;

    List_remove_all(*list);
    free(*list);
    *list = NULL;
}

extern void
List_remove_all(List_T list)
{
    struct List_entry *element, *curr;

    for(curr = list->head; curr != NULL; ) {
        element = curr;
        curr = curr->next;

        /* Destroy the element's value. */
        if(element->v && list->destroy) {
            list->destroy(element->v);
        }

        List_destroy_element(list, &element);
    }

    list->size = 0;
    list->head = NULL;
}

extern void
List_insert_after(List_T list, void *value)
{
    struct List_entry *element, *curr;

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
List_insert_head(List_T list, void *value)
{
    struct List_entry *element;

    element = List_create_element(list, value);
    list->size++;

    element->next = list->head;
    list->head = element;
}

extern void
*List_remove_head(List_T list)
{
    struct List_entry *element;
    void *value;

    if((element = list->head) == NULL) {
        return NULL;
    }

    list->head = element->next;

    value = element->v;
    List_destroy_element(list, &element);
    list->size--;

    return value;
}

extern int
List_size(List_T list)
{
    if(list) {
        return list->size;
    }

    return 0;
}

extern void
*List_entry_value(struct List_entry *entry)
{
    if(entry == NULL)
        return NULL;

    return entry->v;
}

static struct List_entry
*List_create_element(List_T list, void *value)
{
    struct List_entry *element;

    if((element = malloc(sizeof(*element))) == NULL)
        errx(1, "Could not initialize list element");

    element->next = NULL;
    element->v    = value;

    return element;
}

static void
List_destroy_element(List_T list, struct List_entry **element)
{
    if(element && *element) {
        free(*element);
        *element = NULL;
    }
}
