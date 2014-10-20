/**
 * @file   queue.c
 * @brief  Implements an abstract queue data structure.
 * @author Mikey Austin
 * @date   2014
 */

#include "failures.h"
#include "queue.h"

#include <stdlib.h>

#define T Queue_T

extern T
Queue_create(void (*destroy)(void *value))
{
    T queue;

    if((queue = malloc(sizeof(*queue))) == NULL) {
        I_CRIT("Could not create an empty queue");
    }
    else {
        queue->list = List_create(destroy);
    }

    return queue;
}

extern void
Queue_destroy(T *queue)
{
    if(queue != NULL && *queue != NULL) {
        List_destroy(&((*queue)->list));
        free(*queue);
        *queue = NULL;
    }
}

extern void
Queue_enqueue(T queue, void *value)
{
    List_insert_after(queue->list, value);
}

extern void
*Queue_dequeue(T queue)
{
    return List_remove_head(queue->list);
}

extern int
Queue_size(T queue)
{
    return List_size(queue->list);
}

#undef T
