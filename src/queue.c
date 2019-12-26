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
 * @file   queue.c
 * @brief  Implements an abstract queue data structure.
 * @author Mikey Austin
 * @date   2014
 */

#include "queue.h"
#include "failures.h"

#include <stdlib.h>

extern Queue_T
Queue_create(void (*destroy)(void* value))
{
    Queue_T queue;

    if ((queue = malloc(sizeof(*queue))) == NULL) {
        i_critical("Could not create an empty queue");
    } else {
        queue->list = List_create(destroy);
    }

    return queue;
}

extern void
Queue_destroy(Queue_T* queue)
{
    if (queue != NULL && *queue != NULL) {
        List_destroy(&((*queue)->list));
        free(*queue);
        *queue = NULL;
    }
}

extern void
Queue_enqueue(Queue_T queue, void* value)
{
    List_insert_after(queue->list, value);
}

extern void* Queue_dequeue(Queue_T queue)
{
    return List_remove_head(queue->list);
}

extern int
Queue_size(Queue_T queue)
{
    return List_size(queue->list);
}
