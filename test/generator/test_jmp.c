#include <stddef.h>
#include <stdio.h>

#include "test_common.h"


static void generate_test_case_jmp_rel(FILE *fp, size_t rel_size, const char *jmp_dest)
{
    uint64_t expected = 1;

    put_line_with_tab(fp, "mov rdi, %llu", expected);
    put_line_with_tab(fp, "mov rsi, 0");
    put_line_with_tab(fp, "jmp %s    # test target", jmp_dest);
    put_line_with_tab(fp, "call assert_equal_uint64"); // fail if jmp is ignored
    // adjust offset
    for(size_t i = 0; i < rel_size; i++)
    {
        put_line_with_tab(fp, "nop");
    }
    put_line(fp, "%s:", jmp_dest);
    put_line_with_tab(fp, "mov rdi, %llu", expected);
    put_line_with_tab(fp, "mov rsi, %llu", expected);
    put_line_with_tab(fp, "call assert_equal_uint64");
}


static void generate_test_case_jmp_reg(FILE *fp, const RegisterInfo *reg_info)
{
    size_t index_list[] = {reg_info->index};
    const char *reg = reg_info->name;
    uint64_t expected = 1;

    const char *work_reg = generate_save_register(fp, index_list, sizeof(index_list) / sizeof(index_list[0]));
    put_line_with_tab(fp, "mov rdi, %llu", expected);
    put_line_with_tab(fp, "mov rsi, 0");
    put_line_with_tab(fp, "lea %s, qword ptr [rip+jmp_dest_%s]", reg, reg);
    put_line_with_tab(fp, "jmp %s    # test target", reg);
    put_line_with_tab(fp, "call assert_equal_uint64"); // fail if jmp is ignored
    put_line(fp, "jmp_dest_%s:", reg);
    put_line_with_tab(fp, "mov rdi, %llu", expected);
    put_line_with_tab(fp, "mov rsi, %llu", expected);
    generate_restore_register(fp, work_reg);
    put_line_with_tab(fp, "call assert_equal_uint64");
}


static void generate_test_case_jmp_mem(FILE *fp, const char *jmp_dest)
{
    uint64_t expected = 1;

    put_line_with_tab(fp, "mov rdi, %llu", expected);
    put_line_with_tab(fp, "mov rsi, 0");
    put_line_with_tab(fp, "lea rax, qword ptr [rip+%s]", jmp_dest);
    put_line_with_tab(fp, "mov qword ptr [rbp-8], rax");
    put_line_with_tab(fp, "jmp qword ptr [rbp-8]    # test target");
    put_line_with_tab(fp, "call assert_equal_uint64"); // fail if jmp is ignored
    put_line(fp, "%s:", jmp_dest);
    put_line_with_tab(fp, "mov rdi, %llu", expected);
    put_line_with_tab(fp, "mov rsi, %llu", expected);
    put_line_with_tab(fp, "call assert_equal_uint64");
}


static void generate_all_test_case_jmp(FILE *fp)
{
    // JMP rel
    generate_test_case_jmp_rel(fp, 256, "jmp_dest_rel32");
    put_line(fp, "");

    // JMP reg
    for(size_t i = 0; i < REG_LIST_SIZE; i++)
    {
        const RegisterInfo *reg_info = &reg_list[i];
        if(reg_info->size == sizeof(uint64_t))
        {
            generate_test_case_jmp_reg(fp, reg_info);
            put_line(fp, "");
        }
    }

    // JMP mem
    generate_test_case_jmp_mem(fp, "jmp_dest_mem64");
    put_line(fp, "");
}


void generate_test_jmp(void)
{
    generate_test("test/test_jmp.s", STACK_ALIGNMENT, generate_all_test_case_jmp);
}
