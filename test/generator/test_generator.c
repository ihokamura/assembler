#include <stddef.h>

#include "test_generator.h"

static void (* const generate_test[])(void) = 
{
    generate_test_mov,
    generate_test_nop,
    generate_test_sub,
};
static const size_t GENERATE_TEST_SIZE = sizeof(generate_test) / sizeof(generate_test[0]);


int main(void)
{

    for(size_t i = 0; i < GENERATE_TEST_SIZE; i++)
    {
        generate_test[i]();
    }

    return 0;
}
