#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "output.h"
#include "section.h"

#include "list.h"
define_list_operations(BaseSection)
define_list_operations(Elf_Shdr)

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
static void new_section_header_table_from_base_section(const BaseSection *base_section);
static size_t output_section(const ByteBufferType *buffer, Elf_Off offset, size_t end_pos, FILE *fp);

static const Elf_Addr DEFAULT_SECTION_ADDR = 0;
static const Elf_Word DEFAULT_SECTION_INFO = 0;
static const Elf_Xword DEFAULT_SECTION_ALIGNMENT = 1;
static const Elf_Xword RELA_SECTION_ALIGNMENT = 8;
static const Elf_Xword SYMTAB_SECTION_ALIGNMENT = 8;
static const size_t STRLEN_OF_RELA = 5; // strlen(".rela")

static List(BaseSection) *base_section_list;  // list of base sections
static List(Elf_Shdr) *shdr_list;             // list of section header table entries

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
    shdr_list = new_list(Elf_Shdr)();

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
    const BaseSection *base_section_shstrtab = get_base_section(SC_SHSTRTAB);
    shdr->sh_name = get_strtab_position(base_section_shstrtab->body, section_name, sh_type);
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
static void new_section_header_table_from_base_section(const BaseSection *base_section)
{
    new_section_header_table(
        base_section->name,
        base_section->type,
        base_section->flags,
        base_section->offset,
        base_section->size,
        base_section->link,
        base_section->info,
        base_section->alignment,
        base_section->entry_size);

    if(base_section->rela_body->size > 0)
    {
        // .rela.xxx section
        const BaseSection *base_section_symtab = get_base_section(SC_SYMTAB);
        new_section_header_table(
            base_section->name,
            SHT_RELA,
            SHF_INFO_LINK,
            base_section->rela_offset,
            base_section->rela_body->size,
            base_section_symtab->index, // sh_link holds section header index of the associated symbol table
            base_section->index, // sh_info holds section header index of the section to which the relocation applies
            RELA_SECTION_ALIGNMENT,
            sizeof(Elf_Rela));
    }
}


/*
generate section header table entries
*/
void generate_section_header_table_entries(size_t local_labels)
{
    // set information of metadata sections
    BaseSection *base_section_symtab = get_base_section(SC_SYMTAB);
    BaseSection *base_section_strtab = get_base_section(SC_STRTAB);
    BaseSection *base_section_shstrtab = get_base_section(SC_SHSTRTAB);

    base_section_symtab->size = base_section_symtab->body->size;
    base_section_symtab->link = base_section_strtab->index; // sh_link holds section header index of the associated string table (i.e. .strtab section)
    base_section_symtab->info = local_labels; // sh_info holds one greater than the symbol table index of the laxt local symbol

    base_section_strtab->size = base_section_strtab->body->size;

    base_section_shstrtab->size = base_section_shstrtab->body->size;

    // make section header table entries
    for_each_entry(BaseSection, cursor, base_section_list)
    {
        const BaseSection *base_section = get_element(BaseSection)(cursor);
        new_section_header_table_from_base_section(base_section);
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

    for_each_entry(BaseSection, cursor, base_section_list)
    {
        BaseSection *base_section = get_element(BaseSection)(cursor);
        if(base_section->kind != SC_SHSTRTAB)
        {
            end_pos = output_section(base_section->body, base_section->offset, end_pos, fp);
        }
    }
    for_each_entry(BaseSection, cursor, base_section_list)
    {
        BaseSection *base_section = get_element(BaseSection)(cursor);
        end_pos = output_section(base_section->rela_body, base_section->rela_offset, end_pos, fp);
    }
    {
        BaseSection *base_section = get_base_section(SC_SHSTRTAB);
        end_pos = output_section(base_section->body, base_section->offset, end_pos, fp);
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
