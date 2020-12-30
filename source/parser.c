#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "processor.h"
#include "tokenizer.h"

#include "list.h"
define_list_operations(Operation)
define_list_operations(Operand)
define_list_operations(Symbol)

// function prototype
static void program(void);
static void statement(void);
static Directive *directive(void);
static Symbol *symbol(const Token *token);
static Operation *operation(const Token *token);
static const MnemonicInfo *mnemonic(const Token *token);
static List(Operand) *operands(void);
static Operand *operand(void);
static Directive *new_directive(const Symbol *symbol);
static Symbol *new_symbol(SymbolKind kind, const char *body);
static Operation *new_operation(MnemonicKind kind, const List(Operand) *operands);
static Operand *new_operand(OperandKind kind);
static Operand *new_operand_immediate(uint32_t immediate);
static Operand *new_operand_register(const Token *token);
static Operand *new_operand_memory(OperandKind kind);
static Operand *new_operand_symbol(const Token *token);
static const RegisterInfo *get_register_info(const Token *token);
static bool consume_size_specifier(OperandKind *kind);

// global variable
static List(Operation) *operation_list = NULL; // list of operations
static List(Symbol) *symbol_list = NULL; // list of symbols


/*
construct program
*/
void construct(Program *prog)
{
    operation_list = new_list(Operation)();
    symbol_list = new_list(Symbol)();

    program();
    prog->operations = operation_list;
    prog->symbols = symbol_list;
}


/*
parse a program
```
program ::= statement*
```
*/
static void program(void)
{
    while(!at_eof())
    {
        // parse statement
        statement();
    }
}


/*
parse a statement
```
statement ::= (symbol ":")? directive | operation
```
*/
static void statement(void)
{
    Token *token;
    Symbol *sym;
    if(consume_token(TK_SYMBOL, &token))
    {
        sym = symbol(token);
        expect_reserved(":");
    }

    if(consume_reserved("."))
    {
        // symbol is ignored if exists
        directive();
    }
    else if(consume_token(TK_MNEMONIC, &token))
    {
        Operation *op = operation(token);
        sym->operation = op;
    }
}


/*
parse a directive
```
directive ::= ".intel_syntax noprefix"
            | ".globl" symbol
```
*/
static Directive *directive(void)
{
    if(consume_reserved("intel_syntax noprefix"))
    {
        return new_directive(NULL);
    }
    else if(consume_reserved("globl"))
    {
        Symbol *sym = symbol(expect_symbol());
        sym->kind = SY_GLOBAL; // overwrite the kind
        return new_directive(sym);
    }
    else
    {
        report_error(NULL, "expected directive.");
        return NULL;
    }
}


/*
parse a symbol
*/
static Symbol *symbol(const Token *token)
{
    const char *body = make_symbol(token);

    // serch the existing symbols
    for_each_entry(Symbol, cursor, symbol_list)
    {
        Symbol *symbol = get_element(Symbol)(cursor);
        if(strcmp(body, symbol->body) == 0)
        {
            return symbol;
        }
    }

    return new_symbol(SY_LOCAL, body);
}


/*
parse an operation
```
operation ::= mnemonic operands?
```
*/
static Operation *operation(const Token *token)
{
    const MnemonicInfo *map = mnemonic(token);

    return new_operation(map->kind, map->take_operands ? operands() : NULL);
}


/*
parse a mnemonic
*/
static const MnemonicInfo *mnemonic(const Token *token)
{
    for(size_t i = 0; i < MNEMONIC_INFO_LIST_SIZE; i++)
    {
        if(strncmp(token->str, mnemonic_info_list[i].name, token->len) == 0)
        {
            return &mnemonic_info_list[i];
        }
    }

    report_error(NULL, "invalid mnemonic '%s.", make_symbol(token));

    return &mnemonic_info_list[MN_NOP];
}


/*
parse operands
```
operands ::= operand ("," operand)?
```
*/
static List(Operand) *operands(void)
{
    List(Operand) *operand_list = new_list(Operand)();
    add_list_entry_tail(Operand)(operand_list, operand());
    while(consume_reserved(","))
    {
        add_list_entry_tail(Operand)(operand_list, operand());
    }

    return operand_list;
}


/*
parse an operand
```
operand ::= immediate | register | memory | symbol
```
*/
static Operand *operand(void)
{
    Token *token;
    OperandKind kind;
    if(consume_token(TK_IMMEDIATE, &token))
    {
        return new_operand_immediate(token->value);
    }
    else if(consume_token(TK_REGISTER, &token))
    {
        return new_operand_register(token);
    }
    else if(consume_size_specifier(&kind))
    {
        return new_operand_memory(kind);
    }
    else if(consume_token(TK_SYMBOL, &token))
    {
        return new_operand_symbol(token);
    }
    else
    {
        report_error(NULL, "expected immediate, register or symbol.");
        return NULL;
    }
}


/*
make a new directive
*/
static Directive *new_directive(const Symbol *symbol)
{
    Directive *directive = calloc(1, sizeof(Directive));
    directive->symbol = symbol;

    return directive;
}


/*
make a new symbol
*/
static Symbol *new_symbol(SymbolKind kind, const char *body)
{
    Symbol *symbol = calloc(1, sizeof(Symbol));
    symbol->kind = kind;
    symbol->body = body;
    symbol->operation = NULL;

    // update list of symbols
    add_list_entry_tail(Symbol)(symbol_list, symbol);

    return symbol;
}


/*
make a new operation
*/
static Operation *new_operation(MnemonicKind kind, const List(Operand) *operands)
{
    Operation *operation = calloc(1, sizeof(Operation));
    operation->kind = kind;
    operation->operands = operands;

    // update list of operations
    add_list_entry_tail(Operation)(operation_list, operation);

    return operation;
}


/*
make a new operand
*/
static Operand *new_operand(OperandKind kind)
{
    Operand *operand = calloc(1, sizeof(Operand));
    operand->kind = kind;
    operand->immediate = 0;

    return operand;
}


/*
make a new operand for immediate
*/
static Operand *new_operand_immediate(uint32_t immediate)
{
    OperandKind kind;
    switch(get_least_size(immediate))
    {
    case SIZEOF_8BIT:
        kind = OP_IMM8;
        break;

    case SIZEOF_16BIT:
        kind = OP_IMM8;
        break;

    case SIZEOF_32BIT:
    default:
        kind = OP_IMM32;
        break;
    }

    Operand *operand = new_operand(kind);
    operand->immediate = immediate;

    return operand;
}


/*
make a new operand for register
*/
static Operand *new_operand_register(const Token *token)
{
    const RegisterInfo *info = get_register_info(token);
    Operand *operand = new_operand(info->op_kind);
    operand->reg = info->reg_kind;
    return operand;
}


/*
make a new operand for memory
*/
static Operand *new_operand_memory(OperandKind kind)
{
    Operand *operand = new_operand(kind);

    expect_reserved("[");
    Token *token = expect_register();
    operand->reg = get_register_info(token)->reg_kind;
    if(consume_reserved("+"))
    {
        if(consume_token(TK_SYMBOL, &token))
        {
            operand->label = make_symbol(token);
        }
        else
        {
            operand->immediate = expect_immediate()->value;
        }
    }
    else if(consume_reserved("-"))
    {
        operand->immediate = -expect_immediate()->value;
    }
    expect_reserved("]");

    return operand;
}


/*
make a new operand for symbol
*/
static Operand *new_operand_symbol(const Token *token)
{
    Operand *operand = new_operand(OP_SYMBOL);
    operand->label = make_symbol(token);

    return operand;
}


/*
get the least number of bytes to represent an immediate value
*/
size_t get_least_size(uint32_t value)
{
    if(value == 0)
    {
        return 0;
    }
    else if((value < UINT8_MAX) || (-value < UINT8_MAX))
    {
        return SIZEOF_8BIT;
    }
    else if((value < UINT16_MAX) || (-value < UINT16_MAX))
    {
        return SIZEOF_16BIT;
    }
    else
    {
        return SIZEOF_32BIT;
    }
}


/*
get register information by name
*/
static const RegisterInfo *get_register_info(const Token *token)
{
    for(size_t i = 0; i < REGISTER_INFO_LIST_SIZE; i++)
    {
        const RegisterInfo *info = &register_info_list[i];
        if(strncmp(token->str, info->name, token->len) == 0)
        {
            return info;
        }
    }

    return NULL;
}


/*
consume a size specifier
```
size-specifier ::= "qword ptr"
```
*/
static bool consume_size_specifier(OperandKind *kind)
{
    bool consumed = true;

    if(consume_reserved("dword ptr"))
    {
        *kind = OP_M32;
    }
    else if(consume_reserved("qword ptr"))
    {
        *kind = OP_M64;
    }
    else
    {
        consumed = false;
    }

    return consumed;
}
