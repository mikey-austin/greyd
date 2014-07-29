/**
 * @file   queue.h
 * @brief  Defines an abstract queue data structure.
 * @author Mikey Austin
 * @date   2014
 *
 * This abstract data type is defined by way of a doubly-linked list.
 */

#ifndef QUEUE_DEFINED
#define QUEUE_DEFINED

#define T Queue_T
#define E Queue_entry_T

/**
 * A struct to contain a queue entry's key and value pair.
 */
struct E {
    struct E *next; /**< Queue entry next link. */
    void *v;        /**< Queue entry value */
};

/**
 * The main queue table structure.
 */
typedef struct T *T;
struct T {
    int        size;        /**< The queue size. */
    void     (*destroy)(void *value);
    struct E  *head;
};

/**
 * Create a new queue table.
 */
extern T Queue_create(void (*destroy)(void *value));

/**
 * Destroy a queue, freeing all elements.
 */
extern void Queue_destroy(T queue);

/**
 * Insert a new element onto the end of the queue.
 */
extern void Queue_enqueue(T queue, void *value);

/**
 * Fetch and remove an element from the front of the list.
 */
extern void *Queue_dequeue(T queue);

#undef T
#undef E
#endif
