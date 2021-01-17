#ifndef __IDENTIFIER_H__
#define __IDENTIFIER_H__

#include "elf_wrap.h"
#include "processor.h"
#include "section.h"

typedef enum LabelKind LabelKind;
typedef enum SectionKind SectionKind;
typedef struct Label Label;
typedef struct Symbol Symbol;

#include "list.h"
define_list(Label)

// kind of label
enum LabelKind
{
    LB_GLOBAL, // global label
    LB_LOCAL,  // local label
};

// kind of section
enum SectionKind
{
    SC_UND,  // undefined section
    SC_TEXT, // text section
    SC_DATA, // data section
    SC_BSS,  // bss section
};

// structure for label
struct Label
{
    LabelKind kind;            // kind of symbol
    SectionKind section;        // section of label
    const char *body;           // contents of label
    const Operation *operation; // operation of label
    const Data *data;           // data of label
    const Bss *bss;             // bss of label
};

Symbol *new_symbol(const char *body, Elf_Addr address, Elf_Sxword addend);

#endif /* !__IDENTIFIER_H__ */
