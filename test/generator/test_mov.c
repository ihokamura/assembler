#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "test_common.h"


static void generate_test_case_mov_reg_imm(FILE *fp, const RegisterInfo *reg_info, uint32_t value)
{
    size_t size = reg_info->size;
    size_t index_list[] = {reg_info->index};
    const char *reg = reg_info->name;
    const char *arg1 = get_1st_argument_register(size);
    const char *arg2 = get_2nd_argument_register(size);

    const char *work_reg = generate_save_register(fp, index_list, sizeof(index_list) / sizeof(index_list[0]));
    put_line_with_tab(fp, "mov %s, 0x%lx    # test target", reg, value);
    put_line_with_tab(fp, "mov %s, %s", arg2, reg);
    put_line_with_tab(fp, "mov %s, 0x%lx", arg1, value);
    generate_restore_register(fp, index_list, sizeof(index_list) / sizeof(index_list[0]), work_reg);
    put_line_with_tab(fp, "call assert_equal_uint%ld", convert_size_to_bit(size));
}


static void generate_test_case_mov_reg_reg(FILE *fp, const RegisterInfo *reg1_info, const RegisterInfo *reg2_info, uint32_t value)
{
    assert(reg1_info->size == reg2_info->size);
    size_t size = reg1_info->size;
    size_t index_list[] = {reg1_info->index, reg2_info->index};
    const char *reg1 = reg1_info->name;
    const char *reg2 = reg2_info->name;
    const char *arg1 = get_1st_argument_register(size);
    const char *arg2 = get_2nd_argument_register(size);

    const char *work_reg = generate_save_register(fp, index_list, sizeof(index_list) / sizeof(index_list[0]));
    put_line_with_tab(fp, "mov %s, 0x%lx", reg2, value);
    put_line_with_tab(fp, "mov %s, %s    # test target", reg1, reg2);
    put_line_with_tab(fp, "mov %s, %s", arg2, reg1);
    put_line_with_tab(fp, "mov %s, 0x%lx", arg1, value);
    generate_restore_register(fp, index_list, sizeof(index_list) / sizeof(index_list[0]), work_reg);
    put_line_with_tab(fp, "call assert_equal_uint%ld", convert_size_to_bit(size));
}


static void generate_test_case_mov_reg_mem(FILE *fp, const RegisterInfo *reg_info, uint32_t value)
{
    size_t size = reg_info->size;
    size_t index_list[] = {reg_info->index};
    const char *reg = reg_info->name;
    const char *arg1 = get_1st_argument_register(size);
    const char *arg2 = get_2nd_argument_register(size);
    const char *size_spec = get_size_specifier(size);
    size_t offset = 16;

    const char *work_reg = generate_save_register(fp, index_list, sizeof(index_list) / sizeof(index_list[0]));
    put_line_with_tab(fp, "mov %s [%s-%lu], 0x%lx", size_spec, work_reg, offset, value);
    put_line_with_tab(fp, "mov %s, %s [%s-%lu]    # test target", reg, size_spec, work_reg, offset);
    put_line_with_tab(fp, "mov %s, %s", arg2, reg);
    put_line_with_tab(fp, "mov %s, 0x%lx", arg1, value);
    generate_restore_register(fp, index_list, sizeof(index_list) / sizeof(index_list[0]), work_reg);
    put_line_with_tab(fp, "call assert_equal_uint%ld", convert_size_to_bit(size));
}


static void generate_test_case_mov_mem_imm(FILE *fp, size_t size, uint32_t value)
{
    const char *arg1 = get_1st_argument_register(size);
    const char *arg2 = get_2nd_argument_register(size);
    const char *size_spec = get_size_specifier(size);

    put_line_with_tab(fp, "mov %s [rbp-8], 0x%lx    # test target", size_spec, value);
    put_line_with_tab(fp, "mov %s, %s [rbp-8]", arg2, size_spec);
    put_line_with_tab(fp, "mov %s, 0x%lx", arg1, value);
    put_line_with_tab(fp, "call assert_equal_uint%ld", convert_size_to_bit(size));
}


static void generate_test_case_mov_mem_reg(FILE *fp, const RegisterInfo *reg_info, uint32_t value)
{
    size_t size = reg_info->size;
    size_t index_list[] = {reg_info->index};
    const char *reg = reg_info->name;
    const char *arg1 = get_1st_argument_register(size);
    const char *arg2 = get_2nd_argument_register(size);
    const char *size_spec = get_size_specifier(size);
    size_t offset = 16;

    const char *work_reg = generate_save_register(fp, index_list, sizeof(index_list) / sizeof(index_list[0]));
    put_line_with_tab(fp, "mov %s, 0x%lx", reg, value);
    put_line_with_tab(fp, "mov %s [%s-%lu], %s    # test target", size_spec, work_reg, offset, reg);
    put_line_with_tab(fp, "mov %s, %s [%s-%lu]", arg2, size_spec, work_reg, offset);
    put_line_with_tab(fp, "mov %s, 0x%lx", arg1, value);
    generate_restore_register(fp, index_list, sizeof(index_list) / sizeof(index_list[0]), work_reg);
    put_line_with_tab(fp, "call assert_equal_uint%ld", convert_size_to_bit(size));
}


static void generate_all_test_case_mov(FILE *fp)
{
    // MOV reg, imm
    for(size_t i = 0; i < REG_LIST_SIZE; i++)
    {
        const RegisterInfo *reg_info = &reg_list[i];
        for(size_t j = 0; j < IMM_LIST_SIZE; j++)
        {
            const ImmediateInfo *imm_info = &imm_list[j];
            if(reg_info->size >= imm_info->size)
            {
                generate_test_case_mov_reg_imm(fp, reg_info, imm_info->sint_max_value);
                put_line(fp, "");
            }
        }
    }

    // MOV reg, reg
    for(size_t i = 0; i < REG_LIST_SIZE; i++)
    {
        const RegisterInfo *reg1_info = &reg_list[i];
        for(size_t j = 0; j < REG_LIST_SIZE; j++)
        {
            const RegisterInfo *reg2_info = &reg_list[j];
            if(reg1_info->size == reg2_info->size)
            {
                generate_test_case_mov_reg_reg(fp, reg1_info, reg2_info, convert_size_to_sint_max_value(reg1_info->size));
                put_line(fp, "");
            }
        }
    }

    // MOV reg, mem
    for(size_t i = 0; i < REG_LIST_SIZE; i++)
    {
        const RegisterInfo *reg_info = &reg_list[i];
        generate_test_case_mov_reg_mem(fp, reg_info, convert_size_to_sint_max_value(reg_info->size));
        put_line(fp, "");
    }

    // MOV mem, imm
    for(size_t j = 0; j < IMM_LIST_SIZE; j++)
    {
        const ImmediateInfo *imm_info = &imm_list[j];
        generate_test_case_mov_mem_imm(fp, imm_info->size, imm_info->sint_max_value);
        put_line(fp, "");
    }

    // MOV mem, reg
    for(size_t i = 0; i < REG_LIST_SIZE; i++)
    {
        const RegisterInfo *reg_info = &reg_list[i];
        generate_test_case_mov_mem_reg(fp, reg_info, convert_size_to_sint_max_value(reg_info->size));
        put_line(fp, "");
    }
}


void generate_test_mov(void)
{
    const char *filename = "test/test_mov.s";
    static const size_t STACK_SIZE = 16;
    generate_test(filename, STACK_SIZE, generate_all_test_case_mov);
}
