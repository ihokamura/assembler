#ifndef __TEST_BINARY_COMMON_H__
#define __TEST_BINARY_COMMON_H__

#include <stdint.h>
#include <stdio.h>

void generate_all_test_case_binary(FILE *fp, const char *mnemonic, uintmax_t (*binary_operation)(uintmax_t, uintmax_t));

#endif /* __TEST_BINARY_COMMON_H__ */
