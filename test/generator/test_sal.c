#include <stddef.h>
#include <stdio.h>

#include "test_common.h"
#include "test_shift_common.h"


static ShiftOperationInfo make_test_data_sal(uintmax_t lhs, uint8_t rhs)
{
    return (ShiftOperationInfo){"sal", lhs, rhs, lhs << rhs};
}


static ShiftOperationInfo make_test_data_sal_common(uint8_t rhs)
{
    return make_test_data_sal(1, rhs);
}


static ShiftOperationInfo make_test_data_sal_reg_reg(const RegisterInfo *reg_info, uint8_t rhs)
{
    uintmax_t lhs = (reg_info->index == REGISTER_INDEX_ECX) ? rhs : 1;
    return make_test_data_sal(lhs, rhs);
}


static void generate_all_test_case_sal(FILE *fp)
{
    static const ShiftOperationTestDataMaker make_test_data_sal =
    {
        make_test_data_sal_common,
        make_test_data_sal_common,
        make_test_data_sal_reg_reg,
        make_test_data_sal_common,
    };

    generate_all_test_case_shift(fp, &make_test_data_sal);
}


void generate_test_sal(void)
{
    generate_test("test/test_sal.s", 0, generate_all_test_case_sal);
}
