#ifndef __PARSER_H__
#define __PARSER_H__

#include <stdint.h>
#include <stddef.h>

#include "elf_wrap.h"
#include "identifier.h"
#include "processor.h"
#include "section.h"

typedef enum StatementKind StatementKind;
typedef struct Statement Statement;
typedef struct Program Program;

#include "list.h"
define_list(Statement)

// kind of statement
enum StatementKind
{
    ST_INSTRUCTION, // instruction
    ST_VALUE,       // .byte, .word, .dword, .qword directive
    ST_ZERO,        // .zero directive
};

// structure for statement
struct Statement
{
    StatementKind kind;       // kind of statement
    union
    {
        Operation *operation; // instruction
        Data *data;           // .byte, .word, .dword, .qword directive
        Bss *bss;             // .zero directive
    };
};

// structure for program
struct Program
{
    List(Statement) *statement_list; // list of statements
    List(Label) *label_list;         // list of labels
};

void construct(Program *prog);
size_t get_least_size(uintmax_t value);

#endif /* !__PARSER_H__ */
