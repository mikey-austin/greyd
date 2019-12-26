/*
 * Copyright (c) 2014, 2015 Mikey Austin <mikey@greyd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * @file   list.c
 * @brief  Implements an abstract list data structure.
 * @author Mikey Austin
 * @date   2014
 */

#include "list.h"
#include "failures.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

static struct List_entry* List_create_element(List_T list, void* value);
static void List_destroy_element(List_T list, struct List_entry** element);

extern List_T
List_create(void (*destroy)(void* value))
{
    List_T list;

    if ((list = malloc(sizeof(*list))) == NULL)
        i_critical("Could not create an empty list: %s", strerror(errno));

    /* Initialize list pointers. */
    list->destroy = destroy;
    list->size = 0;
    list->head = NULL;

    return list;
}

extern void
List_destroy(List_T* list)
{
    if (list == NULL || *list == NULL)
        return;

    List_remove_all(*list);
    free(*list);
    *list = NULL;
}

extern void
List_remove_all(List_T list)
{
    struct List_entry *element, *curr;

    for (curr = list->head; curr != NULL;) {
        element = curr;
        curr = curr->next;

        /* Destroy the element's value. */
        if (element->v && list->destroy) {
            list->destroy(element->v);
        }

        List_destroy_element(list, &element);
    }

    list->size = 0;
    list->head = NULL;
}

extern void
List_insert_after(List_T list, void* value)
{
    struct List_entry *element, *curr;

    element = List_create_element(list, value);
    list->size++;

    if (list->head) {
        for (curr = list->head; curr != NULL; curr = curr->next) {
            if (curr->next == NULL) {
                curr->next = element;
                break;
            }
        }
    } else {
        list->head = element;
    }
}

extern void
List_insert_head(List_T list, void* value)
{
    struct List_entry* element;

    element = List_create_element(list, value);
    list->size++;

    element->next = list->head;
    list->head = element;
}

extern void* List_remove_head(List_T list)
{
    struct List_entry* element;
    void* value;

    if ((element = list->head) == NULL) {
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
    if (list) {
        return list->size;
    }

    return 0;
}

extern void* List_entry_value(struct List_entry* entry)
{
    if (entry == NULL)
        return NULL;

    return entry->v;
}

static struct List_entry* List_create_element(List_T list, void* value)
{
    struct List_entry* element;

    if ((element = malloc(sizeof(*element))) == NULL)
        i_critical("Could not initialize list element: %s",
            strerror(errno));

    element->next = NULL;
    element->v = value;

    return element;
}

static void
List_destroy_element(List_T list, struct List_entry** element)
{
    if (element && *element) {
        free(*element);
        *element = NULL;
    }
}
