#include <stddef.h>

#include "test_generator.h"

static void (* const generate_test[])(void) = 
{
    generate_test_add,
    generate_test_and,
    generate_test_call,
    generate_test_cmp,
    generate_test_lea,
    generate_test_mov,
    generate_test_neg,
    generate_test_nop,
    generate_test_not,
    generate_test_or,
    generate_test_pop,
    generate_test_push,
    generate_test_sal,
    generate_test_sar,
    generate_test_set,
    generate_test_shl,
    generate_test_shr,
    generate_test_sub,
    generate_test_xor,
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
