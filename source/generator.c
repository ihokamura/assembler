#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "elf_wrap.h"
#include "generator.h"
#include "identifier.h"
#include "parser.h"
#include "processor.h"

typedef struct RelocationInfo RelocationInfo;
typedef struct SectionInfo SectionInfo;

struct Symbol
{
    const char *body;    // symbol body
    Elf_Addr address;    // address to be replaced
    Elf_Sxword addend;   // addend
    SectionKind section; // section of symbol
    bool resolved;       // flag indicating that the symbol is resolved
};

struct RelocationInfo
{
    SectionKind source;      // section of relocation source
    SectionKind destination; // section of relocation destination
    const char *body;        // label body
    Elf_Addr address;        // address to be replaced
    Elf_Sxword addend;       // addend
};

struct SectionInfo
{
    const ByteBufferType *body; // section body
    const Elf_Shdr *shdr;       // section header table entry
};


#include "list.h"
define_list(RelocationInfo)
define_list(SectionInfo)
define_list(Symbol)
define_list_operations(RelocationInfo)
define_list_operations(SectionInfo)
define_list_operations(Symbol)

static RelocationInfo *new_relocation_info(SectionKind source, SectionKind destination, const char *body, Elf_Addr address, Elf_Sxword addend);
static RelocationInfo *new_relocation_info_undefined(Symbol *symbol);
static SectionInfo *new_section_info(ByteBufferType *body, const Elf_Shdr *shdr);
static void set_elf_header
(
    Elf_Off e_shoff,
    Elf_Half e_shnum,
    Elf_Half e_shstrndx,
    Elf_Ehdr *ehdr
);
static Elf_Shdr *set_section_header_table
(
    const char *section_name,
    Elf_Word sh_type,
    Elf_Xword sh_flags,
    Elf_Addr sh_addr,
    Elf_Xword sh_size,
    Elf_Word sh_link,
    Elf_Word sh_info,
    Elf_Xword sh_addralign,
    Elf_Xword sh_entsize
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
    Elf_Sxword r_addend,
    ByteBufferType *rela_body
);
static void set_symbol_table_entries(void);
static Elf_Section get_section_index(SectionKind kind);
static ByteBufferType *get_section_body(SectionKind kind);
static ByteBufferType *get_rela_section_body(SectionKind kind);
static Elf_Xword get_symtab_index(size_t resolved_symbols, const RelocationInfo *reloc_info);
static void set_relocation_table_entries(size_t resolved_symbols);
static void generate_statement_list(const List(Statement) *statement_list);
static void resolve_symbols(const List(Label) *label_list);
static void classify_label_list(const List(Label) *label_list);
static void fill_paddings(size_t pad_size, FILE *fp);
static void generate_sections(const Program *program);
static void generate_section_header_table_entries(void);
static void generate_elf_header(void);

static const Elf_Word DEFAULT_SECTION_INFO = 0;

static const Elf_Xword DEFAULT_SECTION_ALIGNMENT = 1;
static const Elf_Xword RELA_SECTION_ALIGNMENT = 8;
static const Elf_Xword SYMTAB_SECTION_ALIGNMENT = 8;

static const Elf_Section SHNDX_TEXT = 1;
static const Elf_Section SHNDX_DATA = 3;
static const Elf_Section SHNDX_BSS = 4;
static const Elf_Section SHNDX_SYMTAB = 5;
static const Elf_Section SHNDX_STRTAB = 6;

static const size_t RESERVED_SYMTAB_ENTRIES = 4; // number of reserved symbol table entries (undefined, .text, .data, .bss)
static const Elf_Xword SYMTAB_INDEX_DATA = 2; // index of symbol table entry for .data section
static const Elf_Xword SYMTAB_INDEX_BSS = 3;  // index of symbol table entry for .bss section

static Elf_Off e_shoff = 0;     // offset of section header table
static Elf_Half e_shnum = 0;    // number of section header table entries
static Elf_Half e_shstrndx = 0; // index of section ".shstrtab"

static List(SectionInfo) *section_info_list;  // list of section information
static List(Label) *local_label_list;         // list of local labels
static List(Label) *global_label_list;        // list of global labels
static List(Symbol) *symbol_list;             // list of symbols
static List(Symbol) *unresolved_symbol_list;  // list of unresolved symbols
static List(RelocationInfo) *reloc_info_list; // list of relocatable symbols

static ByteBufferType text_body = {NULL, 0, 0};      // buffer for section ".text"
static ByteBufferType data_body = {NULL, 0, 0};      // buffer for section ".data"
static ByteBufferType rela_text_body = {NULL, 0, 0}; // buffer for section ".rela.text"
static ByteBufferType symtab_body = {NULL, 0, 0};    // buffer for section ".symtab"
static ByteBufferType strtab_body = {NULL, 0, 0};    // buffer for string containing names of symbols
static ByteBufferType shstrtab_body = {NULL, 0, 0};  // buffer for string containing names of sections

static size_t bss_size = 0; // size of section ".bss"

static Elf_Word sh_name = 0; // index of string where a section name starts

static Elf_Ehdr elf_header;


/*
make a new label information
*/
Symbol *new_symbol(const char *body, Elf_Addr address, Elf_Sxword addend)
{
    Symbol *symbol = calloc(1, sizeof(Symbol));
    symbol->body = body;
    symbol->address = address;
    symbol->addend = addend;
    symbol->section = SC_TEXT;
    symbol->resolved = false;
    add_list_entry_tail(Symbol)(symbol_list, symbol);

    return symbol;
}


/*
make a new relocatable symbol information
*/
static RelocationInfo *new_relocation_info(SectionKind source, SectionKind destination, const char *body, Elf_Addr address, Elf_Sxword addend)
{
    RelocationInfo *reloc_info = calloc(1, sizeof(RelocationInfo));
    reloc_info->source = source;
    reloc_info->destination = destination;
    reloc_info->body = body;
    reloc_info->address = address;
    reloc_info->addend = addend;
    add_list_entry_tail(RelocationInfo)(reloc_info_list, reloc_info);

    return reloc_info;
}


/*
make a new relocatable symbol information for undefined section
*/
static RelocationInfo *new_relocation_info_undefined(Symbol *symbol)
{
    // make a new relocation information
    RelocationInfo *reloc_info = new_relocation_info(symbol->section, SC_UND, symbol->body, symbol->address, symbol->addend);

    // serch the existing symbols to avoid duplication
    bool found = false;
    for_each_entry(Symbol, cursor, unresolved_symbol_list)
    {
        Symbol *existing_symbol = get_element(Symbol)(cursor);
        if(strcmp(symbol->body, existing_symbol->body) == 0)
        {
            found = true;
            break;
        }
    }

    if(!found)
    {
        add_list_entry_tail(Symbol)(unresolved_symbol_list, symbol);
    }

    return reloc_info;
}


/*
make a new section information
*/
static SectionInfo *new_section_info(ByteBufferType *body, const Elf_Shdr *shdr)
{
    SectionInfo *section_info = calloc(1, sizeof(SectionInfo));
    section_info->body = body;
    section_info->shdr = shdr;
    add_list_entry_tail(SectionInfo)(section_info_list, section_info);

    return section_info;
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
static Elf_Shdr *set_section_header_table
(
    const char *section_name,
    Elf_Word sh_type,
    Elf_Xword sh_flags,
    Elf_Addr sh_addr,
    Elf_Xword sh_size,
    Elf_Word sh_link,
    Elf_Word sh_info,
    Elf_Xword sh_addralign,
    Elf_Xword sh_entsize
)
{
    Elf_Shdr *shdr = calloc(1, sizeof(Elf_Shdr));

    // set offset
    Elf_Off sh_offset;
    if(e_shoff == 0)
    {
        e_shoff = sizeof(Elf_Ehdr);
        sh_offset = 0;
    }
    else
    {
        if(sh_addralign > 1)
        {
            e_shoff = align_to(e_shoff, sh_addralign);
        }
        sh_offset = e_shoff;
    }
    
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

    return shdr;
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
}


/*
set members of a relocation table entry
*/
static void set_relocation_table
(
    Elf_Addr r_offset,
    Elf_Xword r_info,
    Elf_Sxword r_addend,
    ByteBufferType *rela_body
)
{
    Elf_Rela rela;

    // set members
    rela.r_offset = r_offset;
    rela.r_info = r_info;
    rela.r_addend = r_addend;

    // update buffer
    append_bytes((char *)&rela, sizeof(rela), rela_body);
}


/*
set entries of symbol table
*/
static void set_symbol_table_entries(void)
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

    // put '\0' for invalid symbols at first
    append_bytes("\x00", 1, &strtab_body);
    Elf_Word st_name = 1;

    // local labels
    for_each_entry(Label, cursor, local_label_list)
    {
        Label *label = get_element(Label)(cursor);
        const char *body = label->body;
        set_symbol_table(
            st_name,
            ELF_ST_INFO(STB_LOCAL, STT_NOTYPE),
            0,
            get_section_index(label->statement->section),
            label->statement->address,
            0
        );
        append_bytes(body, strlen(body) + 1, &strtab_body);
        st_name += strlen(body) + 1;
    }

    // global labels
    for_each_entry(Label, cursor, global_label_list)
    {
        Label *label = get_element(Label)(cursor);
        const char *body = label->body;
        set_symbol_table(
            st_name,
            ELF_ST_INFO(STB_GLOBAL, STT_NOTYPE),
            0,
            get_section_index(label->statement->section),
            label->statement->address,
            0
        );
        append_bytes(body, strlen(body) + 1, &strtab_body);
        st_name += strlen(body) + 1;
    }

    // unresolved symbols
    for_each_entry(Symbol, cursor, unresolved_symbol_list)
    {
        Symbol *symbol = get_element(Symbol)(cursor);
        const char *body = symbol->body;
        set_symbol_table(
            st_name,
            ELF_ST_INFO(STB_GLOBAL, STT_NOTYPE),
            0,
            SHN_UNDEF,
            0,
            0
        );
        append_bytes(body, strlen(body) + 1, &strtab_body);
        st_name += strlen(body) + 1;
    }
}


/*
get index of section
*/
static Elf_Section get_section_index(SectionKind kind)
{
    switch(kind)
    {
    case SC_TEXT:
        return SHNDX_TEXT;

    case SC_DATA:
        return SHNDX_DATA;

    case SC_BSS:
        return SHNDX_BSS;

    default:
        assert(0);
        return 0;
    }
}


/*
get body of section
*/
static ByteBufferType *get_section_body(SectionKind kind)
{
    switch(kind)
    {
    case SC_TEXT:
        return &text_body;

    case SC_DATA:
        return &data_body;

    case SC_BSS:
        return NULL;

    default:
        assert(0);
        return NULL;
    }
}


/*
get body of relocation section
*/
static ByteBufferType *get_rela_section_body(SectionKind kind)
{
    switch(kind)
    {
    case SC_TEXT:
        return &rela_text_body;

    default:
        assert(0);
        return NULL;
    }
}


/*
get index of symbol table entry for relocatable symbol
*/
static Elf_Xword get_symtab_index(size_t resolved_symbols, const RelocationInfo *reloc_info)
{
    if(reloc_info->destination == SC_DATA)
    {
        return SYMTAB_INDEX_DATA;
    }
    else if(reloc_info->destination == SC_BSS)
    {
        return SYMTAB_INDEX_BSS;
    }
    else
    {
        Elf_Xword sym_index = RESERVED_SYMTAB_ENTRIES + resolved_symbols;
        for_each_entry(Symbol, cursor, unresolved_symbol_list)
        {
            Symbol *symbol = get_element(Symbol)(cursor);
            if(strcmp(reloc_info->body, symbol->body) == 0)
            {
                break;
            }
            sym_index++;
        }

        return sym_index;
    }
}


/*
set entries of relocation table
*/
static void set_relocation_table_entries(size_t resolved_symbols)
{
    for_each_entry(RelocationInfo, cursor, reloc_info_list)
    {
        RelocationInfo *reloc_info = get_element(RelocationInfo)(cursor);
        set_relocation_table(
            reloc_info->address,
            ELF_R_INFO(get_symtab_index(resolved_symbols, reloc_info), R_X86_64_PC32),
            reloc_info->addend,
            get_rela_section_body(reloc_info->source)
        );
    }
}


/*
generate statement list
*/
static void generate_statement_list(const List(Statement) *statement_list)
{
    for_each_entry(Statement, cursor, statement_list)
    {
        Statement *statement = get_element(Statement)(cursor);
        switch (statement->kind)
        {
        case ST_INSTRUCTION:
        {
            ByteBufferType *body = get_section_body(statement->section);
            statement->address = body->size;
            generate_operation(statement->operation, body);
        }
            break;

        case ST_VALUE:
        {
            ByteBufferType *body = get_section_body(statement->section);
            statement->address = body->size;
            generate_data(statement->data, body);
        }
            break;

        case ST_ZERO:
            statement->address = bss_size;
            bss_size += statement->bss->size;
            break;

        default:
            assert(0);
            break;
        }
    }
}


/*
resolve symbols
*/
static void resolve_symbols(const List(Label) *label_list)
{
    for_each_entry(Symbol, symbol_cursor, symbol_list)
    {
        Symbol *symbol = get_element(Symbol)(symbol_cursor);
        for_each_entry(Label, label_cursor, label_list)
        {
            Label *label = get_element(Label)(label_cursor);
            if(strcmp(symbol->body, label->body) == 0)
            {
                switch(label->statement->section)
                {
                case SC_TEXT:
                    *(uint32_t *)&text_body.body[symbol->address] = label->statement->address - (symbol->address + sizeof(uint32_t));
                    break;

                case SC_DATA:
                case SC_BSS:
                    new_relocation_info(symbol->section, label->statement->section, NULL, symbol->address, symbol->addend + label->statement->address);
                    break;

                default:
                    assert(0);
                    break;
                }

                symbol->resolved = true;
                break;
            }
        }

        if(!symbol->resolved)
        {
            new_relocation_info_undefined(symbol);
        }
    }
}


/*
classify labels
*/
static void classify_label_list(const List(Label) *label_list)
{
    for_each_entry(Label, cursor, label_list)
    {
        Label *label = get_element(Label)(cursor);
        if(label->kind == LB_LOCAL)
        {
            add_list_entry_tail(Label)(local_label_list, label);
        }
        else
        {
            add_list_entry_tail(Label)(global_label_list, label);
        }
    }
}


/*
fill padding bytes
*/
static void fill_paddings(size_t pad_size, FILE *fp)
{
    for(size_t i = 0; i < pad_size; i++)
    {
        char pad_byte = 0x00;
        fwrite(&pad_byte, sizeof(char), 1, fp);
    }
}


/*
generate section bodies
*/
static void generate_sections(const Program *program)
{
    generate_statement_list(program->statement_list);
    resolve_symbols(program->label_list);
    set_relocation_table_entries(get_length(Label)(program->label_list));
    classify_label_list(program->label_list);
    set_symbol_table_entries();
}


/*
generate section header table entries
*/
static void generate_section_header_table_entries(void)
{
   // undefined section
    Elf_Shdr *shdr_null = set_section_header_table(
        "",
        SHT_NULL,
        0,
        0,
        0,
        SHN_UNDEF,
        0,
        0,
        0);
    new_section_info(NULL, shdr_null);

    // .text section
    Elf_Shdr *shdr_text = set_section_header_table(
        ".text",
        SHT_PROGBITS,
        SHF_ALLOC | SHF_EXECINSTR,
        0,
        text_body.size,
        SHN_UNDEF,
        DEFAULT_SECTION_INFO,
        DEFAULT_SECTION_ALIGNMENT,
        0);
    new_section_info(&text_body, shdr_text);

    // .rela.text section
    Elf_Shdr *shdr_rela_text = set_section_header_table(
        ".rela.text",
        SHT_RELA,
        SHF_INFO_LINK,
        0,
        rela_text_body.size,
        SHNDX_SYMTAB, // sh_link holds section header index of the associated symbol table (i.e. .symtab section)
        SHNDX_TEXT, // sh_info holds section header index of the section to which the relocation applies (i.e. .text section)
        RELA_SECTION_ALIGNMENT,
        sizeof(Elf_Rela));
    new_section_info(&rela_text_body, shdr_rela_text);

    // .data section
    Elf_Shdr *shdr_data = set_section_header_table(
        ".data",
        SHT_PROGBITS,
        SHF_WRITE | SHF_ALLOC,
        0,
        data_body.size,
        SHN_UNDEF,
        DEFAULT_SECTION_INFO,
        DEFAULT_SECTION_ALIGNMENT,
        0);
    new_section_info(&data_body, shdr_data);

    // .bss section
    Elf_Shdr *shdr_bss = set_section_header_table(
        ".bss",
        SHT_NOBITS,
        SHF_WRITE | SHF_ALLOC,
        0,
        bss_size,
        SHN_UNDEF,
        DEFAULT_SECTION_INFO,
        DEFAULT_SECTION_ALIGNMENT,
        0);
    new_section_info(NULL, shdr_bss);

    // .symtab section
    Elf_Shdr *shdr_symtab = set_section_header_table(
        ".symtab",
        SHT_SYMTAB,
        0,
        0,
        symtab_body.size,
        SHNDX_STRTAB, // sh_link holds section header index of the associated string table (i.e. .strtab section)
        RESERVED_SYMTAB_ENTRIES + get_length(Label)(local_label_list), // sh_info holds one greater than the symbol table index of the laxt local symbol
        SYMTAB_SECTION_ALIGNMENT,
        sizeof(Elf_Sym));
    new_section_info(&symtab_body, shdr_symtab);

    // .strtab section
    Elf_Shdr *shdr_strtab = set_section_header_table(
        ".strtab",
        SHT_STRTAB,
        0,
        0,
        strtab_body.size,
        SHN_UNDEF,
        DEFAULT_SECTION_INFO,
        DEFAULT_SECTION_ALIGNMENT,
        0);
    new_section_info(&strtab_body, shdr_strtab);

    // .shstrtab section
    Elf_Shdr *shdr_shstrtab = set_section_header_table(
        ".shstrtab",
        SHT_STRTAB,
        0,
        0,
        sh_name + strlen(".shstrtab") + 1, // add 1 for the trailing '\0'
        SHN_UNDEF,
        DEFAULT_SECTION_INFO,
        DEFAULT_SECTION_ALIGNMENT,
        0);
    new_section_info(&shstrtab_body, shdr_shstrtab);
}


/*
generate ELF header
*/
static void generate_elf_header(void)
{
    e_shoff = align_to(e_shoff, sizeof(Elf_Shdr));
    e_shstrndx--;
    set_elf_header(
        e_shoff,
        e_shnum,
        e_shstrndx,
        &elf_header
    );
}


/*
generate an object file
*/
void generate(const char *output_file, const Program *program)
{
    // initialize lists
    section_info_list = new_list(SectionInfo)();
    symbol_list = new_list(Symbol)();
    local_label_list = new_list(Label)();
    global_label_list = new_list(Label)();
    unresolved_symbol_list = new_list(Symbol)();
    reloc_info_list = new_list(RelocationInfo)();

    // generate contents
    generate_sections(program);
    generate_section_header_table_entries();
    generate_elf_header();

    // output an object file
    FILE *fp = fopen(output_file, "wb");

    // ELF header
    fwrite(&elf_header, sizeof(Elf_Ehdr), 1, fp);

    // sections
    size_t end_pos = sizeof(Elf_Ehdr);
    for_each_entry(SectionInfo, cursor, section_info_list)
    {
        SectionInfo *section_info = get_element(SectionInfo)(cursor);
        size_t size = section_info->shdr->sh_size;
        if((section_info->body != NULL) && (size > 0))
        {
            size_t pad_size = section_info->shdr->sh_offset - end_pos;
            fill_paddings(pad_size, fp);

            char *body = section_info->body->body;
            fwrite(body, sizeof(char), size, fp);

            end_pos += (pad_size + size);
        }
    }

    // section header table entries
    fill_paddings(elf_header.e_shoff - end_pos, fp);
    for_each_entry(SectionInfo, cursor, section_info_list)
    {
        SectionInfo *section_info = get_element(SectionInfo)(cursor);
        const Elf_Shdr *shdr = section_info->shdr;
        fwrite(shdr, sizeof(Elf_Shdr), 1, fp);
    }

    fclose(fp);
}
