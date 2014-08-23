/**
 * @file   test_list.c
 * @brief  Unit tests for the generic list interface.
 * @author Mikey Austin
 * @date   2014
 */

#include "test.h"
#include "../list.h"

#include <string.h>

static void destroy_string(void *str);

int
main()
{
    List_T list;
    struct List_entry_T *curr;
    char *v1 = "value 1", *v2 = "value 2", *v3 = "value 3", *v;
    int i;

    TEST_START(15);

    list = List_create(NULL);
    TEST_OK((list != NULL), "List created successfully");
    TEST_OK((List_size(list) == 0), "List size correct");

    List_insert_after(list, (void *) v2);
    List_insert_after(list, (void *) v3);
    List_insert_head(list, (void *) v1);
    TEST_OK((List_size(list) == 3), "List size is as expected");

    /*
     * Test list iteration.
     */
    i = 0;
    LIST_FOREACH(list, curr) {
        if(i < 3) {
            TEST_OK((curr != NULL), "List iteration is as expected");

            v = (char *) List_entry_value(curr);
            switch(i)
            {
            case 0:
                TEST_OK((v && strcmp(v, v1) == 0), "Retrieved value is correct");
                break;

            case 1:
                TEST_OK((v && strcmp(v, v2) == 0), "Retrieved value is correct");
                break;

            case 2:
                TEST_OK((v && strcmp(v, v3) == 0), "Retrieved value is correct");
                break;
            }
        }

        i++;
    }

    /*
     * Test list element removal.
     */
    v = (char *) List_remove_head(list);
    TEST_OK((v && strcmp(v, v1) == 0), "Dequeued value is correct");
    TEST_OK((List_size(list) == 2), "List size decremented correctly");

    v = (char *) List_remove_head(list);
    TEST_OK((v && strcmp(v, v2) == 0), "Dequeued value is correct");
    TEST_OK((List_size(list) == 1), "List size decremented correctly");

    v = (char *) List_remove_head(list);
    TEST_OK((v && strcmp(v, v3) == 0), "Dequeued value is correct");
    TEST_OK((List_size(list) == 0), "List size decremented correctly");

    /* Destroy the list. */
    List_destroy(list);

    /* Test list destructor. */
    list = List_create(destroy_string);

    v = (char *) malloc((i = strlen("hello world")) + 1);
    strcpy(v, "hello world");
    v[i] = '\0';

    /*
     * Rely on valgrind to report if the value's memory is cleaned up correctly
     * by the destructor.
     */
    List_insert_head(list, (void *) v);
    List_destroy(list);

    TEST_COMPLETE;
}

static void
destroy_string(void *str)
{
    if(str) {
        free(str);
    }
}
