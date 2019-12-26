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
 * @file   test_queue.c
 * @brief  Unit tests for the generic queue interface.
 * @author Mikey Austin
 * @date   2014
 */

#include "test.h"
#include <queue.h>

#include <string.h>

int main(void)
{
    Queue_T queue;
    char *v1 = "value 1", *v2 = "value 2", *v3 = "value 3", *v;

    TEST_START(9);

    queue = Queue_create(NULL);
    TEST_OK((queue != NULL), "Queue created successfully");
    TEST_OK((Queue_size(queue) == 0), "Queue size correct");

    Queue_enqueue(queue, (void*)v1);
    Queue_enqueue(queue, (void*)v2);
    Queue_enqueue(queue, (void*)v3);
    TEST_OK((Queue_size(queue) == 3), "Queue size is as expected");

    v = (char*)Queue_dequeue(queue);
    TEST_OK((strcmp(v, "value 1") == 0), "Dequeued value correct");
    TEST_OK((Queue_size(queue) == 2), "Queue size is as expected");

    v = (char*)Queue_dequeue(queue);
    TEST_OK((strcmp(v, "value 2") == 0), "Dequeued value correct");
    TEST_OK((Queue_size(queue) == 1), "Queue size is as expected");

    v = (char*)Queue_dequeue(queue);
    TEST_OK((strcmp(v, "value 3") == 0), "Dequeued value correct");
    TEST_OK((Queue_size(queue) == 0), "Queue size is as expected");

    /* Destroy the queue. */
    Queue_destroy(&queue);

    TEST_COMPLETE;
}
