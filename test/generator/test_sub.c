#include "test_binary_common.h"
#include "test_common.h"


static BinaryOperationInfo make_test_data_sub(intmax_t lhs, intmax_t rhs)
{
    return (BinaryOperationInfo){"sub", lhs, rhs, lhs - rhs};
}


static BinaryOperationInfo make_test_data_sub_reg_imm(const ImmediateInfo *imm_info)
{
    return make_test_data_sub(imm_info->sint_max_value, 1);
}


static BinaryOperationInfo make_test_data_sub_reg_reg(const RegisterInfo *reg1_info, const RegisterInfo *reg2_info)
{
    if(reg1_info == reg2_info)
    {
        intmax_t operand = convert_size_to_sint_max_value(reg1_info->size);
        return make_test_data_sub(operand, operand);
    }
    else
    {
        return make_test_data_sub(convert_size_to_sint_max_value(reg1_info->size), 1);
    }
}


static BinaryOperationInfo make_test_data_sub_reg_mem(const RegisterInfo *reg_info)
{
    return make_test_data_sub(convert_size_to_sint_max_value(reg_info->size), 1);
}


static BinaryOperationInfo make_test_data_sub_mem_imm(const ImmediateInfo *imm_info)
{
    return make_test_data_sub(imm_info->sint_max_value, 1);
}


static BinaryOperationInfo make_test_data_sub_mem_reg(const RegisterInfo *reg_info)
{
    return make_test_data_sub(convert_size_to_sint_max_value(reg_info->size), 1);
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
