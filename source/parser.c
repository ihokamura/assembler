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
define_list_operations(Operand)
define_list_operations(Operation)
define_list_operations(Label)

// function prototype
static void program(void);
static void statement(void);
static Directive *parse_directive(Label *label);
static Label *parse_label(const Token *token);
static Operation *parse_operation(const Token *token);
static const MnemonicInfo *parse_mnemonic(const Token *token);
static List(Operand) *parse_operands(void);
static Operand *parse_operand(void);
static Directive *new_directive(const Label *lab);
static Directive *new_directive_data(size_t size, Label *label);
static Directive *new_directive_zero(Label *label);
static Label *new_label(SymbolKind kind, const char *body);
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
static List(Operation) *operation_list = NULL; // list of operations
static List(Data) *data_list = NULL; // list of data
static List(Bss) *bss_list = NULL; // list of bss
static List(Label) *label_list = NULL; // list of labels

static SectionKind current_section = SC_UND;


/*
construct program
*/
void construct(Program *prog)
{
    operation_list = new_list(Operation)();
    data_list = new_list(Data)();
    bss_list = new_list(Bss)();
    label_list = new_list(Label)();

    program();
    prog->operations = operation_list;
    prog->data_list = data_list;
    prog->bss_list = bss_list;
    prog->symbols = label_list;
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
static Directive *parse_directive(Label *label)
{
    if(consume_reserved("bss"))
    {
        current_section = SC_BSS;
        return new_directive(NULL);
    }
    else if(consume_reserved("byte"))
    {
        return new_directive_data(SIZEOF_8BIT, label);
    }
    else if(consume_reserved("data"))
    {
        current_section = SC_DATA;
        return new_directive(NULL);
    }
    else if(consume_reserved("globl"))
    {
        Label *globl_symbol = parse_label(expect_identifier());
        globl_symbol->kind = SY_GLOBAL; // overwrite the kind
        return new_directive(globl_symbol);
    }
    else if(consume_reserved("intel_syntax noprefix"))
    {
        return new_directive(NULL);
    }
    else if(consume_reserved("long"))
    {
        return new_directive_data(SIZEOF_32BIT, label);
    }
    else if(consume_reserved("quad"))
    {
        return new_directive_data(SIZEOF_64BIT, label);
    }
    else if(consume_reserved("text"))
    {
        current_section = SC_TEXT;
        return new_directive(NULL);
    }
    else if(consume_reserved("word"))
    {
        return new_directive_data(SIZEOF_16BIT, label);
    }
    else if(consume_reserved("zero"))
    {
        return new_directive_zero(label);
    }
    else
    {
        report_error(NULL, "expected directive.");
        return NULL;
    }
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

    return new_label(SY_LOCAL, body);
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
make a new directive
*/
static Directive *new_directive(const Label *lab)
{
    Directive *directive = calloc(1, sizeof(Directive));
    directive->symbol = lab;

    return directive;
}


/*
make a new directive for data
*/
static Directive *new_directive_data(size_t size, Label *lab)
{
    lab->data = new_data(size, expect_immediate()->value);
    return new_directive(NULL);
}


/*
make a new directive for zero
*/
static Directive *new_directive_zero(Label *lab)
{
    lab->bss = new_bss(expect_immediate()->value);
    return new_directive(NULL);
}


/*
make a new label
*/
static Label *new_label(SymbolKind kind, const char *body)
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

    // update list of bss
    add_list_entry_tail(Bss)(bss_list, bss);

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

    // update list of data
    add_list_entry_tail(Data)(data_list, data);

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
    if(consume_reserved("+"))
    {
        if(consume_token(TK_IDENTIFIER, &token))
        {
            operand->symbol = make_identifier(token);
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
