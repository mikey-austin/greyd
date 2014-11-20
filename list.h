/**
 * @file   list.h
 * @brief  Defines an abstract list data structure.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef LIST_DEFINED
#define LIST_DEFINED

#include <stdlib.h>

#define LIST_FOREACH(list, curr) \
    for(curr = list->head; curr != NULL; curr = curr->next)

/**
 * A struct to contain a list entry's data and links.
 */
struct List_entry_T {
    struct List_entry_T *next; /**< List entry next link. */
    void *v;        /**< List entry value */
};

/**
 * The main list structure.
 */
typedef struct List_T *List_T;
struct List_T {
    int        size;        /**< The list size. */
    void     (*destroy)(void *value);
    struct List_entry_T  *head;
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
extern void *List_entry_value(struct List_entry_T *entry);

#endif
