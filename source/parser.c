#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "tokenizer.h"

// definition of list operations
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
static OperationKind mnemonic(const Token *token);
static List(Operand) *operands(void);
static Operand *operand(void);
static Directive *new_directive(const Symbol *symbol);
static Symbol *new_symbol(SymbolKind kind, const char *body);
static Operation *new_operation(OperationKind kind, const List(Operand) *operands);
static Operand *new_operand(OperandKind kind);
static Operand *new_operand_immediate(long immediate);
static Operand *new_operand_register(const char *reg);

// global variable
static List(Operation) *operation_list = NULL; // list of operations
static List(Symbol) *symbol_list = NULL; // list of symbols
// mapping from mnemonic string to operation kind
static struct {const char *mnemonic; OperationKind kind;} mnemonic_map[] = 
{
    {"mov", OP_MOV},
    {"ret", OP_RET},
};
static size_t MNIMONIC_MAP_SIZE = sizeof(mnemonic_map) / sizeof(mnemonic_map[0]);


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
statement ::= directive
            | symbol ":"
            | operation
```
*/
static void statement(void)
{
    Token *token;
    if(consume_reserved("."))
    {
        directive();
    }
    else if(consume_token(TK_SYMBOL, &token))
    {
        symbol(token);
        expect_reserved(":");
    }
    else if(consume_token(TK_MNEMONIC, &token))
    {
        operation(token);
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
        return new_directive(symbol(expect_symbol()));
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

    return new_symbol(SY_GLOBAL, body);
}


/*
parse an operation
```
operation ::= mnemonic operands?
```
*/
static Operation *operation(const Token *token)
{
    OperationKind kind = mnemonic(token);

    switch(kind)
    {
    case OP_MOV:
        return new_operation(kind, operands());

    case OP_RET:
    default:
        return new_operation(kind, NULL);
    }
}


/*
parse a mnemonic
```
mnemonic ::= "mov"
           | "ret"
```
*/
static OperationKind mnemonic(const Token *token)
{
    for(size_t i = 0; i < MNIMONIC_MAP_SIZE; i++)
    {
        if(strncmp(token->str, mnemonic_map[i].mnemonic, token->len) == 0)
        {
            return mnemonic_map[i].kind;
        }
    }

    report_error(NULL, "invalid mnemonic '%s.", make_symbol(token));

    return OP_NOP;
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
operand ::= immediate | register
```
*/
static Operand *operand(void)
{
    Token *token;
    if(consume_token(TK_IMMEDIATE, &token))
    {
        return new_operand_immediate(token->value);
    }
    else
    {
        consume_token(TK_REGISTER, &token);
        char *reg = calloc(token->len + 1, sizeof(char));
        strncpy(reg, token->str, token->len);

        return new_operand_register(reg);
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

    // update list of symbols
    add_list_entry_tail(Symbol)(symbol_list, symbol);

    return symbol;
}


/*
make a new operation
*/
static Operation *new_operation(OperationKind kind, const List(Operand) *operands)
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
    Operand *operand = new_operand(OP_IMMEDIATE);
    operand->immediate = immediate;

    return operand;
}


/*
make a new operand for register
*/
static Operand *new_operand_register(const char *reg)
{
    Operand *operand = new_operand(OP_REGISTER);
    operand->reg = reg;

    return operand;
}
