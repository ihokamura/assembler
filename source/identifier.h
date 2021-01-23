#ifndef __IDENTIFIER_H__
#define __IDENTIFIER_H__

#include "elf_wrap.h"

typedef struct Symbol Symbol;

Symbol *new_symbol(const char *body, Elf_Addr address, Elf_Sxword addend);

#endif /* !__IDENTIFIER_H__ */
