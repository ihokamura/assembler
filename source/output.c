#include "output.h"


/*
fill padding bytes
*/
void fill_paddings(size_t pad_size, FILE *fp)
{
    for(size_t i = 0; i < pad_size; i++)
    {
        char pad_byte = 0x00;
        output_buffer(&pad_byte, sizeof(pad_byte), fp);
    }
}


/*
output buffer
*/
size_t output_buffer(const void *buffer, size_t size, FILE *fp)
{
    return fwrite(buffer, sizeof(char), size, fp);
}
