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
 * @file   list.h
 * @brief  Defines an abstract list data structure.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef LIST_DEFINED
#define LIST_DEFINED

#include <stdlib.h>

#define LIST_EACH(list, curr) \
    for(curr = list->head; curr != NULL; curr = curr->next)

/**
 * A struct to contain a list entry's data and links.
 */
struct List_entry {
    struct List_entry *next; /**< List entry next link. */
    void *v;                 /**< List entry value */
};

/**
 * The main list structure.
 */
typedef struct List_T *List_T;
struct List_T {
    int size;
    void (*destroy)(void *value);
    struct List_entry  *head;
};

/**
 * Create a new list.
 */
extern List_T List_create(void (*destroy)(void *value));

/**
 * Destroy a list, destroying all elements.
 */
extern void List_destroy(List_T *list);

/**
 * Destroy all elements in a list.
 */
extern void List_remove_all(List_T list);

/**
 * Insert a new element onto the end of the list.
 */
extern void List_insert_after(List_T list, void *value);

/**
 * Insert a new element onto the front of the list.
 */
extern void List_insert_head(List_T list, void *value);

/**
 * Remove the element from the front of the list and return the value.
 */
extern void *List_remove_head(List_T list);

/**
 * Return the size of the list.
 */
extern int List_size(List_T list);

/**
 * Return the value of this list entry.
 */
extern void *List_entry_value(struct List_entry *entry);

#endif
