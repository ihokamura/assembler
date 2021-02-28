#ifndef OUTPUT_H
#define OUTPUT_H

#include <stddef.h>
#include <stdio.h>

void fill_paddings(size_t pad_size, FILE *fp);
size_t output_buffer(const void *buffer, size_t size, FILE *fp);

#endif /* !OUTPUT_H */
