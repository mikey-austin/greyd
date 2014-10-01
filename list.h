/**
 * @file   list.h
 * @brief  Defines an abstract list data structure.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef LIST_DEFINED
#define LIST_DEFINED

#include <stdlib.h>

#define T List_T
#define E List_entry_T

#define LIST_FOREACH(list, curr) \
    for(curr = list->head; curr != NULL; curr = curr->next)

/**
 * A struct to contain a list entry's data and links.
 */
struct E {
    struct E *next; /**< List entry next link. */
    void *v;        /**< List entry value */
};

/**
 * The main list structure.
 */
typedef struct T *T;
struct T {
    int        size;        /**< The list size. */
    void     (*destroy)(void *value);
    struct E  *head;
};

/**
 * Create a new list.
 */
extern T List_create(void (*destroy)(void *value));

/**
 * Destroy a list, destroying all elements.
 */
extern void List_destroy(T *list);

/**
 * Insert a new element onto the end of the list.
 */
extern void List_insert_after(T list, void *value);

/**
 * Insert a new element onto the front of the list.
 */
extern void List_insert_head(T list, void *value);

/**
 * Remove the element from the front of the list and return the value.
 */
extern void *List_remove_head(T list);

/**
 * Return the size of the list.
 */
extern int List_size(T list);

/**
 * Return the value of this list entry.
 */
extern void *List_entry_value(struct E *entry);

#undef T
#undef E
#endif
