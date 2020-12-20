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
static MnemonicKind mnemonic(const Token *token);
static List(Operand) *operands(void);
static Operand *operand(void);
static Directive *new_directive(const Symbol *symbol);
static Symbol *new_symbol(SymbolKind kind, const char *body);
static Operation *new_operation(MnemonicKind kind, const List(Operand) *operands);
static Operand *new_operand(OperandKind kind);
static Operand *new_operand_immediate(long immediate);
static Operand *new_operand_register(const char *name);
static Operand *new_operand_symbol(const Token *token);

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
    MnemonicKind kind = mnemonic(token);

    switch(kind)
    {
    case MN_CALL:
    case MN_MOV:
        return new_operation(kind, operands());

    case MN_NOP:
    case MN_RET:
    default:
        return new_operation(kind, NULL);
    }
}


/*
parse a mnemonic
```
mnemonic ::= "call"
           | "mov"
           | "nop"
           | "ret"
```
*/
static MnemonicKind mnemonic(const Token *token)
{
    for(size_t i = 0; i < MNEMONIC_MAP_SIZE; i++)
    {
        if(strncmp(token->str, mnemonic_maps[i].name, token->len) == 0)
        {
            return mnemonic_maps[i].kind;
        }
    }

    report_error(NULL, "invalid mnemonic '%s.", make_symbol(token));

    return MN_NOP;
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
operand ::= immediate | register | symbol
```
*/
static Operand *operand(void)
{
    Token *token;
    if(consume_token(TK_IMMEDIATE, &token))
    {
        return new_operand_immediate(token->value);
    }
    else if(consume_token(TK_REGISTER, &token))
    {
        return new_operand_register(make_symbol(token));
    }
    else if (consume_token(TK_SYMBOL, &token))
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

    return operand;
}


/*
make a new operand for immediate
*/
static Operand *new_operand_immediate(long immediate)
{
    Operand *operand = new_operand(OP_IMM32);
    operand->immediate = immediate;

    return operand;
}


/*
make a new operand for register
*/
static Operand *new_operand_register(const char *name)
{
    for(size_t i = 0; i < REGISTER_MAP_SIZE; i++)
    {
        RegisterMap map = register_maps[i];
        if(strcmp(name, map.name) == 0)
        {
            Operand *operand = new_operand(map.op_kind);
            operand->reg = map.reg_kind;
            return operand;
        }
    }

    report_error(NULL, "invalid register name '%s'.", name);
    return NULL;
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
