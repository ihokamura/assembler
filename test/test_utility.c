#include <stdio.h>
#include <stdint.h>


/*
check if two 16-bit values are equal
*/
uint32_t assert_equal_uint16(uint16_t expected, uint16_t actual)
{
    if(expected == actual)
    {
        return 0;
    }
    else
    {
        printf("0x%x expected, but got 0x%x.\n", expected, actual);
        return 1;
    }
}


/*
check if two 32-bit values are equal
*/
uint32_t assert_equal_uint32(uint32_t expected, uint32_t actual)
{
    if(expected == actual)
    {
        return 0;
    }
    else
    {
        printf("0x%x expected, but got 0x%x.\n", expected, actual);
        return 1;
    }
}


/*
check if two 64-bit values are equal
*/
uint32_t assert_equal_uint64(uint64_t expected, uint64_t actual)
{
    if(expected == actual)
    {
        return 0;
    }
    else
    {
        printf("0x%lx expected, but got 0x%lx.\n", expected, actual);
        return 1;
    }
}
