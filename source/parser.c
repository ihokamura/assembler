#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "processor.h"
#include "tokenizer.h"
#include "symbol.h"

#include "list.h"
define_list_operations(Label)
define_list_operations(Operand)
define_list_operations(Statement)

// function prototype
static void program(void);
static void statement(void);
static void parse_directive(Label *label);
static void parse_directive_size(size_t size, Label *label);
static void parse_directive_string(Label *label);
static void parse_directive_zero(Label *label);
static Label *parse_label(const Token *token);
static Operation *parse_operation(const Token *token, Label *label);
static const MnemonicInfo *parse_mnemonic(const Token *token);
static List(Operand) *parse_operands(void);
static Operand *parse_operand(void);
static Statement *new_statement(StatementKind kind, Label *label);
static Label *new_label(const Symbol *symbol);
static Bss *new_bss(size_t size, Label *label);
static Data *new_data(DataKind kind, size_t size, Label *label);
static Data *new_data_immediate(size_t size, uintmax_t value, Label *label);
static Data *new_data_symbol(size_t size, const Token *token, Label *label);
static Data *new_data_string(const char *str, Label *label, size_t *len);
static Operation *new_operation(MnemonicKind kind, const List(Operand) *operands, Label *label);
static Operand *new_operand(OperandKind kind);
static Operand *new_operand_immediate(uintmax_t immediate);
static Operand *new_operand_register(const Token *token);
static Operand *new_operand_memory(OperandKind kind);
static Operand *new_operand_symbol(const Token *token);
static const RegisterInfo *get_register_info(const Token *token);
static bool consume_size_specifier(OperandKind *kind);
static size_t get_current_alignment(void);
static void set_current_alignment(size_t alignment);
static void reset_current_alignment(void);

// global variable
static List(Statement) *statement_list = NULL; // list of statements
static List(Label) *label_list = NULL; // list of labels

static size_t current_alignment = 1; // current alignment


/*
construct program
*/
void construct(Program *prog)
{
    statement_list = new_list(Statement)();
    label_list = new_list(Label)();
    initialize_section();
    initialize_symbol_list();

    program();
    prog->statement_list = statement_list;
    prog->label_list = label_list;
    prog->symbol_list = get_symbol_list();
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
    Label *label = NULL;
    if(consume_token(TK_IDENTIFIER, &token))
    {
        label = parse_label(token);
        expect_reserved(":");
    }

    if(consume_token(TK_MNEMONIC, &token))
    {
        parse_operation(token, label);
    }
    else
    {
        parse_directive(label);
    }
}


/*
parse a directive
```
directive ::= ".align"
            | ".bss"
            | ".byte"
            | ".data"
            | ".globl" symbol
            | ".intel_syntax noprefix"
            | ".long"
            | ".quad"
            | ".string"
            | ".text"
            | ".value"
            | ".word"
            | ".zero"
```
*/
static void parse_directive(Label *label)
{
    if(consume_reserved(".align"))
    {
        set_current_alignment(expect_token(TK_IMMEDIATE)->value);
    }
    else if(consume_reserved(".bss"))
    {
        reset_current_alignment();
        set_current_section(".bss");
    }
    else if(consume_reserved(".byte"))
    {
        parse_directive_size(SIZEOF_8BIT, label);
    }
    else if(consume_reserved(".data"))
    {
        reset_current_alignment();
        set_current_section(".data");
    }
    else if(consume_reserved(".globl"))
    {
        Token *token = expect_token(TK_IDENTIFIER);
        Symbol *symbol = new_symbol(token);
        symbol->bind = STB_GLOBAL;
        symbol->declared = true;
    }
    else if(consume_reserved(".intel_syntax noprefix"))
    {
        // do nothing
    }
    else if(consume_reserved(".long"))
    {
        parse_directive_size(SIZEOF_32BIT, label);
    }
    else if(consume_reserved(".quad"))
    {
        parse_directive_size(SIZEOF_64BIT, label);
    }
    else if(consume_reserved(".string"))
    {
        parse_directive_string(label);
    }
    else if(consume_reserved(".text"))
    {
        reset_current_alignment();
        set_current_section(".text");
    }
    else if(consume_reserved(".value") || consume_reserved(".word"))
    {
        parse_directive_size(SIZEOF_16BIT, label);
    }
    else if(consume_reserved(".zero"))
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
    Token *token;
    if(consume_token(TK_IDENTIFIER, &token))
    {
        new_data_symbol(size, token, label);
    }
    else
    {
        new_data_immediate(size, expect_token(TK_IMMEDIATE)->value, label);
    }
}


/*
parse directive for string
*/
static void parse_directive_string(Label *label)
{
    // save the current alignment
    Elf_Xword saved_alignment = get_current_alignment();
    reset_current_alignment();

    // make data of string body
    Token *token = expect_token(TK_STRING);
    size_t len = 0;
    new_data_string(token->str, label, &len);
    size_t bytes = 1;
    while(len < token->len)
    {
        new_data_string(token->str, NULL, &len);
        bytes++;
    }

    // fill paddings to adjust alignment
    for(size_t end = align_to(bytes, saved_alignment); bytes < end; bytes++)
    {
        new_data_immediate(SIZEOF_8BIT, 0x00, NULL);
    }

    // restore the saved alignment
    set_current_alignment(saved_alignment);
}


/*
parse directive for zero
*/
static void parse_directive_zero(Label *label)
{
    new_bss(expect_token(TK_IMMEDIATE)->value, label);
}


/*
parse a label
*/
static Label *parse_label(const Token *token)
{
    Symbol *symbol = new_symbol(token);
    symbol->located = get_current_section();
    symbol->labeled = true;

    if(search_label(label_list, symbol) != NULL)
    {
        report_error(NULL, "duplicated label '%s'", symbol->body);
    }

    return new_label(symbol);
}


/*
parse an operation
```
operation ::= mnemonic operands?
```
*/
static Operation *parse_operation(const Token *token, Label *label)
{
    const MnemonicInfo *map = parse_mnemonic(token);

    return new_operation(map->kind, map->take_operands ? parse_operands() : NULL, label);
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
static Statement *new_statement(StatementKind kind, Label *label)
{
    Statement *statement = calloc(1, sizeof(Statement));
    statement->kind = kind;
    statement->section = get_current_section();
    statement->alignment = current_alignment;

    // update list of statements
    add_list_entry_tail(Statement)(statement_list, statement);

    // associate statement with label
    if(label != NULL)
    {
        label->statement = statement;
    }

    return statement;
}


/*
make a new label
*/
static Label *new_label(const Symbol *symbol)
{
    Label *label = calloc(1, sizeof(Label));
    label->symbol = symbol;
    label->statement = NULL;

    // update list of symbols
    add_list_entry_tail(Label)(label_list, label);

    return label;
}


/*
make a new bss
*/
static Bss *new_bss(size_t size, Label *label)
{
    Bss *bss = calloc(1, sizeof(Bss));
    bss->size = size;

    Statement *statement = new_statement(ST_ZERO, label);
    statement->bss = bss;

    return bss;
}


/*
make a new data
*/
static Data *new_data(DataKind kind, size_t size, Label *label)
{
    Data *data = calloc(1, sizeof(Data));
    data->kind = kind;
    data->size = size;
    data->value = 0;
    data->symbol = NULL;

    Statement *statement = new_statement(ST_VALUE, label);
    statement->data = data;

    return data;
}


/*
make a new data for immediate
*/
static Data *new_data_immediate(size_t size, uintmax_t value, Label *label)
{
    Data *data = new_data(DT_IMMEDIATE, size, label);
    data->value = value;

    return data;
}


/*
make a new data for memory
*/
static Data *new_data_symbol(size_t size, const Token *token, Label *label)
{
    Data *data = new_data(DT_SYMBOL, size, label);
    data->symbol = new_symbol(token);

    return data;
}


/*
make a new data for a character in string
*/
static Data *new_data_string(const char *str, Label *label, size_t *len)
{
    int value;
    const char *pos = &str[*len];
    *len += convert_escape_sequence(pos, &value);

    return new_data_immediate(SIZEOF_8BIT, value, label);
}


/*
make a new operation
*/
static Operation *new_operation(MnemonicKind kind, const List(Operand) *operands, Label *label)
{
    Operation *operation = calloc(1, sizeof(Operation));
    operation->kind = kind;
    operation->operands = operands;

    Statement *statement = new_statement(ST_INSTRUCTION, label);
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
    Token *token = expect_token(TK_REGISTER);
    operand->reg = get_register_info(token)->reg_kind;
    while(true)
    {
        if(consume_reserved("+"))
        {
            if(consume_token(TK_IDENTIFIER, &token))
            {
                operand->symbol = new_symbol(token);
            }
            else
            {
                operand->immediate += expect_token(TK_IMMEDIATE)->value;
            }
        }
        else if(consume_reserved("-"))
        {
            operand->immediate -= expect_token(TK_IMMEDIATE)->value;
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
    operand->symbol = new_symbol(token);

    return operand;
}


/*
search label by name
*/
Label *search_label(const List(Label) *label_list, const Symbol *symbol)
{
    for_each_entry(Label, cursor, label_list)
    {
        Label *label = get_element(Label)(cursor);
        if(strcmp(label->symbol->body, symbol->body) == 0)
        {
            return label;
        }
    }

    return NULL;
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


/*
get current alignment
*/
static size_t get_current_alignment(void)
{
    return current_alignment;
}


/*
set current alignment
*/
static void set_current_alignment(size_t alignment)
{
    current_alignment = alignment;

    // update alignment of the current section
    Section *section = get_section(get_current_section());
    if(section->alignment < alignment)
    {
        section->alignment = alignment;
    }
}


/*
reset current alignment
*/
static void reset_current_alignment(void)
{
    current_alignment = 1;
}
