#include <stdint.h>
#include <stdio.h>

uint8_t test_external_data_uint8 = 8;
uint16_t test_external_data_uint16 = 16;
uint32_t test_external_data_uint32 = 32;
uint64_t test_external_data_uint64 = 64;


/*
check the value of test_external_data_uint8
*/
uint32_t assert_external_data_uint8(uint8_t actual)
{
    if(actual == test_external_data_uint8)
    {
        return 0;
    }
    else
    {
        printf("0x%x expected, but got 0x%x.\n", test_external_data_uint8, actual);
        return 1;
    }
}


/*
check the value of pointer to test_external_data_uint8
*/
uint32_t assert_pointer_to_external_data_uint8(const uint8_t *actual)
{
    if(actual == &test_external_data_uint8)
    {
        return 0;
    }
    else
    {
        printf("%p expected, but got %p.\n", &test_external_data_uint8, actual);
        return 1;
    }
}


/*
check the value of test_external_data_uint16
*/
uint32_t assert_external_data_uint16(uint16_t actual)
{
    if(actual == test_external_data_uint16)
    {
        return 0;
    }
    else
    {
        printf("0x%x expected, but got 0x%x.\n", test_external_data_uint16, actual);
        return 1;
    }
}


/*
check the value of pointer to test_external_data_uint16
*/
uint32_t assert_pointer_to_external_data_uint16(const uint16_t *actual)
{
    if(actual == &test_external_data_uint16)
    {
        return 0;
    }
    else
    {
        printf("%p expected, but got %p.\n", &test_external_data_uint16, actual);
        return 1;
    }
}


/*
check the value of test_external_data_uint32
*/
uint32_t assert_external_data_uint32(uint32_t actual)
{
    if(actual == test_external_data_uint32)
    {
        return 0;
    }
    else
    {
        printf("0x%x expected, but got 0x%x.\n", test_external_data_uint32, actual);
        return 1;
    }
}


/*
check the value of pointer to test_external_data_uint32
*/
uint32_t assert_pointer_to_external_data_uint32(const uint32_t *actual)
{
    if(actual == &test_external_data_uint32)
    {
        return 0;
    }
    else
    {
        printf("%p expected, but got %p.\n", &test_external_data_uint32, actual);
        return 1;
    }
}


/*
check the value of test_external_data_uint64
*/
uint32_t assert_external_data_uint64(uint64_t actual)
{
    if(actual == test_external_data_uint64)
    {
        return 0;
    }
    else
    {
        printf("0x%lx expected, but got 0x%lx.\n", test_external_data_uint64, actual);
        return 1;
    }
}


/*
check the value of pointer to test_external_data_uint64
*/
uint32_t assert_pointer_to_external_data_uint64(const uint64_t *actual)
{
    if(actual == &test_external_data_uint64)
    {
        return 0;
    }
    else
    {
        printf("%p expected, but got %p.\n", &test_external_data_uint64, actual);
        return 1;
    }
}
