#ifndef __GENERATOR_H__
#define __GENERATOR_H__

#include "elf_wrap.h"
#include "parser.h"

typedef struct LabelInfo LabelInfo;

LabelInfo *new_label_info(const char *body, Elf_Addr address, Elf_Sxword addend);
void generate(const char *output_file, const Program *program);

#endif /* !__GENERATOR_H__ */
