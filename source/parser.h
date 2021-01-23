#ifndef __PARSER_H__
#define __PARSER_H__

#include <stdint.h>
#include <stddef.h>

#include "elf_wrap.h"
#include "identifier.h"
#include "processor.h"
#include "section.h"

typedef enum LabelKind LabelKind;
typedef enum StatementKind StatementKind;
typedef struct Label Label;
typedef struct Statement Statement;
typedef struct Program Program;

#include "list.h"
define_list(Label)
define_list(Statement)

// kind of label
enum LabelKind
{
    LB_GLOBAL, // global label
    LB_LOCAL,  // local label
};

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
    LabelKind kind;             // kind of label
    const char *body;           // contents of label
    const Statement *statement; // statement marked by the label
};

// structure for statement
struct Statement
{
    StatementKind kind;       // kind of statement
    SectionKind section;      // section of statement
    Elf_Addr address;         // address of statement
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
