#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "test_common.h"


static void generate_test_case_sub_reg_imm(FILE *fp, const RegisterInfo *reg_info, intmax_t val1, intmax_t val2, intmax_t result)
{
    size_t size = reg_info->size;
    size_t index_list[] = {reg_info->index};
    const char *reg = reg_info->name;
    const char *arg1 = get_1st_argument_register(size);
    const char *arg2 = get_2nd_argument_register(size);
    const char *mnemonic = "sub";

    const char *work_reg = generate_save_register(fp, index_list, sizeof(index_list) / sizeof(index_list[0]));
    put_line_with_tab(fp, "mov %s, 0x%llx", reg, val1);
    put_line_with_tab(fp, "%s %s, 0x%llx    # test target", mnemonic, reg, val2);
    put_line_with_tab(fp, "mov %s, %s", arg2, reg);
    put_line_with_tab(fp, "mov %s, 0x%llx", arg1, result);
    generate_restore_register(fp, index_list, sizeof(index_list) / sizeof(index_list[0]), work_reg);
    put_line_with_tab(fp, "call assert_equal_uint%ld", convert_size_to_bit(size));
}


static void generate_test_case_sub_reg_reg(FILE *fp, const RegisterInfo *reg1_info, const RegisterInfo *reg2_info, intmax_t val1, intmax_t val2, intmax_t result)
{
    assert(reg1_info->size == reg2_info->size);
    size_t size = reg1_info->size;
    size_t index_list[] = {reg1_info->index, reg2_info->index};
    const char *reg1 = reg1_info->name;
    const char *reg2 = reg2_info->name;
    const char *arg1 = get_1st_argument_register(size);
    const char *arg2 = get_2nd_argument_register(size);
    const char *mnemonic = "sub";

    const char *work_reg = generate_save_register(fp, index_list, sizeof(index_list) / sizeof(index_list[0]));
    put_line_with_tab(fp, "mov %s, 0x%llx", reg1, val1);
    put_line_with_tab(fp, "mov %s, 0x%llx", reg2, val2);
    put_line_with_tab(fp, "%s %s, %s    # test target", mnemonic, reg1, reg2);
    put_line_with_tab(fp, "mov %s, %s", arg2, reg1);
    put_line_with_tab(fp, "mov %s, 0x%llx", arg1, result);
    generate_restore_register(fp, index_list, sizeof(index_list) / sizeof(index_list[0]), work_reg);
    put_line_with_tab(fp, "call assert_equal_uint%ld", convert_size_to_bit(size));
}


static void generate_test_case_sub_reg_mem(FILE *fp, const RegisterInfo *reg_info, intmax_t val1, intmax_t val2, intmax_t result)
{
    size_t size = reg_info->size;
    size_t index_list[] = {reg_info->index};
    const char *reg = reg_info->name;
    const char *arg1 = get_1st_argument_register(size);
    const char *arg2 = get_2nd_argument_register(size);
    const char *size_spec = get_size_specifier(size);
    size_t offset = 16;
    const char *mnemonic = "sub";

    const char *work_reg = generate_save_register(fp, index_list, sizeof(index_list) / sizeof(index_list[0]));
    put_line_with_tab(fp, "mov %s, 0x%llx", reg, val1);
    put_line_with_tab(fp, "mov %s [%s-%lu], 0x%llx", size_spec, work_reg, offset, val2);
    put_line_with_tab(fp, "%s %s, %s [%s-%lu]    # test target", mnemonic, reg, size_spec, work_reg, offset);
    put_line_with_tab(fp, "mov %s, %s", arg2, reg);
    put_line_with_tab(fp, "mov %s, 0x%llx", arg1, result);
    generate_restore_register(fp, index_list, sizeof(index_list) / sizeof(index_list[0]), work_reg);
    put_line_with_tab(fp, "call assert_equal_uint%ld", convert_size_to_bit(size));
}


static void generate_test_case_sub_mem_imm(FILE *fp, size_t size, intmax_t val1, intmax_t val2, intmax_t result)
{
    const char *arg1 = get_1st_argument_register(size);
    const char *arg2 = get_2nd_argument_register(size);
    const char *size_spec = get_size_specifier(size);
    const char *mnemonic = "sub";

    put_line_with_tab(fp, "mov %s [rbp-8], 0x%llx", size_spec, val1);
    put_line_with_tab(fp, "%s %s [rbp-8], 0x%llx    # test target", mnemonic, size_spec, val2);
    put_line_with_tab(fp, "mov %s, %s [rbp-8]", arg2, size_spec);
    put_line_with_tab(fp, "mov %s, 0x%llx", arg1, result);
    put_line_with_tab(fp, "call assert_equal_uint%ld", convert_size_to_bit(size));
}


static void generate_test_case_sub_mem_reg(FILE *fp, const RegisterInfo *reg_info, intmax_t val1, intmax_t val2, intmax_t result)
{
    size_t size = reg_info->size;
    size_t index_list[] = {reg_info->index};
    const char *reg = reg_info->name;
    const char *arg1 = get_1st_argument_register(size);
    const char *arg2 = get_2nd_argument_register(size);
    const char *size_spec = get_size_specifier(size);
    size_t offset = 16;
    const char *mnemonic = "sub";

    const char *work_reg = generate_save_register(fp, index_list, sizeof(index_list) / sizeof(index_list[0]));
    put_line_with_tab(fp, "mov %s [%s-%lu], 0x%llx", size_spec, work_reg, offset, val1);
    put_line_with_tab(fp, "mov %s, 0x%llx", reg, val2);
    put_line_with_tab(fp, "%s %s [%s-%lu], %s    # test target", mnemonic, size_spec, work_reg, offset, reg);
    put_line_with_tab(fp, "mov %s, %s [%s-%lu]", arg2, size_spec, work_reg, offset);
    put_line_with_tab(fp, "mov %s, 0x%llx", arg1, result);
    generate_restore_register(fp, index_list, sizeof(index_list) / sizeof(index_list[0]), work_reg);
    put_line_with_tab(fp, "call assert_equal_uint%ld", convert_size_to_bit(size));
}


static void generate_all_test_case_sub(FILE *fp)
{
    // SUB reg, imm
    for(size_t i = 0; i < REG_LIST_SIZE; i++)
    {
        const RegisterInfo *reg_info = &reg_list[i];
        for(size_t j = 0; j < IMM_LIST_SIZE; j++)
        {
            const ImmediateInfo *imm_info = &imm_list[j];
            if(reg_info->size >= imm_info->size)
            {
                intmax_t val1 = imm_info->sint_max_value;
                intmax_t val2 = 1;
                intmax_t result = val1 - val2;
                generate_test_case_sub_reg_imm(fp, reg_info, val1, val2, result);
                put_line(fp, "");
            }
        }
    }

    // SUB reg, reg
    for(size_t i = 0; i < REG_LIST_SIZE; i++)
    {
        const RegisterInfo *reg1_info = &reg_list[0];
        for(size_t j = 0; j < REG_LIST_SIZE; j++)
        {
            const RegisterInfo *reg2_info = &reg_list[j];
            if(reg1_info->size == reg2_info->size)
            {
                intmax_t val1 = convert_size_to_sint_max_value(reg1_info->size);
                intmax_t val2 = 1;
                intmax_t result = (reg1_info->index == reg2_info->index) ? 0 : (val1 - val2);
                generate_test_case_sub_reg_reg(fp, reg1_info, reg2_info, val1, val2, result);
                put_line(fp, "");
            }
        }
    }

    // SUB reg, mem
    for(size_t i = 0; i < REG_LIST_SIZE; i++)
    {
        const RegisterInfo *reg_info = &reg_list[i];
        intmax_t val1 = convert_size_to_sint_max_value(reg_info->size);
        intmax_t val2 = 1;
        intmax_t result = val1 - val2;
        generate_test_case_sub_reg_mem(fp, reg_info, val1, val2, result);
        put_line(fp, "");
    }

    // SUB mem, imm
    for(size_t j = 0; j < IMM_LIST_SIZE; j++)
    {
        const ImmediateInfo *imm_info = &imm_list[j];
        intmax_t val1 = imm_info->sint_max_value;
        intmax_t val2 = 1;
        intmax_t result = val1 - val2;
        generate_test_case_sub_mem_imm(fp, imm_info->size, val1, val2, result);
        put_line(fp, "");
    }

    // SUB mem, reg
    for(size_t i = 0; i < REG_LIST_SIZE; i++)
    {
        const RegisterInfo *reg_info = &reg_list[i];
        intmax_t val1 = convert_size_to_sint_max_value(reg_info->size);
        intmax_t val2 = 1;
        intmax_t result = val1 - val2;
        generate_test_case_sub_mem_reg(fp, reg_info, val1, val2, result);
        put_line(fp, "");
    }
}


void generate_test_sub(void)
{
    const char *filename = "test/test_sub.s";
    const size_t stack_size = 16;
    generate_test(filename, stack_size, generate_all_test_case_sub);
}
