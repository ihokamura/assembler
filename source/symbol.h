#ifndef __IDENTIFIER_H__
#define __IDENTIFIER_H__

#include <stdbool.h>

#include "elf_wrap.h"
#include "tokenizer.h"
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
    Elf_Addr value;
    SectionKind section; // section of symbol
    SectionKind destination;
    unsigned char bind;  // bind of symbol
    bool resolved;       // flag indicating that the symbol is resolved
    bool labeled;        // flag indicating that the symbol is label
    bool declared;       // flag indicating that the symbol is declaration
};

Symbol *new_symbol(const Token *token);
Symbol *set_symbol(Elf_Addr address, Elf_Sxword addend, SectionKind section, Symbol *symbol);
Symbol *search_symbol(const List(Symbol) *symbol_list, const char *body);
void initialize_symbol_list(void);
List(Symbol) *get_symbol_list(void);

#endif /* !__IDENTIFIER_H__ */
