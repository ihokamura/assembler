#include <stdlib.h>
#include <string.h>

#include "symbol.h"

#include "list.h"
define_list_operations(Symbol)

// global variable
static List(Symbol) *symbol_list; // list of symbols


/*
make a new symbol
*/
Symbol *new_symbol(const Token *token)
{
    Symbol *symbol = calloc(1, sizeof(Symbol));
    symbol->body = make_identifier(token);
    symbol->value = 0;
    symbol->address = 0;
    symbol->addend = 0;
    symbol->appeared = SC_UND;
    symbol->located = SC_UND;
    symbol->bind = STB_LOCAL;
    symbol->labeled = false;
    symbol->declared = false;
    add_list_entry_tail(Symbol)(symbol_list, symbol);

    return symbol;
}


/*
set information of symbol
*/
Symbol *set_symbol(Elf_Addr address, Elf_Sxword addend, SectionKind appeared, Symbol *symbol)
{
    symbol->address = address;
    symbol->addend = addend;
    symbol->appeared = appeared;

    return symbol;
}


/*
search symbol by name
*/
Symbol *search_symbol(const List(Symbol) *symbol_list, const char *body)
{
    for_each_entry(Symbol, cursor, symbol_list)
    {
        Symbol *symbol = get_element(Symbol)(cursor);
        if(strcmp(symbol->body, body) == 0)
        {
            return symbol;
        }
    }

    return NULL;
}


/*
search declared symbol by name
*/
Symbol *search_symbol_declaration(const List(Symbol) *symbol_list, const char *body)
{
    for_each_entry(Symbol, cursor, symbol_list)
    {
        Symbol *symbol = get_element(Symbol)(cursor);
        if(symbol->declared && strcmp(symbol->body, body) == 0)
        {
            return symbol;
        }
    }

    return NULL;
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
