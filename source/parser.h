#ifndef __PARSER_H__
#define __PARSER_H__

#include "elf_wrap.h"
#include "processor.h"

typedef enum SymbolKind SymbolKind;
typedef struct Directive Directive;
typedef struct Program Program;
typedef struct Symbol Symbol;

#include "list.h"
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
