#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "test_binary_common.h"
#include "test_common.h"


static void generate_test_case_binary_reg_imm(FILE *fp, const BinaryOperationInfo *op_info, const RegisterInfo *reg_info)
{
    size_t size = reg_info->size;
    size_t index_list[] = {reg_info->index};
    const char *reg = reg_info->name;
    const char *arg1 = get_1st_argument_register(size);
    const char *arg2 = get_2nd_argument_register(size);

    const char *work_reg = generate_save_register(fp, index_list, sizeof(index_list) / sizeof(index_list[0]));
    put_line_with_tab(fp, "mov %s, 0x%llx", reg, op_info->lhs);
    put_line_with_tab(fp, "%s %s, 0x%llx    # test target", op_info->mnemonic, reg, op_info->rhs);
    put_line_with_tab(fp, "mov %s, %s", arg2, reg);
    put_line_with_tab(fp, "mov %s, 0x%llx", arg1, op_info->result);
    generate_restore_register(fp, work_reg);
    put_line_with_tab(fp, "call assert_equal_uint%ld", convert_size_to_bit(size));
}


static void generate_test_case_binary_reg_reg(FILE *fp, const BinaryOperationInfo *op_info, const RegisterInfo *reg1_info, const RegisterInfo *reg2_info)
{
    assert(reg1_info->size == reg2_info->size);
    size_t size = reg1_info->size;
    size_t index_list[] = {reg1_info->index, reg2_info->index};
    const char *reg1 = reg1_info->name;
    const char *reg2 = reg2_info->name;
    const char *arg1 = get_1st_argument_register(size);
    const char *arg2 = get_2nd_argument_register(size);

    const char *work_reg = generate_save_register(fp, index_list, sizeof(index_list) / sizeof(index_list[0]));
    put_line_with_tab(fp, "mov %s, 0x%llx", reg1, op_info->lhs);
    put_line_with_tab(fp, "mov %s, 0x%llx", reg2, op_info->rhs);
    put_line_with_tab(fp, "%s %s, %s    # test target", op_info->mnemonic, reg1, reg2);
    put_line_with_tab(fp, "mov %s, %s", arg2, reg1);
    put_line_with_tab(fp, "mov %s, 0x%llx", arg1, op_info->result);
    generate_restore_register(fp, work_reg);
    put_line_with_tab(fp, "call assert_equal_uint%ld", convert_size_to_bit(size));
}


static void generate_test_case_binary_reg_mem(FILE *fp, const BinaryOperationInfo *op_info, const RegisterInfo *reg_info)
{
    size_t size = reg_info->size;
    size_t index_list[] = {reg_info->index};
    const char *reg = reg_info->name;
    const char *arg1 = get_1st_argument_register(size);
    const char *arg2 = get_2nd_argument_register(size);
    const char *size_spec = get_size_specifier(size);
    size_t offset = 16;

    const char *work_reg = generate_save_register(fp, index_list, sizeof(index_list) / sizeof(index_list[0]));
    put_line_with_tab(fp, "mov %s, 0x%llx", reg, op_info->lhs);
    put_line_with_tab(fp, "mov %s [%s-%lu], 0x%llx", size_spec, work_reg, offset, op_info->rhs);
    put_line_with_tab(fp, "%s %s, %s [%s-%lu]    # test target", op_info->mnemonic, reg, size_spec, work_reg, offset);
    put_line_with_tab(fp, "mov %s, %s", arg2, reg);
    put_line_with_tab(fp, "mov %s, 0x%llx", arg1, op_info->result);
    generate_restore_register(fp, work_reg);
    put_line_with_tab(fp, "call assert_equal_uint%ld", convert_size_to_bit(size));
}


static void generate_test_case_binary_mem_imm(FILE *fp, const BinaryOperationInfo *op_info, size_t size)
{
    const char *arg1 = get_1st_argument_register(size);
    const char *arg2 = get_2nd_argument_register(size);
    const char *size_spec = get_size_specifier(size);

    put_line_with_tab(fp, "mov %s [rbp-8], 0x%llx", size_spec, op_info->lhs);
    put_line_with_tab(fp, "%s %s [rbp-8], 0x%llx    # test target", op_info->mnemonic, size_spec, op_info->rhs);
    put_line_with_tab(fp, "mov %s, %s [rbp-8]", arg2, size_spec);
    put_line_with_tab(fp, "mov %s, 0x%llx", arg1, op_info->result);
    put_line_with_tab(fp, "call assert_equal_uint%ld", convert_size_to_bit(size));
}


static void generate_test_case_binary_mem_reg(FILE *fp, const BinaryOperationInfo *op_info, const RegisterInfo *reg_info)
{
    size_t size = reg_info->size;
    size_t index_list[] = {reg_info->index};
    const char *reg = reg_info->name;
    const char *arg1 = get_1st_argument_register(size);
    const char *arg2 = get_2nd_argument_register(size);
    const char *size_spec = get_size_specifier(size);
    size_t offset = 16;

    const char *work_reg = generate_save_register(fp, index_list, sizeof(index_list) / sizeof(index_list[0]));
    put_line_with_tab(fp, "mov %s [%s-%lu], 0x%llx", size_spec, work_reg, offset, op_info->lhs);
    put_line_with_tab(fp, "mov %s, 0x%llx", reg, op_info->rhs);
    put_line_with_tab(fp, "%s %s [%s-%lu], %s    # test target", op_info->mnemonic, size_spec, work_reg, offset, reg);
    put_line_with_tab(fp, "mov %s, %s [%s-%lu]", arg2, size_spec, work_reg, offset);
    put_line_with_tab(fp, "mov %s, 0x%llx", arg1, op_info->result);
    generate_restore_register(fp, work_reg);
    put_line_with_tab(fp, "call assert_equal_uint%ld", convert_size_to_bit(size));
}


void generate_all_test_case_binary(FILE *fp, const BinaryOperationTestDataMaker *make_test_data)
{
    // <mnemonic> reg, imm
    for(size_t i = 0; i < REG_LIST_SIZE; i++)
    {
        const RegisterInfo *reg_info = &reg_list[i];
        for(size_t j = 0; j < IMM_LIST_SIZE; j++)
        {
            const ImmediateInfo *imm_info = &imm_list[j];
            if((reg_info->size >= imm_info->size) && (imm_info->size <= sizeof(uint32_t)))
            {
                BinaryOperationInfo op_info = make_test_data->reg_imm(imm_info);
                generate_test_case_binary_reg_imm(fp, &op_info, reg_info);
                put_line(fp, "");
            }
        }
    }

    // <mnemonic> reg, reg
    for(size_t i = 0; i < REG_LIST_SIZE; i++)
    {
        const RegisterInfo *reg1_info = &reg_list[i];
        for(size_t j = 0; j < REG_LIST_SIZE; j++)
        {
            const RegisterInfo *reg2_info = &reg_list[j];
            if(reg1_info->size == reg2_info->size)
            {
                BinaryOperationInfo op_info = make_test_data->reg_reg(reg1_info, reg2_info);
                generate_test_case_binary_reg_reg(fp, &op_info, reg1_info, reg2_info);
                put_line(fp, "");
            }
        }
    }

    // <mnemonic> reg, mem
    for(size_t i = 0; i < REG_LIST_SIZE; i++)
    {
        const RegisterInfo *reg_info = &reg_list[i];
        BinaryOperationInfo op_info = make_test_data->reg_mem(reg_info);
        generate_test_case_binary_reg_mem(fp, &op_info, reg_info);
        put_line(fp, "");
    }

    // <mnemonic> mem, imm
    for(size_t j = 0; j < IMM_LIST_SIZE; j++)
    {
        const ImmediateInfo *imm_info = &imm_list[j];
        size_t size = imm_info->size;
        if(size <= sizeof(uint32_t))
        {
            BinaryOperationInfo op_info = make_test_data->mem_imm(imm_info);
            generate_test_case_binary_mem_imm(fp, &op_info, size);
            put_line(fp, "");
        }
    }

    // <mnemonic> mem, reg
    for(size_t i = 0; i < REG_LIST_SIZE; i++)
    {
        const RegisterInfo *reg_info = &reg_list[i];
        BinaryOperationInfo op_info = make_test_data->mem_reg(reg_info);
        generate_test_case_binary_mem_reg(fp, &op_info, reg_info);
        put_line(fp, "");
    }
}
