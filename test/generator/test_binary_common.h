#ifndef __TEST_BINARY_COMMON_H__
#define __TEST_BINARY_COMMON_H__

#include <stdint.h>
#include <stdio.h>

#include "test_common.h"

typedef struct BinaryOperationInfo BinaryOperationInfo;
typedef struct BinaryOperationTestDataMaker BinaryOperationTestDataMaker;

struct BinaryOperationInfo
{
    const char *mnemonic;
    intmax_t lhs;
    intmax_t rhs;
    intmax_t result;
};

struct BinaryOperationTestDataMaker
{
    BinaryOperationInfo (*reg_imm)(const ImmediateInfo *);
    BinaryOperationInfo (*reg_reg)(const RegisterInfo *, const RegisterInfo *);
    BinaryOperationInfo (*reg_mem)(const RegisterInfo *);
    BinaryOperationInfo (*mem_imm)(const ImmediateInfo *);
    BinaryOperationInfo (*mem_reg)(const RegisterInfo *);
};

void generate_all_test_case_binary(FILE *fp, const BinaryOperationTestDataMaker *make_test_data);

#endif /* __TEST_BINARY_COMMON_H__ */
