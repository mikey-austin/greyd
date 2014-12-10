/**
 * @file   queue.c
 * @brief  Implements an abstract queue data structure.
 * @author Mikey Austin
 * @date   2014
 */

#include "failures.h"
#include "queue.h"

#include <stdlib.h>

extern Queue_T
Queue_create(void (*destroy)(void *value))
{
    Queue_T queue;

    if((queue = malloc(sizeof(*queue))) == NULL) {
        i_critical("Could not create an empty queue");
    }
    else {
        queue->list = List_create(destroy);
    }

    return queue;
}

extern void
Queue_destroy(Queue_T *queue)
{
    if(queue != NULL && *queue != NULL) {
        List_destroy(&((*queue)->list));
        free(*queue);
        *queue = NULL;
    }
}

extern void
Queue_enqueue(Queue_T queue, void *value)
{
    List_insert_after(queue->list, value);
}

extern void
*Queue_dequeue(Queue_T queue)
{
    return List_remove_head(queue->list);
}

extern int
Queue_size(Queue_T queue)
{
    return List_size(queue->list);
}
