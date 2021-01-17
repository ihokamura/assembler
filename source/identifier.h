#ifndef __IDENTIFIER_H__
#define __IDENTIFIER_H__

#include "elf_wrap.h"
#include "processor.h"
#include "section.h"

typedef enum SectionKind SectionKind;
typedef enum SymbolKind SymbolKind;
typedef struct Label Label;
typedef struct Symbol Symbol;

#include "list.h"
define_list(Label)

// kind of section
enum SectionKind
{
    SC_UND,  // undefined section
    SC_TEXT, // text section
    SC_DATA, // data section
    SC_BSS,  // bss section
};

// kind of symbol
enum SymbolKind
{
    SY_GLOBAL, // global symbol
    SY_LOCAL,  // local symbol
};

// structure for label
struct Label
{
    SymbolKind kind;            // kind of symbol
    SectionKind section;        // section of label
    const char *body;           // contents of label
    const Operation *operation; // operation of label
    const Data *data;           // data of label
    const Bss *bss;             // bss of label
};

Symbol *new_symbol(const char *body, Elf_Addr address, Elf_Sxword addend);

#endif /* !__IDENTIFIER_H__ */
