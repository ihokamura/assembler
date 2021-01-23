#include <stdlib.h>

#include "section.h"

#include "list.h"
define_list(BaseSection)
define_list_operations(BaseSection)

static BaseSection *new_base_section(SectionKind kind);

static List(BaseSection) *base_section_list;  // list of base sections

static SectionKind current_section = SC_TEXT;


/*
make a new base section
*/
static BaseSection *new_base_section(SectionKind kind)
{
    BaseSection *base_section = calloc(1, sizeof(BaseSection));
    base_section->kind = kind;
    add_list_entry_tail(BaseSection)(base_section_list, base_section);

    return base_section;
}


/*
initialize base sections
*/
void initialize_base_section(void)
{
    base_section_list = new_list(BaseSection)();

    // make indispensable sections
    new_base_section(SC_TEXT);
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
void set_current_section(SectionKind kind)
{
    BaseSection *base_section = get_base_section(kind);
    if(base_section == NULL)
    {
        new_base_section(kind);
    }

    current_section = kind;
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

    return new_base_section(kind);
}
