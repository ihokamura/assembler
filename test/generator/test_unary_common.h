#ifndef TEST_UNARY_COMMON_H
#define TEST_UNARY_COMMON_H

#include <stdint.h>

typedef struct UnaryOperationInfo UnaryOperationInfo;

struct UnaryOperationInfo
{
    uintmax_t operand;
    uintmax_t result;
};

#endif /* TEST_UNARY_COMMON_H */
