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
    const char *body;     // symbol body
    Elf_Addr value;       // offset from the top of the located section
    Elf_Addr address;     // address where the symbol appeared
    Elf_Sxword addend;    // addend for relocation
    SectionKind appeared; // section where symbol appeared
    SectionKind located;  // section where symbol is located
    unsigned char bind;   // bind of symbol
    bool labeled;         // flag indicating that the symbol is label
    bool declared;        // flag indicating that the symbol is declaration
};

Symbol *new_symbol(const Token *token);
Symbol *set_symbol(Elf_Addr address, Elf_Sxword addend, SectionKind appeared, Symbol *symbol);
Symbol *search_symbol(const List(Symbol) *symbol_list, const char *body);
Symbol *search_symbol_declaration(const List(Symbol) *symbol_list, const char *body);
void initialize_symbol_list(void);
List(Symbol) *get_symbol_list(void);

#endif /* !__IDENTIFIER_H__ */
