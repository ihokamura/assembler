#ifndef __PARSER_H__
#define __PARSER_H__

#include "elf_wrap.h"
#include "processor.h"

typedef enum SymbolKind SymbolKind;
typedef struct Directive Directive;
typedef struct Operand Operand;
typedef struct Operation Operation;
typedef struct Program Program;
typedef struct Symbol Symbol;

#include "list.h"
define_list(Operand)
define_list(Operation)
define_list(Symbol)

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
        long immediate;    // immediate value
        RegisterKind reg;  // kind of register
        const char *label; // label
    };
};

// structure for operation
struct Operation
{
    MnemonicKind kind;             // kind of operation
    const List(Operand) *operands; // list of operands
    Elf_Addr address;              // address of operation
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
    SymbolKind kind;            // kind of symbol
    const char *body;           // contents of symbol
    const Operation *operation; // operation labeled by symbol
};

void construct(Program *prog);

#endif /* !__PARSER_H__ */
