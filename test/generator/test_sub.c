#include "test_binary_common.h"
#include "test_common.h"


static BinaryOperationInfo make_test_data_sub_reg_imm(const ImmediateInfo *imm_info)
{
    intmax_t lhs = imm_info->sint_max_value;
    intmax_t rhs = 1;
    intmax_t result = lhs - rhs;

    return (BinaryOperationInfo){"sub", lhs, rhs, result};
}


static BinaryOperationInfo make_test_data_sub_reg_reg(const RegisterInfo *reg1_info, const RegisterInfo *reg2_info)
{
    intmax_t lhs = convert_size_to_sint_max_value(reg1_info->size);
    intmax_t rhs = 1;
    intmax_t result = (reg1_info->index == reg2_info->index) ? 0 : lhs - rhs;

    return (BinaryOperationInfo){"sub", lhs, rhs, result};
}


static BinaryOperationInfo make_test_data_sub_reg_mem(const RegisterInfo *reg_info)
{
    intmax_t lhs = convert_size_to_sint_max_value(reg_info->size);
    intmax_t rhs = 1;
    intmax_t result = lhs - rhs;

    return (BinaryOperationInfo){"sub", lhs, rhs, result};
}


static BinaryOperationInfo make_test_data_sub_mem_imm(const ImmediateInfo *imm_info)
{
    intmax_t lhs = imm_info->sint_max_value;
    intmax_t rhs = 1;
    intmax_t result = lhs - rhs;

    return (BinaryOperationInfo){"sub", lhs, rhs, result};
}


static BinaryOperationInfo make_test_data_sub_mem_reg(const RegisterInfo *reg_info)
{
    intmax_t lhs = convert_size_to_sint_max_value(reg_info->size);
    intmax_t rhs = 1;
    intmax_t result = lhs - rhs;

    return (BinaryOperationInfo){"sub", lhs, rhs, result};
}


static void generate_all_test_case_sub(FILE *fp)
{
    static const BinaryOperationTestDataMaker make_test_data_sub =
    {
        make_test_data_sub_reg_imm,
        make_test_data_sub_reg_reg,
        make_test_data_sub_reg_mem,
        make_test_data_sub_mem_imm,
        make_test_data_sub_mem_reg
    };

    generate_all_test_case_binary(fp, &make_test_data_sub);
}


void generate_test_sub(void)
{
    const char *filename = "test/test_sub.s";
    const size_t stack_size = 16;
    generate_test(filename, stack_size, generate_all_test_case_sub);
}
