#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "identifier.h"
#include "processor.h"
#include "tokenizer.h"

#include "list.h"
define_list_operations(Bss)
define_list_operations(Data)
define_list_operations(Statement)
define_list_operations(Operand)
define_list_operations(Operation)
define_list_operations(Label)

// function prototype
static void program(void);
static void statement(void);
static void parse_directive(Label *label);
static void parse_directive_size(size_t size, Label *label);
static void parse_directive_zero(Label *label);
static Label *parse_label(const Token *token);
static Operation *parse_operation(const Token *token);
static const MnemonicInfo *parse_mnemonic(const Token *token);
static List(Operand) *parse_operands(void);
static Operand *parse_operand(void);
static Statement *new_statement(StatementKind kind);
static Label *new_label(LabelKind kind, const char *body);
static Bss *new_bss(size_t size);
static Data *new_data(size_t size, uintmax_t value);
static Operation *new_operation(MnemonicKind kind, const List(Operand) *operands);
static Operand *new_operand(OperandKind kind);
static Operand *new_operand_immediate(uintmax_t immediate);
static Operand *new_operand_register(const Token *token);
static Operand *new_operand_memory(OperandKind kind);
static Operand *new_operand_symbol(const Token *token);
static const RegisterInfo *get_register_info(const Token *token);
static bool consume_size_specifier(OperandKind *kind);

// global variable
static List(Statement) *statement_list = NULL; // list of statements
static List(Label) *label_list = NULL; // list of labels

static SectionKind current_section = SC_UND;


/*
construct program
*/
void construct(Program *prog)
{
    statement_list = new_list(Statement)();
    label_list = new_list(Label)();

    program();
    prog->statement_list = statement_list;
    prog->label_list = label_list;
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
statement ::= (label ":")? directive | operation
```
*/
static void statement(void)
{
    Token *token;
    Label *label;
    if(consume_token(TK_IDENTIFIER, &token))
    {
        label = parse_label(token);
        expect_reserved(":");
    }

    if(consume_reserved("."))
    {
        parse_directive(label);
    }
    else if(consume_token(TK_MNEMONIC, &token))
    {
        Operation *op = parse_operation(token);
        label->operation = op;
    }
}


/*
parse a directive
```
directive ::= ".bss"
            | ".byte"
            | ".data"
            | ".globl" symbol
            | ".intel_syntax noprefix"
            | ".long"
            | ".quad"
            | ".text"
            | ".word"
            | ".zero"
```
*/
static void parse_directive(Label *label)
{
    if(consume_reserved("bss"))
    {
        current_section = SC_BSS;
    }
    else if(consume_reserved("byte"))
    {
        parse_directive_size(SIZEOF_8BIT, label);
    }
    else if(consume_reserved("data"))
    {
        current_section = SC_DATA;
    }
    else if(consume_reserved("globl"))
    {
        Label *label = parse_label(expect_identifier());
        label->kind = LB_GLOBAL; // overwrite the kind
    }
    else if(consume_reserved("intel_syntax noprefix"))
    {
        // do nothing
    }
    else if(consume_reserved("long"))
    {
        parse_directive_size(SIZEOF_32BIT, label);
    }
    else if(consume_reserved("quad"))
    {
        parse_directive_size(SIZEOF_64BIT, label);
    }
    else if(consume_reserved("text"))
    {
        current_section = SC_TEXT;
    }
    else if(consume_reserved("word"))
    {
        parse_directive_size(SIZEOF_16BIT, label);
    }
    else if(consume_reserved("zero"))
    {
        parse_directive_zero(label);
    }
    else
    {
        report_error(NULL, "expected directive.");
    }
}


/*
parse directive for size
*/
static void parse_directive_size(size_t size, Label *label)
{
    label->data = new_data(size, expect_immediate()->value);
}


/*
parse directive for zero
*/
static void parse_directive_zero(Label *label)
{
    label->bss = new_bss(expect_immediate()->value);
}


/*
parse a label
*/
static Label *parse_label(const Token *token)
{
    const char *body = make_identifier(token);

    // serch the existing labels
    for_each_entry(Label, cursor, label_list)
    {
        Label *label = get_element(Label)(cursor);
        if(strcmp(body, label->body) == 0)
        {
            return label;
        }
    }

    return new_label(LB_LOCAL, body);
}


/*
parse an operation
```
operation ::= mnemonic operands?
```
*/
static Operation *parse_operation(const Token *token)
{
    const MnemonicInfo *map = parse_mnemonic(token);

    return new_operation(map->kind, map->take_operands ? parse_operands() : NULL);
}


/*
parse a mnemonic
*/
static const MnemonicInfo *parse_mnemonic(const Token *token)
{
    for(size_t i = 0; i < MNEMONIC_INFO_LIST_SIZE; i++)
    {
        if(strncmp(token->str, mnemonic_info_list[i].name, token->len) == 0)
        {
            return &mnemonic_info_list[i];
        }
    }

    report_error(NULL, "invalid mnemonic '%s.", make_identifier(token));

    return &mnemonic_info_list[MN_NOP];
}


/*
parse operands
```
operands ::= operand ("," operand)?
```
*/
static List(Operand) *parse_operands(void)
{
    List(Operand) *operand_list = new_list(Operand)();
    add_list_entry_tail(Operand)(operand_list, parse_operand());
    while(consume_reserved(","))
    {
        add_list_entry_tail(Operand)(operand_list, parse_operand());
    }

    return operand_list;
}


/*
parse an operand
```
operand ::= immediate | register | memory | symbol
```
*/
static Operand *parse_operand(void)
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
    else if(consume_token(TK_IDENTIFIER, &token))
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
make a new statement
*/
static Statement *new_statement(StatementKind kind)
{
    Statement *statement = calloc(1, sizeof(Statement));
    statement->kind = kind;

    // update list of statements
    add_list_entry_tail(Statement)(statement_list, statement);

    return statement;
}


/*
make a new label
*/
static Label *new_label(LabelKind kind, const char *body)
{
    Label *lab = calloc(1, sizeof(Label));
    lab->kind = kind;
    lab->section = current_section;
    lab->body = body;
    lab->operation = NULL;
    lab->data = NULL;

    // update list of symbols
    add_list_entry_tail(Label)(label_list, lab);

    return lab;
}


/*
make a new bss
*/
static Bss *new_bss(size_t size)
{
    Bss *bss = calloc(1, sizeof(Bss));
    bss->size = size;

    Statement *statement = new_statement(ST_ZERO);
    statement->bss = bss;

    return bss;
}


/*
make a new data
*/
static Data *new_data(size_t size, uintmax_t value)
{
    Data *data = calloc(1, sizeof(Data));
    data->size = size;
    data->value = value;

    Statement *statement = new_statement(ST_VALUE);
    statement->data = data;

    return data;
}


/*
make a new operation
*/
static Operation *new_operation(MnemonicKind kind, const List(Operand) *operands)
{
    Operation *operation = calloc(1, sizeof(Operation));
    operation->kind = kind;
    operation->operands = operands;

    Statement *statement = new_statement(ST_INSTRUCTION);
    statement->operation = operation;

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
static Operand *new_operand_immediate(uintmax_t immediate)
{
    OperandKind kind;
    switch(get_least_size(immediate))
    {
    case 0:
    case SIZEOF_8BIT:
        kind = OP_IMM8;
        break;

    case SIZEOF_16BIT:
        kind = OP_IMM16;
        break;

    case SIZEOF_32BIT:
        kind = OP_IMM32;
        break;

    case SIZEOF_64BIT:
    default:
        kind = OP_IMM64;
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
    while(true)
    {
        if(consume_reserved("+"))
        {
            if(consume_token(TK_IDENTIFIER, &token))
            {
                operand->symbol = make_identifier(token);
            }
            else
            {
                operand->immediate += expect_immediate()->value;
            }
        }
        else if(consume_reserved("-"))
        {
            operand->immediate -= expect_immediate()->value;
        }
        else
        {
            break;
        }
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
    operand->symbol = make_identifier(token);

    return operand;
}


/*
get the least number of bytes to represent an immediate value
*/
size_t get_least_size(uintmax_t value)
{
    if(value == 0)
    {
        return 0;
    }
    else if((value <= UINT8_MAX) || (-value <= UINT8_MAX))
    {
        return SIZEOF_8BIT;
    }
    else if((value <= UINT16_MAX) || (-value <= UINT16_MAX))
    {
        return SIZEOF_16BIT;
    }
    else if((value <= UINT32_MAX) || (-value <= UINT32_MAX))
    {
        return SIZEOF_32BIT;
    }
    else
    {
        return SIZEOF_64BIT;
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
        if((token->len == strlen(info->name)) && (strncmp(token->str, info->name, token->len) == 0))
        {
            return info;
        }
    }

    return NULL;
}


/*
consume a size specifier
```
size-specifier ::= "byte ptr" | "word ptr" | "dword ptr" | "qword ptr"
```
*/
static bool consume_size_specifier(OperandKind *kind)
{
    bool consumed = true;

    if(consume_reserved("byte ptr"))
    {
        *kind = OP_M8;
    }
    else if(consume_reserved("word ptr"))
    {
        *kind = OP_M16;
    }
    else if(consume_reserved("dword ptr"))
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
