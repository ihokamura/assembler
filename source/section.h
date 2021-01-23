#ifndef __SECTION_H__
#define __SECTION_H__

#include <stddef.h>

#include "elf_wrap.h"

typedef enum SectionKind SectionKind;
typedef struct Bss Bss;
typedef struct Data Data;

// kind of section
enum SectionKind
{
    SC_UND,  // undefined section
    SC_TEXT, // text section
    SC_DATA, // data section
    SC_BSS,  // bss section
};

// structure for bss
struct Bss
{
    size_t size;      // size of data
};

// structure for data
struct Data
{
    size_t size;      // size of data
    uintmax_t value;  // value of data
};

#endif /* !__SECTION_H__ */
