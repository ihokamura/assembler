#include <stdlib.h>
#include <string.h>

#include "buffer.h"


static const size_t REALLOC_SIZE = 1024;


/*
align value
*/
size_t align_to(size_t n, size_t alignment)
{
    return (n + (alignment - 1)) & ~(alignment - 1);
}


/*
append bytes to buffer
*/
ByteBufferType *append_bytes(const char *bytes, size_t size, ByteBufferType *buffer)
{
    if(buffer->size + size > buffer->capacity)
    {
        buffer->capacity += align_to(size, REALLOC_SIZE);
        buffer->body = realloc(buffer->body, buffer->capacity);
    }

    memcpy(&buffer->body[buffer->size], bytes, size);
    buffer->size += size;

    return buffer;
}


/*
fill value to buffer
*/
ByteBufferType *fill_bytes(char byte, size_t size, ByteBufferType *buffer)
{
    for(size_t i = 0; i < size; i++)
    {
        append_bytes(&byte, sizeof(byte), buffer);
    }

    return buffer;
}
