/**
 * @file   test_queue.c
 * @brief  Unit tests for the generic queue interface.
 * @author Mikey Austin
 * @date   2014
 */

#include "test.h"
#include "../queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

int
main()
{
    Queue_T queue;
    char *v1 = "value 1", *v2 = "value 2", *v3 = "value 3", *v;
    int i;

    TEST_START(9);

    queue = Queue_create(NULL);
    TEST_OK((queue != NULL), "Queue created successfully");
    TEST_OK((queue->size == 0), "Queue size correct");

    Queue_enqueue(queue, (void *) v1);
    Queue_enqueue(queue, (void *) v2);
    Queue_enqueue(queue, (void *) v3);
    TEST_OK((queue->size == 3), "Queue size is as expected");

    v = (char *) Queue_dequeue(queue);
    TEST_OK((strcmp(v, "value 1") == 0), "Dequeued value correct");
    TEST_OK((queue->size == 2), "Queue size is as expected");

    v = (char *) Queue_dequeue(queue);
    TEST_OK((strcmp(v, "value 2") == 0), "Dequeued value correct");
    TEST_OK((queue->size == 1), "Queue size is as expected");

    v = (char *) Queue_dequeue(queue);
    TEST_OK((strcmp(v, "value 3") == 0), "Dequeued value correct");
    TEST_OK((queue->size == 0), "Queue size is as expected");

    /* Destroy the queue. */
    Queue_destroy(queue);

    TEST_COMPLETE;
}
