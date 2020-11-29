#include <stdio.h>
#include <string.h>

#include "elf_wrap.h"
#include "generate.h"

#define STRING_MAX_SIZE          (1024)
#define SECTION_BODY_MAX_SIZE    (1024)

typedef struct
{
    Elf_Shdr shdr;
    char body[SECTION_BODY_MAX_SIZE];
} SectionContent;

static Elf_Off e_shoff = sizeof(Elf_Ehdr);  // offset of section header table
static Elf_Half e_shnum = 0;                // number of section header table entries
static Elf_Half e_shstrndx = 0;             // index of section ".shstrtab"

static char strtab_body[STRING_MAX_SIZE];   // body of string containing names of symbols
static char shstrtab_body[STRING_MAX_SIZE]; // body of string containing names of sections
static Elf_Word sh_name = 0;                // index of string where a section name starts


/*
set members of an ELF header
*/
static void set_elf_header
(
    Elf_Off e_shoff,
    Elf_Half e_shnum,
    Elf_Half e_shstrndx,
    Elf_Ehdr *ehdr
)
{
    ehdr->e_ident[EI_MAG0]       = ELFMAG0;
    ehdr->e_ident[EI_MAG1]       = ELFMAG1;
    ehdr->e_ident[EI_MAG2]       = ELFMAG2;
    ehdr->e_ident[EI_MAG3]       = ELFMAG3;
    ehdr->e_ident[EI_CLASS]      = ELF_CLASS;
    ehdr->e_ident[EI_DATA]       = ELF_DATA;
    ehdr->e_ident[EI_VERSION]    = EV_CURRENT;
    ehdr->e_ident[EI_OSABI]      = ELFOSABI_NONE;
    ehdr->e_ident[EI_ABIVERSION] = 0;

    ehdr->e_type    = ET_REL;
    ehdr->e_machine = EM_MACHINE;
    ehdr->e_version = EV_CURRENT;

    ehdr->e_entry     = 0;
    ehdr->e_phoff     = 0;
    ehdr->e_shoff     = e_shoff;
    ehdr->e_flags     = 0;
    ehdr->e_ehsize    = sizeof(Elf_Ehdr);
    ehdr->e_phentsize = 0;
    ehdr->e_phnum     = 0;
    ehdr->e_shentsize = sizeof(Elf_Shdr);
    ehdr->e_shnum     = e_shnum;
    ehdr->e_shstrndx  = e_shstrndx;
}


/*
set members of a section header table entry
*/
static void set_section_header_table
(
    const char *section_name,
    Elf_Word sh_type,
    Elf_Xword sh_flags,
    Elf_Addr sh_addr,
    Elf_Off sh_offset,
    Elf_Xword sh_size,
    Elf_Word sh_link,
    Elf_Word sh_info,
    Elf_Xword sh_addralign,
    Elf_Xword sh_entsize,
    Elf_Shdr *shdr
)
{
    // set members
    shdr->sh_name = sh_name;
    shdr->sh_type = sh_type;
    shdr->sh_flags = sh_flags;
    shdr->sh_addr = sh_addr;
    shdr->sh_offset = sh_offset;
    shdr->sh_size = sh_size;
    shdr->sh_link = sh_link;
    shdr->sh_info = sh_info;
    shdr->sh_addralign = sh_addralign;
    shdr->sh_entsize = sh_entsize;

    // update information on section header table
    e_shoff += sh_size;
    e_shnum++;
    e_shstrndx++;

    // update list of section names
    size_t size = strlen(section_name) + 1;
    memcpy(&shstrtab_body[sh_name], section_name, size);
    sh_name += size;
}


/*
set members of a symbol table entry
*/
static void set_symbol_table
(
    Elf_Word st_name,
    unsigned char st_info,
    unsigned char st_other,
    Elf_Section st_shndx,
    Elf_Addr st_value,
    Elf_Xword st_size,
    Elf_Sym *sym
)
{
    sym->st_name  = st_name;
    sym->st_info  = st_info;
    sym->st_other  = st_other;
    sym->st_shndx  = st_shndx;
    sym->st_value  = st_value;
    sym->st_size  = st_size;
}


/*
generate an object file
*/
void generate(const char *file_name)
{
   // NULL section
    SectionContent shdr_null;
    set_section_header_table(
        "",
        SHT_NULL,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        &shdr_null.shdr);

    // .text section
    SectionContent shdr_text;
    {
    size_t sh_size = 8;
    set_section_header_table(
        ".text",
        SHT_PROGBITS,
        SHF_ALLOC | SHF_EXECINSTR,
        0,
        e_shoff,
        sh_size,
        0,
        0,
        1,
        0,
        &shdr_text.shdr);
    shdr_text.body[0] = 0x48;
    shdr_text.body[1] = 0xc7;
    shdr_text.body[2] = 0xc0;
    shdr_text.body[3] = 0x2a;
    shdr_text.body[4] = 0x00;
    shdr_text.body[5] = 0x00;
    shdr_text.body[6] = 0x00;
    shdr_text.body[7] = 0xc3;
    }

    // .data section
    SectionContent shdr_data;
    set_section_header_table(
        ".data",
        SHT_PROGBITS,
        SHF_WRITE | SHF_ALLOC,
        0,
        e_shoff,
        0,
        0,
        0,
        1,
        0,
        &shdr_data.shdr);

    // .bss section
    SectionContent shdr_bss;
    set_section_header_table(
        ".bss",
        SHT_NOBITS,
        SHF_WRITE | SHF_ALLOC,
        0,
        e_shoff,
        0,
        0,
        0,
        1,
        0,
        &shdr_bss.shdr);

    // .symtab section
    SectionContent shdr_symtab;
    {
    size_t sh_size = 5 * sizeof(Elf_Sym);
    set_section_header_table(
        ".symtab",
        SHT_SYMTAB,
        0,
        0,
        e_shoff,
        sh_size,
        5,
        4,
        8,
        sizeof(Elf_Sym),
        &shdr_symtab.shdr);
    Elf_Sym sym[5];
    set_symbol_table(
        0,
        ELF_ST_INFO(STB_LOCAL, STT_NOTYPE),
        0,
        SHN_UNDEF,
        0,
        0,
        &sym[0]
    );
    set_symbol_table(
        0,
        ELF_ST_INFO(STB_LOCAL, STT_SECTION),
        0,
        1,
        0,
        0,
        &sym[1]
    );
    set_symbol_table(
        0,
        ELF_ST_INFO(STB_LOCAL, STT_SECTION),
        0,
        2,
        0,
        0,
        &sym[2]
    );
    set_symbol_table(
        0,
        ELF_ST_INFO(STB_LOCAL, STT_SECTION),
        0,
        3,
        0,
        0,
        &sym[3]
    );
    set_symbol_table(
        1,
        ELF_ST_INFO(STB_GLOBAL, STT_NOTYPE),
        0,
        1,
        0,
        0,
        &sym[4]
    );
    memcpy(shdr_symtab.body, &sym, sh_size);
    }

    // .strtab section
    SectionContent shdr_strtab;
    {
    size_t sh_size = 6;
    memcpy(strtab_body, "\0main\0", sh_size);
    set_section_header_table(
        ".strtab",
        SHT_STRTAB,
        0,
        0,
        e_shoff,
        sh_size,
        0,
        0,
        1,
        0,
        &shdr_strtab.shdr);
    memcpy(shdr_strtab.body, strtab_body, sh_size);
    }

    // .shstrtab section
    SectionContent shdr_shstrtab;
    {
    size_t sh_size = sh_name + strlen(".shstrtab") + 1; // add 1 for the trailing '\0'
    set_section_header_table(
        ".shstrtab",
        SHT_STRTAB,
        0,
        0,
        e_shoff,
        sh_size,
        0,
        0,
        1,
        0,
        &shdr_shstrtab.shdr);
    memcpy(shdr_shstrtab.body, shstrtab_body, sh_size);
    }

    Elf_Ehdr ehdr;
    e_shstrndx--;
    set_elf_header(
        e_shoff,
        e_shnum,
        e_shstrndx,
        &ehdr
    );
    SectionContent *section_list[] =
    {
        &shdr_null,
        &shdr_text,
        &shdr_data,
        &shdr_bss,
        &shdr_symtab,
        &shdr_strtab,
        &shdr_shstrtab
    };


    // output an object file
    FILE *fp = fopen(file_name, "wb");

    // ELF header
    fwrite(&ehdr, sizeof(Elf_Ehdr), 1, fp);

    // sections
    for(size_t i = 0; i < e_shnum; i++)
    {
        char *body = section_list[i]->body;
        size_t size = section_list[i]->shdr.sh_size;
        fwrite(body, size, 1, fp);
    }

    // section table
    for(size_t i = 0; i < e_shnum; i++)
    {
        Elf_Shdr *shdr = &section_list[i]->shdr;
        fwrite(shdr, sizeof(Elf_Shdr), 1, fp);
    }

    fclose(fp);
}
