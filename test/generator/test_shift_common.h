#ifndef __TEST_SHIFT_COMMON_H__
#define __TEST_SHIFT_COMMON_H__

#include <stdint.h>
#include <stdio.h>

typedef struct ShiftOperationInfo ShiftOperationInfo;
typedef struct ShiftOperationTestDataMaker ShiftOperationTestDataMaker;

struct ShiftOperationInfo
{
    const char *mnemonic;
    uintmax_t lhs;
    uint8_t rhs;
    uintmax_t result;
};

struct ShiftOperationTestDataMaker
{
    ShiftOperationInfo (*reg_imm)(uint8_t);
    ShiftOperationInfo (*mem_imm)(uint8_t);
    ShiftOperationInfo (*reg_reg)(const RegisterInfo *, uint8_t);
    ShiftOperationInfo (*mem_reg)(uint8_t);
};

void generate_all_test_case_shift(FILE *fp, const ShiftOperationTestDataMaker *make_test_data);

#endif /* __TEST_SHIFT_COMMON_H__ */
