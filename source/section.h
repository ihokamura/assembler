#ifndef __SECTION_H__
#define __SECTION_H__

#include <stddef.h>

#include "buffer.h"
#include "elf_wrap.h"

typedef enum SectionKind SectionKind;
typedef struct BaseSection BaseSection;
typedef struct Bss Bss;
typedef struct Data Data;

#include "list.h"
define_list(BaseSection)

// kind of section
enum SectionKind
{
    SC_UND,      // undefined section
    SC_TEXT,     // .text section
    SC_DATA,     // .data section
    SC_BSS,      // .bss section
    SC_SYMTAB,   // .symtab section
    SC_STRTAB,   // .strtab section
    SC_SHSTRTAB, // .shstrtab section
    SC_CUSTOM,   // custom section
};

// structure for base section
struct BaseSection
{
    SectionKind kind;          // kind of section
    const char *name;          // name of section
    ByteBufferType *body;      // body of section
    ByteBufferType *rela_body; // body of relocation section
    Elf_Off rela_offset;       // offset of corresponding relocation section
    Elf_Word index;            // index of section
    Elf_Word type;             // type of section
    Elf_Xword flags;           // flagss of section
    Elf_Off offset;            // offset of section
    Elf_Xword size;            // size of section
    Elf_Word link;             // link to another section
    Elf_Word info;             // dditional section information
    Elf_Xword alignment;       // alignment of section
    Elf_Xword entry_size;      // entry size of table in section (if exists)
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

extern List(BaseSection) *base_section_list;
extern const Elf_Xword RELA_SECTION_ALIGNMENT;

void initialize_base_section(void);
void make_metadata_sections(ByteBufferType *symtab_body, ByteBufferType *strtab_body, ByteBufferType *shstrtab_body);
SectionKind get_current_section(void);
void set_current_section(const char *name);
BaseSection *get_base_section(SectionKind kind);
ByteBufferType *make_shstrtab(ByteBufferType *buffer);
void set_offset_of_sections(void);

#endif /* !__SECTION_H__ */
