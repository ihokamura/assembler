#ifndef __SECTION_H__
#define __SECTION_H__

#include <stddef.h>
#include <stdio.h>

#include "buffer.h"
#include "elf_wrap.h"

typedef enum SectionKind SectionKind;
typedef struct Bss Bss;
typedef struct Data Data;
typedef struct Section Section;

#include "list.h"
define_list(Elf_Shdr)
define_list(Section)

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

// structure for bss
struct Bss
{
    size_t size;      // size of data
};

// structure for data
struct Data
{
    size_t size;        // size of data
    uintmax_t value;    // value of data
    const char *symbol; // body of symbol
};

// structure for base section
struct Section
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

void initialize_section(void);
void make_metadata_sections(ByteBufferType *symtab_body, ByteBufferType *strtab_body, ByteBufferType *shstrtab_body);
SectionKind get_current_section(void);
void set_current_section(const char *name);
Section *get_section(SectionKind kind);
ByteBufferType *make_shstrtab(ByteBufferType *buffer);
void set_offset_of_sections(void);
void generate_section_header_table_entries(size_t local_labels);
size_t output_section_bodies(size_t start_pos, FILE *fp);
void output_section_header_table_entries(FILE *fp);

#endif /* !__SECTION_H__ */
