#ifndef __PARSER_H__
#define __PARSER_H__

#include <stdint.h>
#include <stddef.h>

#include "elf_wrap.h"
#include "identifier.h"
#include "processor.h"
#include "section.h"

typedef struct Directive Directive;
typedef struct Program Program;

// structure for directive
struct Directive
{
    const Label *symbol; // symbol associated with the directive
};

// structure for program
struct Program
{
    List(Operation) *operations; // list of operations
    List(Data) *data_list;       // list of data
    List(Bss) *bss_list;         // list of bss
    List(Label) *symbols;        // list of symbols
};

void construct(Program *prog);
size_t get_least_size(uintmax_t value);

#endif /* !__PARSER_H__ */
