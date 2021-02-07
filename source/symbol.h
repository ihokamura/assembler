#ifndef __IDENTIFIER_H__
#define __IDENTIFIER_H__

#include <stdbool.h>

#include "elf_wrap.h"
#include "section.h"

typedef struct Symbol Symbol;

#include "list.h"
define_list(Symbol)

// structure for symbol
struct Symbol
{
    const char *body;    // symbol body
    Elf_Addr address;    // address to be replaced
    Elf_Sxword addend;   // addend
    SectionKind section; // section of symbol
    bool resolved;       // flag indicating that the symbol is resolved
};

Symbol *new_symbol(const char *body, Elf_Addr address, Elf_Sxword addend, SectionKind section);
void initialize_symbol_list(void);
List(Symbol) *get_symbol_list(void);

#endif /* !__IDENTIFIER_H__ */
