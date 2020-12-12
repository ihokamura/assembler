#ifndef __PARSER_H__
#define __PARSER_H__

#include "list.h"
typedef enum OperandKind OperandKind;
typedef enum OperationKind OperationKind;
typedef enum SymbolKind SymbolKind;
typedef struct Directive Directive;
typedef struct Operand Operand;
typedef struct Operation Operation;
typedef struct Program Program;
typedef struct Symbol Symbol;
define_list(Operand)
define_list(Operation)
define_list(Symbol)

// kind of operand
enum OperandKind
{
    OP_IMMEDIATE, // immediate
    OP_REGISTER,  // register
};

// kind of mnemonic
enum OperationKind
{
    OP_NOP, // nop
    OP_MOV, // mov
    OP_RET, // ret
};

// kind of symbol
enum SymbolKind
{
    SY_GLOBAL, // global symbol
    SY_LOCAL,  // local symbol
};

// structure for directive
struct Directive
{
    const Symbol *symbol; // symbol associated with the directive
};

// structure for operand
struct Operand
{
    OperandKind kind;    // kind of operand
    union
    {
        long immediate;  // immediate value
        const char *reg; // register name
    };
};

// structure for operation
struct Operation
{
    OperationKind kind;            // kind of operation
    const List(Operand) *operands; // list of operands
};

// structure for program
struct Program
{
    List(Operation) *operations; // list of operations
    List(Symbol) *symbols;       // list of symbols
};

// structure for symbol
struct Symbol
{
    SymbolKind kind;  // kind of symbol
    const char *body; // contents of symbol
};

void construct(Program *prog);

#endif /* !__PARSER_H__ */
