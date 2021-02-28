#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include <stddef.h>

#include "elf_wrap.h"
#include "processor.h"
#include "section.h"
#include "symbol.h"

typedef enum StatementKind StatementKind;
typedef struct Label Label;
typedef struct Program Program;
typedef struct Statement Statement;

#include "list.h"
define_list(Label)
define_list(Statement)

// kind of statement
enum StatementKind
{
    ST_INSTRUCTION, // instruction
    ST_VALUE,       // .byte, .word, .dword, .qword directive
    ST_ZERO,        // .zero directive
};

// structure for label
struct Label
{
    const Symbol *symbol;       // contents of label
    const Statement *statement; // statement marked by the label
};

// structure for program
struct Program
{
    List(Statement) *statement_list; // list of statements
    List(Label) *label_list;         // list of labels
    List(Symbol) *symbol_list;       // list of symbols
};

// structure for statement
struct Statement
{
    StatementKind kind;       // kind of statement
    SectionKind section;      // section of statement
    Elf_Addr address;         // address of statement
    Elf_Xword alignment;      // alignment of statement
    union
    {
        Operation *operation; // instruction
        Data *data;           // .byte, .word, .dword, .qword directive
        Bss *bss;             // .zero directive
    };
};

void construct(Program *prog);
Label *search_label(const List(Label) *label_list, const Symbol *symbol);

#endif /* !PARSER_H */
