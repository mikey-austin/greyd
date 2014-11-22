/**
 * @file   queue.h
 * @brief  Defines an abstract queue data structure.
 * @author Mikey Austin
 * @date   2014
 *
 * This abstract data type is defined by way of a singly-linked list.
 */

#ifndef QUEUE_DEFINED
#define QUEUE_DEFINED

#include "list.h"

/**
 * The main queue structure.
 */
typedef struct Queue_T *Queue_T;
struct Queue_T {
    List_T list;
};

/**
 * Create a new queue table.
 */
extern Queue_T Queue_create(void (*destroy)(void *value));

/**
 * Destroy a queue, freeing all elements.
 */
extern void Queue_destroy(Queue_T *queue);

/**
 * Insert a new element onto the end of the queue.
 */
extern void Queue_enqueue(Queue_T queue, void *value);

/**
 * Fetch and remove an element from the front of the list.
 */
extern void *Queue_dequeue(Queue_T queue);

/**
 * Return the size of the queue.
 */
extern int Queue_size(Queue_T queue);

#endif
