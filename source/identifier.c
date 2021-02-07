#include <stdlib.h>

#include "identifier.h"

#include "list.h"
define_list_operations(Symbol)

// global variable
static List(Symbol) *symbol_list; // list of symbols


/*
make a new symbol
*/
Symbol *new_symbol(const char *body, Elf_Addr address, Elf_Sxword addend, SectionKind section)
{
    Symbol *symbol = calloc(1, sizeof(Symbol));
    symbol->body = body;
    symbol->address = address;
    symbol->addend = addend;
    symbol->section = section;
    symbol->resolved = false;
    add_list_entry_tail(Symbol)(symbol_list, symbol);

    return symbol;
}


/*
initialize list of symbols
*/
void initialize_symbol_list(void)
{
    symbol_list = new_list(Symbol)();
}


/*
get list of symbols
*/
List(Symbol) *get_symbol_list(void)
{
    return symbol_list;
}
