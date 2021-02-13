#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "output.h"
#include "section.h"

#include "list.h"
define_list_operations(Elf_Shdr)
define_list_operations(Section)

static Section *new_section
(
    SectionKind kind,
    const char *name,
    Elf_Word type,
    Elf_Xword flags,
    Elf_Xword alignment,
    Elf_Xword entry_size
);
static Section *get_section_by_name(const char *name);
static bool has_rela_section(const Section *section);
static void set_index_of_sections(void);
static Elf_Word get_strtab_position(const ByteBufferType *strtab_body, const char *name, Elf_Word sh_type);
static Elf_Shdr *new_section_header_table
(
    const char *section_name,
    Elf_Word sh_type,
    Elf_Xword sh_flags,
    Elf_Off sh_offset,
    Elf_Xword sh_size,
    Elf_Word sh_link,
    Elf_Word sh_info,
    Elf_Xword sh_addralign,
    Elf_Xword sh_entsize
);
static void new_section_header_table_from_section(const Section *section);
static size_t output_section(const ByteBufferType *buffer, Elf_Off offset, size_t end_pos, FILE *fp);

static const Elf_Addr DEFAULT_SECTION_ADDR = 0;
static const Elf_Word DEFAULT_SECTION_INFO = 0;
static const Elf_Xword DEFAULT_SECTION_ALIGNMENT = 1;
static const Elf_Xword RELA_SECTION_ALIGNMENT = 8;
static const Elf_Xword SYMTAB_SECTION_ALIGNMENT = 8;
static const size_t STRLEN_OF_RELA = 5; // strlen(".rela")

static List(Section) *section_list; // list of base sections
static List(Elf_Shdr) *shdr_list;   // list of section header table entries

static SectionKind current_section = SC_TEXT;


/*
make a new base section
*/
static Section *new_section
(
    SectionKind kind,
    const char *name,
    Elf_Word type,
    Elf_Xword flags,
    Elf_Xword alignment,
    Elf_Xword entry_size
)
{
    Section *section = calloc(1, sizeof(Section));
    section->body = calloc(1, sizeof(ByteBufferType));
    section->rela_body = calloc(1, sizeof(ByteBufferType));
    section->kind = kind;
    section->name = name;
    section->type = type;
    section->flags = flags;
    section->link = SHN_UNDEF;
    section->info = DEFAULT_SECTION_INFO;
    section->alignment = alignment;
    section->entry_size = entry_size;
    add_list_entry_tail(Section)(section_list, section);

    return section;
}


/*
initialize base sections
*/
void initialize_section(void)
{
    section_list = new_list(Section)();
    shdr_list = new_list(Elf_Shdr)();

    // make reserved sections
    new_section(SC_UND, "", SHT_NULL, 0, 0, 0);
    new_section(SC_TEXT, ".text", SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR, DEFAULT_SECTION_ALIGNMENT, 0);
    new_section(SC_DATA, ".data", SHT_PROGBITS, SHF_WRITE | SHF_ALLOC, DEFAULT_SECTION_ALIGNMENT, 0);
    new_section(SC_BSS, ".bss", SHT_NOBITS, SHF_WRITE | SHF_ALLOC, DEFAULT_SECTION_ALIGNMENT, 0);
}


/*
make metadata sections
*/
void make_metadata_sections(ByteBufferType *symtab_body, ByteBufferType *strtab_body, ByteBufferType *shstrtab_body)
{
    Section *section_symtab = new_section(SC_SYMTAB, ".symtab", SHT_SYMTAB, 0, SYMTAB_SECTION_ALIGNMENT, sizeof(Elf_Sym));
    section_symtab->body = symtab_body;

    Section *section_strtab = new_section(SC_STRTAB, ".strtab", SHT_STRTAB, 0, DEFAULT_SECTION_ALIGNMENT, 0);
    section_strtab->body = strtab_body;

    Section *section_shstrtab = new_section(SC_SHSTRTAB, ".shstrtab", SHT_STRTAB, 0, DEFAULT_SECTION_ALIGNMENT, 0);
    section_shstrtab->body = shstrtab_body;

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
    Section *section = get_section_by_name(name);
    assert(section != NULL);

    current_section = section->kind;
}


/*
get base section
*/
Section *get_section(SectionKind kind)
{
    for_each_entry(Section, cursor, section_list)
    {
        Section *section = get_element(Section)(cursor);
        if(section->kind == kind)
        {
            return section;
        }
    }

    return NULL;
}


/*
get base section by name
*/
static Section *get_section_by_name(const char *name)
{
    for_each_entry(Section, cursor, section_list)
    {
        Section *section = get_element(Section)(cursor);
        if(strcmp(section->name, name) == 0)
        {
            return section;
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
    for_each_entry(Section, cursor, section_list)
    {
        Section *section = get_element(Section)(cursor);
        if(section->kind != SC_UND)
        {
            if(section->rela_body->size > 0)
            {
                append_bytes(rela_prefix, prefix_size, buffer); // append prefix ".rela" if the section has the associated relocation section
            }
            append_shstrtab(section->name, buffer);
        }
    }

    return buffer;
}


/*
check if the section has the associated relocation section
*/
static bool has_rela_section(const Section *section)
{
    return (section->rela_body->size > 0);
}


/*
set index of sections
*/
static void set_index_of_sections(void)
{
    Elf_Word index = 0;
    for_each_entry(Section, cursor, section_list)
    {
        Section *section = get_element(Section)(cursor);
        section->index = index;
        index++;
        if(has_rela_section(section))
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
    for_each_entry(Section, cursor, section_list)
    {
        Section *section = get_element(Section)(cursor);
        switch(section->kind)
        {
        case SC_UND:
        case SC_SHSTRTAB:
            // set offset in later
            break;

        default:
            section->offset = align_to(offset, section->alignment);
            offset = section->offset + section->body->size;
            break;
        }
    }

    for_each_entry(Section, cursor, section_list)
    {
        Section *section = get_element(Section)(cursor);
        if(has_rela_section(section))
        {
            section->rela_offset = align_to(offset, RELA_SECTION_ALIGNMENT);
            offset = section->rela_offset + section->rela_body->size;
        }
    }

    get_section(SC_UND)->offset = 0;
    get_section(SC_SHSTRTAB)->offset = offset;
}


/*
get string table index of name
*/
static Elf_Word get_strtab_position(const ByteBufferType *strtab_body, const char *name, Elf_Word sh_type)
{
    size_t len = strlen(name);
    size_t range = strtab_body->size - len;

    for(Elf_Word pos = 0; pos < range; pos++)
    {
        const char *start = &strtab_body->body[pos];
        bool equal = true;
        for(size_t i = 0; i < len; i++)
        {
            if(start[i] != name[i])
            {
                equal = false;
                break;
            }
        }

        if(equal)
        {
            if(sh_type == SHT_RELA)
            {
                pos -= STRLEN_OF_RELA;
            }

            return pos;
        }
    }

    return range;
}


/*
set members of a section header table entry
*/
static Elf_Shdr *new_section_header_table
(
    const char *section_name,
    Elf_Word sh_type,
    Elf_Xword sh_flags,
    Elf_Off sh_offset,
    Elf_Xword sh_size,
    Elf_Word sh_link,
    Elf_Word sh_info,
    Elf_Xword sh_addralign,
    Elf_Xword sh_entsize
)
{
    Elf_Shdr *shdr = calloc(1, sizeof(Elf_Shdr));

    // set members
    const Section *section_shstrtab = get_section(SC_SHSTRTAB);
    shdr->sh_name = get_strtab_position(section_shstrtab->body, section_name, sh_type);
    shdr->sh_type = sh_type;
    shdr->sh_flags = sh_flags;
    shdr->sh_addr = DEFAULT_SECTION_ADDR;
    shdr->sh_offset = sh_offset;
    shdr->sh_size = sh_size;
    shdr->sh_link = sh_link;
    shdr->sh_info = sh_info;
    shdr->sh_addralign = sh_addralign;
    shdr->sh_entsize = sh_entsize;

    // update list of section header table entries
    add_list_entry_tail(Elf_Shdr)(shdr_list, shdr);

    return shdr;
}


/*
set members of a section header table entry
*/
static void new_section_header_table_from_section(const Section *section)
{
    new_section_header_table(
        section->name,
        section->type,
        section->flags,
        section->offset,
        section->size,
        section->link,
        section->info,
        section->alignment,
        section->entry_size);

    if(section->rela_body->size > 0)
    {
        // .rela.xxx section
        const Section *section_symtab = get_section(SC_SYMTAB);
        new_section_header_table(
            section->name,
            SHT_RELA,
            SHF_INFO_LINK,
            section->rela_offset,
            section->rela_body->size,
            section_symtab->index, // sh_link holds section header index of the associated symbol table
            section->index, // sh_info holds section header index of the section to which the relocation applies
            RELA_SECTION_ALIGNMENT,
            sizeof(Elf_Rela));
    }
}


/*
generate section header table entries
*/
void generate_section_header_table_entries(size_t symtab_shinfo)
{
    // set information of metadata sections
    Section *section_symtab = get_section(SC_SYMTAB);
    Section *section_strtab = get_section(SC_STRTAB);
    Section *section_shstrtab = get_section(SC_SHSTRTAB);

    section_symtab->size = section_symtab->body->size;
    section_symtab->link = section_strtab->index; // sh_link holds section header index of the associated string table (i.e. .strtab section)
    section_symtab->info = symtab_shinfo; // sh_info holds one greater than the symbol table index of the laxt local symbol

    section_strtab->size = section_strtab->body->size;

    section_shstrtab->size = section_shstrtab->body->size;

    // make section header table entries
    for_each_entry(Section, cursor, section_list)
    {
        const Section *section = get_element(Section)(cursor);
        new_section_header_table_from_section(section);
    }
}


/*
output section body
*/
static size_t output_section(const ByteBufferType *buffer, Elf_Off offset, size_t end_pos, FILE *fp)
{
    size_t size = buffer->size;
    if(size > 0)
    {
        fill_paddings(offset - end_pos, fp);
        output_buffer(buffer->body, size, fp);
        return offset + size;
    }
    else
    {
        return end_pos;
    }
}


/*
output section bodies
*/
size_t output_section_bodies(size_t start_pos, FILE *fp)
{
    size_t end_pos = start_pos;

    for_each_entry(Section, cursor, section_list)
    {
        Section *section = get_element(Section)(cursor);
        if(section->kind != SC_SHSTRTAB)
        {
            end_pos = output_section(section->body, section->offset, end_pos, fp);
        }
    }
    for_each_entry(Section, cursor, section_list)
    {
        Section *section = get_element(Section)(cursor);
        end_pos = output_section(section->rela_body, section->rela_offset, end_pos, fp);
    }
    {
        Section *section = get_section(SC_SHSTRTAB);
        end_pos = output_section(section->body, section->offset, end_pos, fp);
    }

    return end_pos;
}


/*
output section header table entries
*/
void output_section_header_table_entries(FILE *fp)
{
    for_each_entry(Elf_Shdr, cursor, shdr_list)
    {
        const Elf_Shdr *shdr = get_element(Elf_Shdr)(cursor);
        output_buffer(shdr, sizeof(Elf_Shdr), fp);
    }
}
