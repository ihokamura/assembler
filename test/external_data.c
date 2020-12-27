#include <stdint.h>
#include <stdio.h>

uint32_t test_external_data_uint32 = 32;
uint64_t test_external_data_uint64 = 64;


/*
check the value of test_external_data_uint32
*/
uint32_t assert_external_data_uint32(uint32_t expected)
{
    if(expected == test_external_data_uint32)
    {
        return 0;
    }
    else
    {
        printf("0x%x expected, but got 0x%x.\n", expected, test_external_data_uint32);
        return 1;
    }
}


/*
check the value of test_external_data_uint64
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
