#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "elf_wrap.h"
#include "generator.h"
#include "parser.h"

#define INIT_SECTION_CONTENT    {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, NULL}

typedef struct
{
    Elf_Shdr shdr;
    ByteBufferType *body;
} SectionContent;

static void set_elf_header
(
    Elf_Off e_shoff,
    Elf_Half e_shnum,
    Elf_Half e_shstrndx,
    Elf_Ehdr *ehdr
);
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
);
static void set_symbol_table
(
    Elf_Word st_name,
    unsigned char st_info,
    unsigned char st_other,
    Elf_Section st_shndx,
    Elf_Addr st_value,
    Elf_Xword st_size,
    Elf_Sym *sym
);
static void generate_operations(const List(Operation) *operations);
static void generate_operation(const Operation *operation);
static void generate_op_mov(const List(Operand) *operands);
static void generate_op_ret(const List(Operand) *operands);
static uint8_t get_modrm_byte(uint8_t dst_encoding, uint8_t dst_index, uint8_t src_index);
static uint8_t get_encoding_index_rm(RegisterKind kind);
static void generate_symbols(const List(Symbol) *symbols);

// list of functions to generate operation
static const void (*generate_op_functions[])(const List(Operand) *) =
{
    NULL,
    generate_op_mov,
    generate_op_ret,
};

static const Elf_Xword DEFAULT_SECTION_ALIGNMENT = 1;
static const Elf_Xword SYMTAB_SECTION_ALIGNMENT = 8;

static Elf_Off e_shoff = sizeof(Elf_Ehdr);  // offset of section header table
static Elf_Half e_shnum = 0;                // number of section header table entries
static Elf_Half e_shstrndx = 0;             // index of section ".shstrtab"

static size_t local_symols = 0; // number of local symbols

static ByteBufferType text_body = {NULL, 0, 0};     // buffer for section ".text"
static ByteBufferType symtab_body = {NULL, 0, 0};   // buffer for section ".symtab"
static ByteBufferType strtab_body = {NULL, 0, 0};   // buffer for string containing names of symbols
static ByteBufferType shstrtab_body = {NULL, 0, 0}; // buffer for string containing names of sections
static Elf_Word sh_name = 0;                        // index of string where a section name starts

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
    ehdr->e_ident[EI_MAG0] = ELFMAG0;
    ehdr->e_ident[EI_MAG1] = ELFMAG1;
    ehdr->e_ident[EI_MAG2] = ELFMAG2;
    ehdr->e_ident[EI_MAG3] = ELFMAG3;
    ehdr->e_ident[EI_CLASS] = ELF_CLASS;
    ehdr->e_ident[EI_DATA] = ELF_DATA;
    ehdr->e_ident[EI_VERSION] = EV_CURRENT;
    ehdr->e_ident[EI_OSABI] = ELFOSABI_NONE;
    ehdr->e_ident[EI_ABIVERSION] = 0;

    ehdr->e_type = ET_REL;
    ehdr->e_machine = EM_MACHINE;
    ehdr->e_version = EV_CURRENT;

    ehdr->e_entry = 0;
    ehdr->e_phoff = 0;
    ehdr->e_shoff = e_shoff;
    ehdr->e_flags = 0;
    ehdr->e_ehsize = sizeof(Elf_Ehdr);
    ehdr->e_phentsize = 0;
    ehdr->e_phnum = 0;
    ehdr->e_shentsize = sizeof(Elf_Shdr);
    ehdr->e_shnum = e_shnum;
    ehdr->e_shstrndx = e_shstrndx;
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
    append_bytes(section_name, size, &shstrtab_body);
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
    // set members
    sym->st_name = st_name;
    sym->st_info = st_info;
    sym->st_other = st_other;
    sym->st_shndx = st_shndx;
    sym->st_value = st_value;
    sym->st_size = st_size;

    // count number of local symbols
    if(ELF_ST_BIND(st_info) == STB_LOCAL)
    {
        local_symols++;
    }
}


/*
generate operations
*/
static void generate_operations(const List(Operation) *operations)
{
    for_each_entry(Operation, cursor, operations)
    {
        Operation *operation = get_element(Operation)(cursor);
        generate_operation(operation);
    }
}


/*
generate an operation
*/
static void generate_operation(const Operation *operation)
{
    generate_op_functions[operation->kind](operation->operands);
}


/*
generate mov operation
*/
static void generate_op_mov(const List(Operand) *operands)
{
    ListEntry(Operand) *entry = get_first_entry(Operand)(operands);
    Operand *first = get_element(Operand)(entry);
    Operand *second = get_element(Operand)(next_entry(Operand, entry));
    if((first->kind == OP_R64) && (second->kind == OP_R64))
    {
        const char *prefixes = "\x48";
        append_bytes(prefixes, 1, &text_body);

        const char *opecode = "\x89";
        append_bytes(opecode, 1, &text_body);

        uint8_t dst_encoding = 0x03;
        uint8_t dst_index = get_encoding_index_rm(first->reg);
        uint8_t src_index = get_encoding_index_rm(second->reg);
        uint8_t modrm = get_modrm_byte(dst_encoding, dst_index, src_index);
        append_bytes((char *)&modrm, sizeof(modrm), &text_body);
    }
    else if((first->kind == OP_R64) && (second->kind == OP_IMM32))
    {
        const char *prefixes = "\x48";
        append_bytes(prefixes, 1, &text_body);

        const char *opecode = "\xc7";
        append_bytes(opecode, 1, &text_body);

        uint8_t dst_encoding = 0x03;
        uint8_t dst_index = get_encoding_index_rm(first->reg);
        uint8_t src_index = 0x00;
        uint8_t modrm = get_modrm_byte(dst_encoding, dst_index, src_index);
        append_bytes((char *)&modrm, sizeof(modrm), &text_body);

        uint32_t immediate = second->immediate;
        append_bytes((char *)&immediate, sizeof(immediate), &text_body);
    }
}


/*
generate ret operation
*/
static void generate_op_ret(const List(Operand) *operands)
{
    uint8_t mnemonic = 0xc3;
    append_bytes((char *)&mnemonic, sizeof(mnemonic), &text_body);
}


/*
get value of ModR/M byte
*/
static uint8_t get_modrm_byte(uint8_t dst_encoding, uint8_t dst_index, uint8_t src_index)
{
    return (dst_encoding << 6) + (src_index << 3) + dst_index;
}


/*
get index of register for addressing-mode encoding
*/
static uint8_t get_encoding_index_rm(RegisterKind kind)
{
    switch(kind)
    {
    case REG_RAX:
        return 0x00;

    case REG_RCX:
        return 0x01;

    case REG_RDX:
        return 0x02;

    case REG_RBX:
        return 0x03;

    case REG_RSP:
        return 0x04;

    case REG_RBP:
        return 0x05;

    case REG_RSI:
        return 0x06;

    case REG_RDI:
        return 0x07;

    default:
        return 0x00;
    }
}


/*
generate symbols
*/
static void generate_symbols(const List(Symbol) *symbols)
{
    append_bytes("\x00", 1, &strtab_body);
    for_each_entry(Symbol, cursor, symbols)
    {
        Symbol *symbol = get_element(Symbol)(cursor);
        const char *body = symbol->body;
        append_bytes(body, strlen(body) + 1, &strtab_body);
    }
}


/*
generate an object file
*/
void generate(const char *output_file, const Program *program)
{
   // undefined section
    SectionContent shdr_null = INIT_SECTION_CONTENT;
    set_section_header_table(
        "",
        SHT_NULL,
        0,
        0,
        0,
        0,
        SHN_UNDEF,
        0,
        0,
        0,
        &shdr_null.shdr);

    // .text section
    SectionContent shdr_text = INIT_SECTION_CONTENT;
    {
        generate_operations(program->operations);
        Elf_Xword sh_size = text_body.size;
        set_section_header_table(
            ".text",
            SHT_PROGBITS,
            SHF_ALLOC | SHF_EXECINSTR,
            0,
            e_shoff,
            sh_size,
            0,
            0,
            DEFAULT_SECTION_ALIGNMENT,
            0,
            &shdr_text.shdr);
        shdr_text.body = &text_body;
    }

    // .data section
    SectionContent shdr_data = INIT_SECTION_CONTENT;
    set_section_header_table(
        ".data",
        SHT_PROGBITS,
        SHF_WRITE | SHF_ALLOC,
        0,
        e_shoff,
        0,
        0,
        0,
        DEFAULT_SECTION_ALIGNMENT,
        0,
        &shdr_data.shdr);

    // .bss section
    SectionContent shdr_bss = INIT_SECTION_CONTENT;
    set_section_header_table(
        ".bss",
        SHT_NOBITS,
        SHF_WRITE | SHF_ALLOC,
        0,
        e_shoff,
        0,
        0,
        0,
        DEFAULT_SECTION_ALIGNMENT,
        0,
        &shdr_bss.shdr);

    // .symtab section
    SectionContent shdr_symtab = INIT_SECTION_CONTENT;
    {
        // define symbol table entries
        Elf_Sym *sym = calloc(e_shnum + 1, sizeof(Elf_Sym));
        // undefined symbol
        set_symbol_table(
            0,
            ELF_ST_INFO(STB_LOCAL, STT_NOTYPE),
            0,
            SHN_UNDEF,
            0,
            0,
            &sym[0]
        );
        // .text section
        set_symbol_table(
            0,
            ELF_ST_INFO(STB_LOCAL, STT_SECTION),
            0,
            1,
            0,
            0,
            &sym[1]
        );
        // .data section
        set_symbol_table(
            0,
            ELF_ST_INFO(STB_LOCAL, STT_SECTION),
            0,
            2,
            0,
            0,
            &sym[2]
        );
        // .bss section
        set_symbol_table(
            0,
            ELF_ST_INFO(STB_LOCAL, STT_SECTION),
            0,
            3,
            0,
            0,
            &sym[3]
        );
        // 'main'
        set_symbol_table(
            1,
            ELF_ST_INFO(STB_GLOBAL, STT_NOTYPE),
            0,
            1,
            0,
            0,
            &sym[4]
        );

        Elf_Xword sh_size = (e_shnum + 1) * sizeof(Elf_Sym); // add 1 for the symbol 'main'
        set_section_header_table(
            ".symtab",
            SHT_SYMTAB,
            0,
            0,
            e_shoff,
            sh_size,
            e_shnum + 1, // sh_link holds section header index of the associated string table (i.e. .strtab section)
            local_symols, // sh_info holds one greater than the symbol table index of the laxt local symbol
            SYMTAB_SECTION_ALIGNMENT,
            sizeof(Elf_Sym),
            &shdr_symtab.shdr);
        append_bytes((char *)sym, sh_size, &symtab_body);
        shdr_symtab.body = &symtab_body;
    }

    // .strtab section
    SectionContent shdr_strtab = INIT_SECTION_CONTENT;
    {
        generate_symbols(program->symbols);
        Elf_Xword sh_size = strtab_body.size;
        set_section_header_table(
            ".strtab",
            SHT_STRTAB,
            0,
            0,
            e_shoff,
            sh_size,
            0,
            0,
            DEFAULT_SECTION_ALIGNMENT,
            0,
            &shdr_strtab.shdr);
        shdr_strtab.body = &strtab_body;
    }

    // .shstrtab section
    SectionContent shdr_shstrtab = INIT_SECTION_CONTENT;
    {
        Elf_Xword sh_size = sh_name + strlen(".shstrtab") + 1; // add 1 for the trailing '\0'
        set_section_header_table(
            ".shstrtab",
            SHT_STRTAB,
            0,
            0,
            e_shoff,
            sh_size,
            0,
            0,
            DEFAULT_SECTION_ALIGNMENT,
            0,
            &shdr_shstrtab.shdr);
        shdr_shstrtab.body = &shstrtab_body;
    }

    // set ELF header
    Elf_Ehdr ehdr;
    e_shstrndx--;
    set_elf_header(
        e_shoff,
        e_shnum,
        e_shstrndx,
        &ehdr
    );

    // define sections
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
    FILE *fp = fopen(output_file, "wb");

    // ELF header
    fwrite(&ehdr, sizeof(Elf_Ehdr), 1, fp);

    // sections
    for(size_t i = 0; i < e_shnum; i++)
    {
        size_t size = section_list[i]->shdr.sh_size;
        if(size > 0)
        {
            char *body = section_list[i]->body->body;
            fwrite(body, size, 1, fp);
        }
    }

    // section table
    for(size_t i = 0; i < e_shnum; i++)
    {
        Elf_Shdr *shdr = &section_list[i]->shdr;
        fwrite(shdr, sizeof(Elf_Shdr), 1, fp);
    }

    fclose(fp);
}
