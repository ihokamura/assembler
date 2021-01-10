#include "test_binary_common.h"
#include "test_common.h"


static BinaryOperationInfo make_test_data_add(intmax_t lhs, intmax_t rhs)
{
    return (BinaryOperationInfo){"add", lhs, rhs, lhs + rhs};
}


static BinaryOperationInfo make_test_data_add_reg_imm(const ImmediateInfo *imm_info)
{
    return make_test_data_add(imm_info->sint_max_value - 1, 1);
}


static BinaryOperationInfo make_test_data_add_reg_reg(const RegisterInfo *reg1_info, const RegisterInfo *reg2_info)
{
    if(reg1_info == reg2_info)
    {
        intmax_t operand = convert_size_to_sint_max_value(reg1_info->size) / 2;
        return make_test_data_add(operand, operand);
    }
    else
    {
        return make_test_data_add(convert_size_to_sint_max_value(reg1_info->size) - 1, 1);
    }
}


static BinaryOperationInfo make_test_data_add_reg_mem(const RegisterInfo *reg_info)
{
    return make_test_data_add(convert_size_to_sint_max_value(reg_info->size) - 1, 1);
}


static BinaryOperationInfo make_test_data_add_mem_imm(const ImmediateInfo *imm_info)
{
    return make_test_data_add(imm_info->sint_max_value - 1, 1);
}


static BinaryOperationInfo make_test_data_add_mem_reg(const RegisterInfo *reg_info)
{
    return make_test_data_add(convert_size_to_sint_max_value(reg_info->size) - 1, 1);
}


static void generate_all_test_case_add(FILE *fp)
{
    static const BinaryOperationTestDataMaker make_test_data_add =
    {
        make_test_data_add_reg_imm,
        make_test_data_add_reg_reg,
        make_test_data_add_reg_mem,
        make_test_data_add_mem_imm,
        make_test_data_add_mem_reg
    };

    generate_all_test_case_binary(fp, &make_test_data_add);
}


void generate_test_add(void)
{
    generate_test("test/test_add.s", STACK_ALIGNMENT, generate_all_test_case_add);
}
