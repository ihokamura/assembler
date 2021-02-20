#include <stddef.h>
#include <stdio.h>

#include "test_common.h"


static void generate_test_case_jmp_rel(FILE *fp, size_t size, const char *jmp_dest)
{
    uint64_t expected = 1;
    put_line_with_tab(fp, "mov rdi, %llu", expected);
    put_line_with_tab(fp, "mov rsi, 0");
    put_line_with_tab(fp, "jmp %s    # test target", jmp_dest);
    put_line_with_tab(fp, "call assert_equal_uint64"); // fail if jmp is ignored
    // adjust offset
    for(size_t i = 0; i < size; i++)
    {
        put_line_with_tab(fp, "nop");
    }
    put_line(fp, "%s:", jmp_dest);
    put_line_with_tab(fp, "mov rsi, %llu", expected);
    put_line_with_tab(fp, "call assert_equal_uint64");
}


static void generate_all_test_case_jmp(FILE *fp)
{
    // JMP rel
    generate_test_case_jmp_rel(fp, 256, "jmp_dest_rel32");
    put_line(fp, "");
}


void generate_test_jmp(void)
{
    generate_test("test/test_jmp.s", 0, generate_all_test_case_jmp);
}
