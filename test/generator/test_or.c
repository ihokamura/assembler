#include "test_binary_common.h"
#include "test_common.h"


static BinaryOperationInfo make_test_data_or(intmax_t lhs, intmax_t rhs)
{
    return (BinaryOperationInfo){"or", lhs, rhs, lhs | rhs};
}


static BinaryOperationInfo make_test_data_or_reg_imm(const ImmediateInfo *imm_info)
{
    return make_test_data_or(imm_info->sint_max_value, 1);
}


static BinaryOperationInfo make_test_data_or_reg_reg(const RegisterInfo *reg1_info, const RegisterInfo *reg2_info)
{
    if(reg1_info == reg2_info)
    {
        intmax_t operand = convert_size_to_sint_max_value(reg1_info->size);
        return make_test_data_or(operand, operand);
    }
    else
    {
        return make_test_data_or(convert_size_to_sint_max_value(reg1_info->size), 1);
    }
}


static BinaryOperationInfo make_test_data_or_reg_mem(const RegisterInfo *reg_info)
{
    return make_test_data_or(convert_size_to_sint_max_value(reg_info->size), 1);
}


static BinaryOperationInfo make_test_data_or_mem_imm(const ImmediateInfo *imm_info)
{
    return make_test_data_or(imm_info->sint_max_value, 1);
}


static BinaryOperationInfo make_test_data_or_mem_reg(const RegisterInfo *reg_info)
{
    return make_test_data_or(convert_size_to_sint_max_value(reg_info->size), 1);
}


static void generate_all_test_case_or(FILE *fp)
{
    static const BinaryOperationTestDataMaker make_test_data_or =
    {
        make_test_data_or_reg_imm,
        make_test_data_or_reg_reg,
        make_test_data_or_reg_mem,
        make_test_data_or_mem_imm,
        make_test_data_or_mem_reg
    };

    generate_all_test_case_binary(fp, &make_test_data_or);
}


void generate_test_or(void)
{
    generate_test("test/test_or.s", STACK_ALIGNMENT, generate_all_test_case_or);
}
