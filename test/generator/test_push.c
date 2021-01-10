#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "test_common.h"


static void generate_test_case_push_imm(FILE *fp, size_t size, intmax_t value)
{
    const char *arg1 = get_1st_argument_register(size);
    const char *arg2 = get_2nd_argument_register(size);
    const char *size_spec = get_size_specifier(size);

    const char *work_reg = generate_save_register(fp, NULL, 0);
    put_line_with_tab(fp, "push 0x%llx    # test target", value);
    put_line_with_tab(fp, "mov %s, %s [rsp]", arg2, size_spec);
    put_line_with_tab(fp, "mov %s, 0x%llx", arg1, value);
    generate_restore_register(fp, work_reg);
    put_line_with_tab(fp, "call assert_equal_uint%ld", convert_size_to_bit(size));
}


static void generate_test_case_push_reg(FILE *fp, const RegisterInfo *reg_info, intmax_t value)
{
    size_t size = reg_info->size;
    size_t index_list[] = {reg_info->index};
    const char *reg = reg_info->name;
    const char *arg1 = get_1st_argument_register(size);
    const char *arg2 = get_2nd_argument_register(size);
    const char *size_spec = get_size_specifier(size);

    const char *work_reg = generate_save_register(fp, index_list, sizeof(index_list) / sizeof(index_list[0]));
    // avoid to overwrite esp
    if(reg_info->index != REGISTER_INDEX_ESP)
    {
        put_line_with_tab(fp, "mov %s, 0x%llx", reg, value);
    }
    put_line_with_tab(fp, "mov %s, %s", arg1, reg);
    put_line_with_tab(fp, "push %s    # test target", reg);
    put_line_with_tab(fp, "mov %s, %s [rsp]", arg2, size_spec);
    generate_restore_register(fp, work_reg);
    put_line_with_tab(fp, "call assert_equal_uint%ld", convert_size_to_bit(size));
}


static void generate_test_case_push_mem(FILE *fp, size_t size, intmax_t value)
{
    const char *arg1 = get_1st_argument_register(size);
    const char *arg2 = get_2nd_argument_register(size);
    const char *size_spec = get_size_specifier(size);

    const char *work_reg = generate_save_register(fp, NULL, 0);
    put_line_with_tab(fp, "mov %s [rbp-8], 0x%llx", size_spec, value);
    put_line_with_tab(fp, "push %s [rbp-8]    # test target", size_spec, value);
    put_line_with_tab(fp, "mov %s, %s [rsp]", arg2, size_spec);
    put_line_with_tab(fp, "mov %s, 0x%llx", arg1, value);
    generate_restore_register(fp, work_reg);
    put_line_with_tab(fp, "call assert_equal_uint%ld", convert_size_to_bit(size));
}


static void generate_all_test_case_push(FILE *fp)
{
    // PUSH imm
    for(size_t j = 0; j < IMM_LIST_SIZE; j++)
    {
        const ImmediateInfo *imm_info = &imm_list[j];
        generate_test_case_push_imm(fp, imm_info->size, imm_info->sint_max_value);
        put_line(fp, "");
    }

    // PUSH reg
    for(size_t i = 0; i < REG_LIST_SIZE; i++)
    {
        const RegisterInfo *reg_info = &reg_list[i];
        size_t size = reg_info->size;
        if((size == sizeof(uint16_t)) || (size == sizeof(uint64_t)))
        {
            generate_test_case_push_reg(fp, reg_info, convert_size_to_sint_max_value(reg_info->size));
            put_line(fp, "");
        }
    }

    // PUSH mem
    for(size_t j = 0; j < IMM_LIST_SIZE; j++)
    {
        const ImmediateInfo *imm_info = &imm_list[j];
        size_t size = imm_info->size;
        if((size == sizeof(uint16_t)) || (size == sizeof(uint64_t)))
        {
            generate_test_case_push_mem(fp, imm_info->size, imm_info->sint_max_value);
            put_line(fp, "");
        }
    }
}


void generate_test_push(void)
{
    generate_test("test/test_push.s", STACK_ALIGNMENT, generate_all_test_case_push);
}
