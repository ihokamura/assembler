#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "section.h"

#include "list.h"
define_list(BaseSection)
define_list_operations(BaseSection)

static BaseSection *new_base_section(SectionKind kind, const char *name);
static BaseSection *get_base_section_by_name(const char *name);

static List(BaseSection) *base_section_list;  // list of base sections

static SectionKind current_section = SC_TEXT;


/*
make a new base section
*/
static BaseSection *new_base_section(SectionKind kind, const char *name)
{
    BaseSection *base_section = calloc(1, sizeof(BaseSection));
    base_section->kind = kind;
    base_section->name = name;
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
    new_base_section(SC_TEXT, ".text");
    new_base_section(SC_DATA, ".data");
    new_base_section(SC_BSS, ".bss");
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
        if(base_section->rela_body.size > 0)
        {
            append_bytes(rela_prefix, prefix_size, buffer); // append prefix ".rela" if the section has the associated relocation section
        }
        append_shstrtab(base_section->name, buffer);
    }

    return buffer;
}
