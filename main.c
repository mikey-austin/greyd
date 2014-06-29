#include <stdio.h>
#include "failures.h"

int main()
{
    I_INFO("here is %s the %drd\n", "mikey", 3);
    I_WARN("single\n");

    return 0;
}
