#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "section.h"

#include "list.h"
define_list_operations(BaseSection)

static BaseSection *new_base_section
(
    SectionKind kind,
    const char *name,
    Elf_Word type,
    Elf_Xword flags,
    Elf_Xword alignment,
    Elf_Xword entry_size
);
static BaseSection *get_base_section_by_name(const char *name);
static bool has_rela_section(const BaseSection *base_section);
static void set_index_of_sections(void);

List(BaseSection) *base_section_list;  // list of base sections

static const Elf_Word DEFAULT_SECTION_INFO = 0;
static const Elf_Xword DEFAULT_SECTION_ALIGNMENT = 1;
const Elf_Xword RELA_SECTION_ALIGNMENT = 8;
static const Elf_Xword SYMTAB_SECTION_ALIGNMENT = 8;

static SectionKind current_section = SC_TEXT;


/*
make a new base section
*/
static BaseSection *new_base_section
(
    SectionKind kind,
    const char *name,
    Elf_Word type,
    Elf_Xword flags,
    Elf_Xword alignment,
    Elf_Xword entry_size
)
{
    BaseSection *base_section = calloc(1, sizeof(BaseSection));
    base_section->body = calloc(1, sizeof(ByteBufferType));
    base_section->rela_body = calloc(1, sizeof(ByteBufferType));
    base_section->kind = kind;
    base_section->name = name;
    base_section->type = type;
    base_section->flags = flags;
    base_section->link = SHN_UNDEF;
    base_section->info = DEFAULT_SECTION_INFO;
    base_section->alignment = alignment;
    base_section->entry_size = entry_size;
    add_list_entry_tail(BaseSection)(base_section_list, base_section);

    return base_section;
}


/*
initialize base sections
*/
void initialize_base_section(void)
{
    base_section_list = new_list(BaseSection)();

    // make reserved sections
    new_base_section(SC_UND, "", SHT_NULL, 0, 0, 0);
    new_base_section(SC_TEXT, ".text", SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR, DEFAULT_SECTION_ALIGNMENT, 0);
    new_base_section(SC_DATA, ".data", SHT_PROGBITS, SHF_WRITE | SHF_ALLOC, DEFAULT_SECTION_ALIGNMENT, 0);
    new_base_section(SC_BSS, ".bss", SHT_NOBITS, SHF_WRITE | SHF_ALLOC, DEFAULT_SECTION_ALIGNMENT, 0);
}


/*
make metadata sections
*/
void make_metadata_sections(ByteBufferType *symtab_body, ByteBufferType *strtab_body, ByteBufferType *shstrtab_body)
{
    BaseSection *base_section_symtab = new_base_section(SC_SYMTAB, ".symtab", SHT_SYMTAB, 0, SYMTAB_SECTION_ALIGNMENT, sizeof(Elf_Sym));
    base_section_symtab->body = symtab_body;

    BaseSection *base_section_strtab = new_base_section(SC_STRTAB, ".strtab", SHT_STRTAB, 0, DEFAULT_SECTION_ALIGNMENT, 0);
    base_section_strtab->body = strtab_body;

    BaseSection *base_section_shstrtab = new_base_section(SC_SHSTRTAB, ".shstrtab", SHT_STRTAB, 0, DEFAULT_SECTION_ALIGNMENT, 0);
    base_section_shstrtab->body = shstrtab_body;

    set_index_of_sections();
}


/*
get current section
*/
SectionKind get_current_section(void)
{
    return current_section;
}


/*
set current section
*/
void set_current_section(const char *name)
{
    BaseSection *base_section = get_base_section_by_name(name);
    assert(base_section != NULL);

    current_section = base_section->kind;
}


/*
get base section
*/
BaseSection *get_base_section(SectionKind kind)
{
    for_each_entry(BaseSection, cursor, base_section_list)
    {
        BaseSection *base_section = get_element(BaseSection)(cursor);
        if(base_section->kind == kind)
        {
            return base_section;
        }
    }

    return NULL;
}


/*
get base section by name
*/
static BaseSection *get_base_section_by_name(const char *name)
{
    for_each_entry(BaseSection, cursor, base_section_list)
    {
        BaseSection *base_section = get_element(BaseSection)(cursor);
        if(strcmp(base_section->name, name) == 0)
        {
            return base_section;
        }
    }

    return NULL;
}


static ByteBufferType *append_shstrtab(const char *name, ByteBufferType *buffer)
{
    return append_bytes(name, strlen(name) + 1, buffer);
}


/*
make section name string table
*/
ByteBufferType *make_shstrtab(ByteBufferType *buffer)
{
    // add names of indispensable sections
    append_shstrtab("", buffer);
    append_shstrtab(".symtab", buffer);
    append_shstrtab(".strtab", buffer);
    append_shstrtab(".shstrtab", buffer);

    // add names of defined sections
    static const char *rela_prefix = ".rela";
    const size_t prefix_size = strlen(rela_prefix);
    for_each_entry(BaseSection, cursor, base_section_list)
    {
        BaseSection *base_section = get_element(BaseSection)(cursor);
        if(base_section->kind != SC_UND)
        {
            if(base_section->rela_body->size > 0)
            {
                append_bytes(rela_prefix, prefix_size, buffer); // append prefix ".rela" if the section has the associated relocation section
            }
            append_shstrtab(base_section->name, buffer);
        }
    }

    return buffer;
}


/*
check if the section has the associated relocation section
*/
static bool has_rela_section(const BaseSection *base_section)
{
    return (base_section->rela_body->size > 0);
}


/*
set index of sections
*/
static void set_index_of_sections(void)
{
    Elf_Word index = 0;
    for_each_entry(BaseSection, cursor, base_section_list)
    {
        BaseSection *base_section = get_element(BaseSection)(cursor);
        base_section->index = index;
        index++;
        if(has_rela_section(base_section))
        {
            index++;
        }
    }
}


/*
set offset of sections
*/
void set_offset_of_sections(void)
{
    Elf_Off offset = sizeof(Elf_Ehdr);
    for_each_entry(BaseSection, cursor, base_section_list)
    {
        BaseSection *base_section = get_element(BaseSection)(cursor);
        switch(base_section->kind)
        {
        case SC_UND:
        case SC_SHSTRTAB:
            // set offset in later
            break;

        default:
            base_section->offset = align_to(offset, base_section->alignment);
            offset = base_section->offset + base_section->body->size;
            break;
        }
    }

    for_each_entry(BaseSection, cursor, base_section_list)
    {
        BaseSection *base_section = get_element(BaseSection)(cursor);
        if(has_rela_section(base_section))
        {
            base_section->rela_offset = align_to(offset, RELA_SECTION_ALIGNMENT);
            offset = base_section->rela_offset + base_section->rela_body->size;
        }
    }

    get_base_section(SC_UND)->offset = 0;
    get_base_section(SC_SHSTRTAB)->offset = offset;
}
