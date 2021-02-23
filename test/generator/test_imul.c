#include <stddef.h>
#include <stdio.h>

#include "test_common.h"
#include "test_binary_common.h"


static BinaryOperationInfo make_test_data_imul(intmax_t lhs, intmax_t rhs)
{
    return (BinaryOperationInfo){"imul", lhs, rhs, lhs * rhs};
}


static BinaryOperationInfo make_test_data_imul_reg(const RegisterInfo *reg_info)
{
    intmax_t rhs = 5;
    intmax_t lhs = (reg_info->index == REGISTER_INDEX_EAX) ? rhs : 3;
    return make_test_data_imul(lhs, rhs);
}


static BinaryOperationInfo make_test_data_imul_mem(void)
{
    return make_test_data_imul(3, 5);
}


static void generate_test_case_imul_reg(FILE *fp, const BinaryOperationInfo *op_info, const RegisterInfo *reg_info)
{
    size_t size = reg_info->size;
    size_t index_list[] = {reg_info->index, REGISTER_INDEX_EAX, REGISTER_INDEX_EDX};
    const char *reg = reg_info->name;
    const char *arg1 = get_1st_argument_register(size);
    const char *arg2 = get_2nd_argument_register(size);
    const char *eax_reg = get_register_by_index_and_size(REGISTER_INDEX_EAX, size);

    const char *work_reg = generate_save_register(fp, index_list, sizeof(index_list) / sizeof(index_list[0]));
    put_line_with_tab(fp, "mov %s, 0x%llx", eax_reg, op_info->lhs);
    put_line_with_tab(fp, "mov %s, 0x%llx", reg, op_info->rhs);
    put_line_with_tab(fp, "%s %s   # test target", op_info->mnemonic, reg);
    generate_restore_register(fp, work_reg);
    put_line_with_tab(fp, "mov %s, %s", arg2, eax_reg);
    put_line_with_tab(fp, "mov %s, 0x%llx", arg1, op_info->result);
    put_line_with_tab(fp, "call assert_equal_uint%ld", convert_size_to_bit(size));
}


static void generate_test_case_imul_mem(FILE *fp, const BinaryOperationInfo *op_info, size_t size)
{
    const char *arg1 = get_1st_argument_register(size);
    const char *arg2 = get_2nd_argument_register(size);
    const char *eax_reg = get_register_by_index_and_size(REGISTER_INDEX_EAX, size);
    const char *size_spec = get_size_specifier(size);

    put_line_with_tab(fp, "mov %s, 0x%llx", eax_reg, op_info->lhs);
    put_line_with_tab(fp, "mov %s [rbp-8], 0x%llx", size_spec, op_info->rhs);
    put_line_with_tab(fp, "%s %s [rbp-8]    # test target", op_info->mnemonic, size_spec);
    put_line_with_tab(fp, "mov %s, %s", arg2, eax_reg);
    put_line_with_tab(fp, "mov %s, 0x%llx", arg1, op_info->result);
    put_line_with_tab(fp, "call assert_equal_uint%ld", convert_size_to_bit(size));
}


static void generate_all_test_case_imul(FILE *fp)
{
    // IMUL reg
    for(size_t i = 0; i < REG_LIST_SIZE; i++)
    {
        const RegisterInfo *reg_info = &reg_list[i];
        BinaryOperationInfo op_info = make_test_data_imul_reg(reg_info);
        generate_test_case_imul_reg(fp, &op_info, reg_info);
        put_line(fp, "");
    }

    // IMUL mem
    for(size_t j = 0; j < IMM_LIST_SIZE; j++)
    {
        const ImmediateInfo *imm_info = &imm_list[j];
        BinaryOperationInfo op_info = make_test_data_imul_mem();
        generate_test_case_imul_mem(fp, &op_info, imm_info->size);
        put_line(fp, "");
    }
}


void generate_test_imul(void)
{
    generate_test("test/test_imul.s", STACK_ALIGNMENT, generate_all_test_case_imul);
}
