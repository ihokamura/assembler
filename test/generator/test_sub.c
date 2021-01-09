#include "test_binary_common.h"
#include "test_common.h"


static uintmax_t binary_operation_sub(uintmax_t lhs, uintmax_t rhs)
{
    return lhs - rhs;
}


static void generate_all_test_case_sub(FILE *fp)
{
    generate_all_test_case_binary(fp, "sub", binary_operation_sub);
}


void generate_test_sub(void)
{
    const char *filename = "test/test_sub.s";
    const size_t stack_size = 16;
    generate_test(filename, stack_size, generate_all_test_case_sub);
}
