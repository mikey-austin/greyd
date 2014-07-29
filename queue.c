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
#define E Queue_entry_T

static struct E *Queue_create_element(T queue, void *value);
static void      Queue_destroy_element(T queue, struct E *element);

extern T
Queue_create(void (*destroy)(void *value))
{
    T queue;

    if((queue = (T) malloc(sizeof(*queue))) == NULL) {
        I_CRIT("Could not create an empty queue");
    }

    /* Initialize queue pointers. */
    queue->destroy = destroy;
    queue->size    = 0;
    queue->head    = NULL;

    return queue;
}

extern void
Queue_destroy(T queue)
{
    struct E *element, *curr;

    for(curr = queue->head; curr != NULL; ) {
        element = curr;
        curr = curr->next;

        /* Destroy the element's value. */
        if(element->v && queue->destroy) {
            queue->destroy(element->v);
        }

        Queue_destroy_element(queue, element);
    }

    free(queue);
}

extern void
Queue_enqueue(T queue, void *value)
{
    struct E *element, *curr;

    element = Queue_create_element(queue, value);
    queue->size++;

    if(queue->head) {
        for(curr = queue->head; curr != NULL; curr = curr->next) {
            if(curr->next == NULL) {
                curr->next = element;
                break;
            }
        }
    }
    else {
        queue->head = element;
    }
}

extern void
*Queue_dequeue(T queue)
{
    struct E *element;
    void *value;

    if((element = queue->head) == NULL) {
        return NULL;
    }

    queue->head = element->next;

    value = element->v;
    Queue_destroy_element(queue, element);
    queue->size--;

    return value;
}

static struct E
*Queue_create_element(T queue, void *value)
{
    struct E *element;

    if((element = (struct E *) malloc(sizeof(*element))) == NULL) {
        I_CRIT("Could not initialize queue element");
    }

    element->next = NULL;
    element->v    = value;

    return element;
}

static void
Queue_destroy_element(T queue, struct E *element)
{
    if(element) {
        free(element);
    }
}

#undef T
#undef E
