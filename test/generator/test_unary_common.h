#ifndef __TEST_UNARY_COMMON_H__
#define __TEST_UNARY_COMMON_H__

#include <stdint.h>

typedef struct UnaryOperationInfo UnaryOperationInfo;

struct UnaryOperationInfo
{
    uintmax_t operand;
    uintmax_t result;
};

#endif /* __TEST_UNARY_COMMON_H__ */
