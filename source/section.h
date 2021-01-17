#ifndef __SECTION_H__
#define __SECTION_H__

#include <stddef.h>

#include "elf_wrap.h"

typedef struct Bss Bss;
typedef struct Data Data;

#include "list.h"
define_list(Bss)
define_list(Data)

// structure for bss
struct Bss
{
    size_t size;      // size of data
    Elf_Addr address; // address of data
};

// structure for data
struct Data
{
    size_t size;      // size of data
    uintmax_t value;  // value of data
    Elf_Addr address; // address of data
};

#endif /* !__SECTION_H__ */
