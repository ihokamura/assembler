#ifndef __SECTION_H__
#define __SECTION_H__

#include <stddef.h>

#include "buffer.h"
#include "elf_wrap.h"

typedef enum SectionKind SectionKind;
typedef struct BaseSection BaseSection;
typedef struct Bss Bss;
typedef struct Data Data;

// kind of section
enum SectionKind
{
    SC_UND,    // undefined section
    SC_TEXT,   // .text section
    SC_DATA,   // .data section
    SC_BSS,    // .bss section
    SC_CUSTOM, // custom section
};

// structure for base section
struct BaseSection
{
    SectionKind kind;         // kind of section
    const char *name;         // name of section
    ByteBufferType body;      // body of section
    ByteBufferType rela_body; // body of relocation section
    Elf_Xword size;           // size of section
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

void initialize_base_section(void);
SectionKind get_current_section(void);
void set_current_section(const char *name);
BaseSection *get_base_section(SectionKind kind);
ByteBufferType *make_shstrtab(ByteBufferType *buffer);

#endif /* !__SECTION_H__ */
