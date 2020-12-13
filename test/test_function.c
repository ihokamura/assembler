#include <stdio.h>
#include <stdint.h>


/*
check if two values are equal
*/
uint32_t assert_equal(uint64_t expected, uint64_t actual)
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
