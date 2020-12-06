#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <stddef.h>

typedef struct
{
    char *body;
    size_t capacity;
    size_t size;
} ByteBufferType;

ByteBufferType *append_bytes(const char *bytes, size_t size, ByteBufferType *buffer);

#endif /* !__BUFFER_H__ */
