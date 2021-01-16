#ifndef __PARSER_H__
#define __PARSER_H__

#include <stdint.h>
#include <stddef.h>

#include "elf_wrap.h"
#include "processor.h"

typedef enum SectionKind SectionKind;
typedef enum SymbolKind SymbolKind;
typedef struct Directive Directive;
typedef struct Program Program;
typedef struct Symbol Symbol;

#include "list.h"
define_list(Symbol)

// kind of section
enum SectionKind
{
    SC_UND,  // undefined section
    SC_TEXT, // text section
    SC_DATA, // data section
    SC_BSS,  // bss section
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

// structure for program
struct Program
{
    List(Operation) *operations; // list of operations
    List(Data) *data_list;       // list of data
    List(Bss) *bss_list;         // list of bss
    List(Symbol) *symbols;       // list of symbols
};

// structure for symbol
struct Symbol
{
    SymbolKind kind;            // kind of symbol
    SectionKind section;        // section of symbol
    const char *body;           // contents of symbol
    const Operation *operation; // operation labeled by symbol
    const Data *data;           // data labeled by symbol
    const Bss *bss;             // bss labeled by symbol
};

void construct(Program *prog);
size_t get_least_size(uintmax_t value);

#endif /* !__PARSER_H__ */
