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

typedef struct LabelInfo LabelInfo;
struct LabelInfo
{
    const char *body; // label body
    Elf_Addr address; // address to be replaced
};

typedef struct UnresolvedSymbol UnresolvedSymbol;
struct UnresolvedSymbol
{
    const char *body;  // contents of symbol
    Elf_Addr address;  // address to be replaced
};

#include "list.h"
define_list(LabelInfo)
define_list(UnresolvedSymbol)
define_list_operations(LabelInfo)
define_list_operations(UnresolvedSymbol)

static LabelInfo *new_label_info(const char *body, Elf_Addr address);
static UnresolvedSymbol *new_unresolved_symbol(const char *body, Elf_Addr address);
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
    Elf_Xword st_size
);
static void set_relocation_table
(
    Elf_Addr r_offset,
    Elf_Xword r_info,
    Elf_Sxword r_addend
);
static void set_symbol_table_entries(const List(Symbol) *symbols);
static void set_relocation_table_entries(size_t resolved_symbols);
static void generate_operations(const List(Operation) *operations);
static void generate_operation(const Operation *operation);
static void generate_op_call(const List(Operand) *operands);
static void generate_op_nop(const List(Operand) *operands);
static void generate_op_mov(const List(Operand) *operands);
static void generate_op_ret(const List(Operand) *operands);
static uint8_t get_modrm_byte(uint8_t dst_encoding, uint8_t dst_index, uint8_t src_index);
static uint8_t get_encoding_index_rm(RegisterKind kind);
static void generate_symbols(const List(Symbol) *symbols);
static void resolve_symbol(const List(Symbol) *symbols);

// list of functions to generate operation
static const void (*generate_op_functions[])(const List(Operand) *) =
{
    generate_op_call,
    generate_op_mov,
    generate_op_nop,
    generate_op_ret,
};

static const Elf_Xword DEFAULT_SECTION_ALIGNMENT = 1;
static const Elf_Xword SYMTAB_SECTION_ALIGNMENT = 8;

static const Elf_Section SHNDX_TEXT = 1;
static const Elf_Section SHNDX_DATA = 2;
static const Elf_Section SHNDX_BSS = 3;
static const Elf_Section SHNDX_SYMTAB = 5;

static Elf_Off e_shoff = sizeof(Elf_Ehdr);  // offset of section header table
static Elf_Half e_shnum = 0;                // number of section header table entries
static Elf_Half e_shstrndx = 0;             // index of section ".shstrtab"

static List(LabelInfo) *label_info_list;               // list of label information
static List(UnresolvedSymbol) *unresolved_symbol_list; // list of unresolved symbols
static size_t local_symols = 0; // number of local symbols

static ByteBufferType text_body = {NULL, 0, 0};      // buffer for section ".text"
static ByteBufferType rela_text_body = {NULL, 0, 0}; // buffer for section ".rela.text"
static ByteBufferType symtab_body = {NULL, 0, 0};    // buffer for section ".symtab"
static ByteBufferType strtab_body = {NULL, 0, 0};    // buffer for string containing names of symbols
static ByteBufferType shstrtab_body = {NULL, 0, 0};  // buffer for string containing names of sections

static Elf_Word sh_name = 0; // index of string where a section name starts

/*
make a new label information
*/
static LabelInfo *new_label_info(const char *body, Elf_Addr address)
{
    LabelInfo *label_info = calloc(1, sizeof(LabelInfo));
    label_info->body = body;
    label_info->address = address;

    return label_info;
}


/*
make a new unresolved symbol
*/
static UnresolvedSymbol *new_unresolved_symbol(const char *body, Elf_Addr address)
{
    UnresolvedSymbol *unresolved_symbol = calloc(1, sizeof(UnresolvedSymbol));
    unresolved_symbol->body = body;
    unresolved_symbol->address = address;

    return unresolved_symbol;
}


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
    Elf_Xword st_size
)
{
    Elf_Sym sym;

    // set members
    sym.st_name = st_name;
    sym.st_info = st_info;
    sym.st_other = st_other;
    sym.st_shndx = st_shndx;
    sym.st_value = st_value;
    sym.st_size = st_size;

    // update buffer
    append_bytes((char *)&sym, sizeof(sym), &symtab_body);

    // count number of local symbols
    if(ELF_ST_BIND(st_info) == STB_LOCAL)
    {
        local_symols++;
    }
}


/*
set members of a relocation table entry
*/
static void set_relocation_table
(
    Elf_Addr r_offset,
    Elf_Xword r_info,
    Elf_Sxword r_addend
)
{
    Elf_Rela rela;

    // set members
    rela.r_offset = r_offset;
    rela.r_info = r_info;
    rela.r_addend = r_addend;

    // update buffer
    append_bytes((char *)&rela, sizeof(rela), &rela_text_body);
}


/*
set entries of symbol table
*/
static void set_symbol_table_entries(const List(Symbol) *symbols)
{
    // undefined symbol
    set_symbol_table(
        0,
        ELF_ST_INFO(STB_LOCAL, STT_NOTYPE),
        0,
        SHN_UNDEF,
        0,
        0
    );

    // .text section
    set_symbol_table(
        0,
        ELF_ST_INFO(STB_LOCAL, STT_SECTION),
        0,
        SHNDX_TEXT,
        0,
        0
    );

    // .data section
    set_symbol_table(
        0,
        ELF_ST_INFO(STB_LOCAL, STT_SECTION),
        0,
        SHNDX_DATA,
        0,
        0
    );

    // .bss section
    set_symbol_table(
        0,
        ELF_ST_INFO(STB_LOCAL, STT_SECTION),
        0,
        SHNDX_BSS,
        0,
        0
    );

    // resolved symbols
    Elf_Word st_name = 1;
    for_each_entry(Symbol, cursor, symbols)
    {
        Symbol *symbol = get_element(Symbol)(cursor);
        unsigned char bind = (symbol->kind == SY_GLOBAL) ? STB_GLOBAL : STB_LOCAL;
        set_symbol_table(
            st_name,
            ELF_ST_INFO(bind, STT_NOTYPE),
            0,
            SHNDX_TEXT,
            symbol->operation->address,
            0
        );
        st_name += strlen(symbol->body) + 1;
    }

    // unresolved symbols
    for_each_entry(UnresolvedSymbol, cursor, unresolved_symbol_list)
    {
        UnresolvedSymbol *unresolved_symbol = get_element(UnresolvedSymbol)(cursor);
        set_symbol_table(
            st_name,
            ELF_ST_INFO(STB_GLOBAL, STT_NOTYPE),
            0,
            SHN_UNDEF,
            0,
            0
        );
        st_name += strlen(unresolved_symbol->body) + 1;
    }
}


/*
set entries of relocation table
*/
static void set_relocation_table_entries(size_t resolved_symbols)
{
    Elf64_Xword sym_index = 4 + resolved_symbols; // symbol table index with respect to which the relocation will be made
    for_each_entry(UnresolvedSymbol, cursor, unresolved_symbol_list)
    {
        UnresolvedSymbol *unresolved_symbol = get_element(UnresolvedSymbol)(cursor);
        set_relocation_table(
            unresolved_symbol->address,
            ELF_R_INFO(sym_index, R_X86_64_PC32),
            -sizeof(uint32_t)
        );
        sym_index++;
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
        operation->address = text_body.size;

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
generate call operation
*/
static void generate_op_call(const List(Operand) *operands)
{
    Operand *operand = get_first_element(Operand)(operands);
    if(operand->kind == OP_SYMBOL)
    {
        LabelInfo *label_info = new_label_info(operand->label, text_body.size + 1);
        add_list_entry_tail(LabelInfo)(label_info_list, label_info);

        const char *opecode = "\xe8";
        append_bytes(opecode, 1, &text_body);

        uint32_t rel32 = 0; // temporal value
        append_bytes((char *)&rel32, sizeof(rel32), &text_body);
    }
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
generate nop operation
*/
static void generate_op_nop(const List(Operand) *operands)
{
    uint8_t mnemonic = 0x90;
    append_bytes((char *)&mnemonic, sizeof(mnemonic), &text_body);
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
    for_each_entry(UnresolvedSymbol, cursor, unresolved_symbol_list)
    {
        UnresolvedSymbol *unresolved_symbol = get_element(UnresolvedSymbol)(cursor);
        const char *body = unresolved_symbol->body;
        append_bytes(body, strlen(body) + 1, &strtab_body);
    }
}


/*
resolve symbols
*/
static void resolve_symbol(const List(Symbol) *symbols)
{
    for_each_entry(LabelInfo, label_cursor, label_info_list)
    {
        LabelInfo *label = get_element(LabelInfo)(label_cursor);
        bool resolved = false;

        for_each_entry(Symbol, symbol_cursor, symbols)
        {
            Symbol *symbol = get_element(Symbol)(symbol_cursor);
            if(strcmp(label->body, symbol->body) == 0)
            {
                uint32_t rel32 = symbol->operation->address - (label->address + sizeof(uint32_t));
                *(uint32_t *)&text_body.body[label->address] = rel32;
                resolved = true;
                break;
            }
        }

        if(!resolved)
        {
            add_list_entry_tail(UnresolvedSymbol)(unresolved_symbol_list, new_unresolved_symbol(label->body, label->address));
        }
    }
}


/*
generate an object file
*/
void generate(const char *output_file, const Program *program)
{
    label_info_list = new_list(LabelInfo)();
    unresolved_symbol_list = new_list(UnresolvedSymbol)();

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
    generate_operations(program->operations);
    resolve_symbol(program->symbols);
    set_section_header_table(
        ".text",
        SHT_PROGBITS,
        SHF_ALLOC | SHF_EXECINSTR,
        0,
        e_shoff,
        text_body.size,
        0,
        0,
        DEFAULT_SECTION_ALIGNMENT,
        0,
        &shdr_text.shdr);
    shdr_text.body = &text_body;

    // .rela.text section
    SectionContent shdr_rela_text = INIT_SECTION_CONTENT;
    set_relocation_table_entries(get_length(Symbol)(program->symbols));
    set_section_header_table(
        ".rela.text",
        SHT_RELA,
        SHF_INFO_LINK,
        0,
        e_shoff,
        rela_text_body.size,
        SHNDX_SYMTAB,
        SHNDX_TEXT,
        DEFAULT_SECTION_ALIGNMENT,
        sizeof(Elf_Rela),
        &shdr_rela_text.shdr);
    shdr_rela_text.body = &rela_text_body;

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
    set_symbol_table_entries(program->symbols);
    set_section_header_table(
        ".symtab",
        SHT_SYMTAB,
        0,
        0,
        e_shoff,
        symtab_body.size,
        e_shnum + 1, // sh_link holds section header index of the associated string table (i.e. .strtab section)
        local_symols, // sh_info holds one greater than the symbol table index of the laxt local symbol
        SYMTAB_SECTION_ALIGNMENT,
        sizeof(Elf_Sym),
        &shdr_symtab.shdr);
    shdr_symtab.body = &symtab_body;

    // .strtab section
    SectionContent shdr_strtab = INIT_SECTION_CONTENT;
    generate_symbols(program->symbols);
    set_section_header_table(
        ".strtab",
        SHT_STRTAB,
        0,
        0,
        e_shoff,
        strtab_body.size,
        0,
        0,
        DEFAULT_SECTION_ALIGNMENT,
        0,
        &shdr_strtab.shdr);
    shdr_strtab.body = &strtab_body;

    // .shstrtab section
    SectionContent shdr_shstrtab = INIT_SECTION_CONTENT;
    set_section_header_table(
        ".shstrtab",
        SHT_STRTAB,
        0,
        0,
        e_shoff,
        sh_name + strlen(".shstrtab") + 1, // add 1 for the trailing '\0'
        0,
        0,
        DEFAULT_SECTION_ALIGNMENT,
        0,
        &shdr_shstrtab.shdr);
    shdr_shstrtab.body = &shstrtab_body;

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
        &shdr_rela_text,
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
