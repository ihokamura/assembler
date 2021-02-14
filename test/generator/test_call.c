#include <stddef.h>
#include <stdio.h>

#include "test_common.h"

static const char *TEST_CALLEE_NAME = "test_callee";


static void generate_test_case_call_rel(FILE *fp)
{
    put_line_with_tab(fp, "call %s    # test target", TEST_CALLEE_NAME);
}


static void generate_test_case_call_reg(FILE *fp, const RegisterInfo *reg_info)
{
    size_t index_list[] = {reg_info->index};
    const char *reg = reg_info->name;

    const char *work_reg = generate_save_register(fp, index_list, sizeof(index_list) / sizeof(index_list[0]));
    put_line_with_tab(fp, "lea %s, qword ptr [rip+%s]", reg, TEST_CALLEE_NAME);
    put_line_with_tab(fp, "call %s    # test target", reg);
    generate_restore_register(fp, work_reg);
}


static void generate_test_case_call_mem(FILE *fp, size_t size)
{
    const char *size_spec = get_size_specifier(size);
    size_t offset = 8;

    put_line_with_tab(fp, "lea rax, qword ptr [rip+%s]", TEST_CALLEE_NAME);
    put_line_with_tab(fp, "mov %s [rbp-%lu], rax", size_spec, offset, TEST_CALLEE_NAME);
    put_line_with_tab(fp, "call %s [rbp-%lu]    # test target", size_spec, offset);
}


static void generate_all_test_case_call(FILE *fp)
{
    // CALL rel
    generate_test_case_call_rel(fp);
    put_line(fp, "");

    // CALL reg
    for(size_t i = 0; i < REG_LIST_SIZE; i++)
    {
        const RegisterInfo *reg_info = &reg_list[i];
        if((reg_info->size == sizeof(uint64_t)) && (reg_info->index != REGISTER_INDEX_ESP))
        {
            generate_test_case_call_reg(fp, reg_info);
            put_line(fp, "");
        }
    }

    // CALL mem
    generate_test_case_call_mem(fp, sizeof(uint64_t));
    put_line(fp, "");
}


static void generate_test_callee(FILE *fp)
{
    put_line(fp, "%s:", TEST_CALLEE_NAME);
    put_line_with_tab(fp, "nop");
    put_line_with_tab(fp, "ret");
}


void generate_test_call(void)
{
    const char *filename = "test/test_call.s";
    size_t stack_size = STACK_ALIGNMENT;

    FILE *fp = fopen(filename, "w");
    if(fp == NULL)
    {
        fprintf(stderr, "cannot open %s\n", filename);
        return;
    }

    generate_prologue(fp, stack_size);
    generate_all_test_case_call(fp);
    generate_epilogue(fp);
    put_line(fp, "");

    generate_test_callee(fp);

    fclose(fp);
}
