#include <stdint.h>
#include <stdio.h>

uint64_t test_external_data_uint64 = 64;


/*
check the value of test_external_data_uint64_1
*/
uint32_t assert_external_data_uint64(uint64_t expected)
{
    if(expected == test_external_data_uint64)
    {
        return 0;
    }
    else
    {
        printf("0x%lx expected, but got 0x%lx.\n", expected, test_external_data_uint64);
        return 1;
    }
}
