#include <stddef.h>
#include <stdio.h>

#include "test_common.h"
#include "test_unary_common.h"


static void generate_test_case_neg_reg(FILE *fp, const UnaryOperationInfo *op_info, const RegisterInfo *reg_info)
{
    size_t size = reg_info->size;
    size_t index_list[] = {reg_info->index};
    const char *reg = reg_info->name;
    const char *arg1 = get_1st_argument_register(size);
    const char *arg2 = get_2nd_argument_register(size);

    const char *work_reg = generate_save_register(fp, index_list, sizeof(index_list) / sizeof(index_list[0]));
    put_line_with_tab(fp, "mov %s, 0x%llx", reg, op_info->operand);
    put_line_with_tab(fp, "neg %s    # test target", reg);
    put_line_with_tab(fp, "mov %s, %s", arg2, reg);
    put_line_with_tab(fp, "mov %s, 0x%llx", arg1, op_info->result);
    generate_restore_register(fp, work_reg);
    put_line_with_tab(fp, "call assert_equal_uint%ld", convert_size_to_bit(size));
}


static void generate_test_case_neg_mem(FILE *fp, const UnaryOperationInfo *op_info, size_t size)
{
    const char *arg1 = get_1st_argument_register(size);
    const char *arg2 = get_2nd_argument_register(size);
    const char *size_spec = get_size_specifier(size);
    size_t offset = 16;

    put_line_with_tab(fp, "mov %s [rbp-%lu], 0x%llx", size_spec, offset, op_info->operand);
    put_line_with_tab(fp, "neg %s [rbp-%lu]    # test target", size_spec, offset);
    put_line_with_tab(fp, "mov %s, %s [rbp-%lu]", arg2, size_spec, offset);
    put_line_with_tab(fp, "mov %s, 0x%llx", arg1, op_info->result);
    put_line_with_tab(fp, "call assert_equal_uint%ld", convert_size_to_bit(size));
}


static void generate_all_test_case_neg(FILE *fp)
{
    // NOT reg
    for(size_t i = 0; i < REG_LIST_SIZE; i++)
    {
        const RegisterInfo *reg_info = &reg_list[i];
        uintmax_t operand = 1;
        uintmax_t result = imm_list[convert_size_to_index(reg_info->size)].uint_max_value - operand + 1;
        const UnaryOperationInfo *op_info = &(UnaryOperationInfo){operand, result};
        generate_test_case_neg_reg(fp, op_info, reg_info);
        put_line(fp, "");
    }

    // NOT mem
    for(size_t j = 0; j < IMM_LIST_SIZE; j++)
    {
        const ImmediateInfo *imm_info = &imm_list[j];
        uintmax_t operand = 1;
        uintmax_t result = imm_info->uint_max_value - operand + 1;
        const UnaryOperationInfo *op_info = &(UnaryOperationInfo){operand, result};
        generate_test_case_neg_mem(fp, op_info, imm_info->size);
        put_line(fp, "");
    }
}


void generate_test_neg(void)
{
    generate_test("test/test_neg.s", STACK_ALIGNMENT, generate_all_test_case_neg);
}
