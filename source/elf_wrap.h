#ifndef __ELF_WRAP_H__
#define __ELF_WRAP_H__

#include <elf.h>

#define ELF_CLASS          ELFCLASS64
#define ELF_DATA           ELFDATA2LSB
#define ELF_R_SYM          ELF64_R_SYM
#define ELF_R_TYPE         ELF64_R_TYPE
#define ELF_ST_BIND        ELF64_ST_BIND
#define ELF_ST_TYPE        ELF64_ST_TYPE
#define ELF_ST_VISIBILITY  ELF64_ST_VISIBILITY
#define EM_MACHINE         EM_X86_64

#define ELF_ST_INFO        ELF64_ST_INFO

typedef Elf64_Addr    Elf_Addr;
typedef Elf64_Ehdr    Elf_Ehdr;
typedef Elf64_Half    Elf_Half;
typedef Elf64_Off     Elf_Off;
typedef Elf64_Phdr    Elf_Phdr;
typedef Elf64_Rel     Elf_Rel;
typedef Elf64_Rela    Elf_Rela;
typedef Elf64_Section Elf_Section;
typedef Elf64_Shdr    Elf_Shdr;
typedef Elf64_Sxword  Elf_Size;
typedef Elf64_Sym     Elf_Sym;
typedef Elf64_Sxword  Elf_Sxword;
typedef Elf64_Xword   Elf_Xword;
typedef Elf64_Word    Elf_Word;

#endif /* !__ELF_WRAP_H__ */
