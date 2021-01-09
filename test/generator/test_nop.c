#include <stddef.h>
#include <stdio.h>

#include "test_common.h"


static void generate_test_case_nop(FILE *fp)
{
    put_line_with_tab(fp, "nop    # test target");
}


static void generate_all_test_case_nop(FILE *fp)
{
    // NOP
    generate_test_case_nop(fp);
    put_line(fp, "");
}


void generate_test_nop(void)
{
    const char *filename = "test/test_nop.s";
    static const size_t STACK_SIZE = 0;
    generate_test(filename, STACK_SIZE, generate_all_test_case_nop);
}
