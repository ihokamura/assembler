#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "elf_wrap.h"
#include "generator.h"
#include "parser.h"
#include "processor.h"

typedef struct RelocationInfo RelocationInfo;
typedef struct SectionInfo SectionInfo;

struct LabelInfo
{
    const char *body;  // label body
    Elf_Section shndx; // section index
    Elf_Addr address;  // address to be replaced
    Elf_Sxword addend; // addend
};

struct RelocationInfo
{
    const LabelInfo *label; // information on label
};

struct SectionInfo
{
    ByteBufferType *body; // section body
    Elf_Shdr shdr;        // section header table entry
};


#include "list.h"
define_list(LabelInfo)
define_list(RelocationInfo)
define_list(SectionInfo)
define_list_operations(LabelInfo)
define_list_operations(RelocationInfo)
define_list_operations(SectionInfo)

static RelocationInfo *new_relocation_info(const LabelInfo *label);
static SectionInfo *new_section_info(ByteBufferType *body, const Elf_Shdr *shdr);
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
static Elf_Xword get_symtab_index(size_t resolved_symbols, const LabelInfo *label);
static void set_relocation_table_entries(size_t resolved_symbols);
static void generate_operations(const List(Operation) *operations);
static void generate_data_list(const List(Data) *data_list);
static void resolve_symbols(const List(Symbol) *symbols);
static void classify_symbols(const List(Symbol) *symbols);
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

size_t RESERVED_SYMTAB_ENTRIES = 4; // number of reserved symbol table entries (undefined, .text, .data, .bss)

static Elf_Off e_shoff = 0;     // offset of section header table
static Elf_Half e_shnum = 0;    // number of section header table entries
static Elf_Half e_shstrndx = 0; // index of section ".shstrtab"

static List(SectionInfo) *section_info_list;  // list of section information
static List(LabelInfo) *label_info_list;      // list of label information
static List(Symbol) *local_symbol_list;       // list of local symbols
static List(Symbol) *global_symbol_list;      // list of global symbols
static List(Symbol) *unresolved_symbol_list;  // list of unresolved symbols
static List(RelocationInfo) *reloc_info_list; // list of relocatable symbols

static ByteBufferType text_body = {NULL, 0, 0};      // buffer for section ".text"
static ByteBufferType data_body = {NULL, 0, 0};      // buffer for section ".data"
static ByteBufferType rela_text_body = {NULL, 0, 0}; // buffer for section ".rela.text"
static ByteBufferType symtab_body = {NULL, 0, 0};    // buffer for section ".symtab"
static ByteBufferType strtab_body = {NULL, 0, 0};    // buffer for string containing names of symbols
static ByteBufferType shstrtab_body = {NULL, 0, 0};  // buffer for string containing names of sections

static Elf_Word sh_name = 0; // index of string where a section name starts

static Elf_Ehdr elf_header;


/*
make a new label information
*/
LabelInfo *new_label_info(const char *body, Elf_Addr address, Elf_Sxword addend)
{
    LabelInfo *label_info = calloc(1, sizeof(LabelInfo));
    label_info->body = body;
    label_info->shndx = SHNDX_TEXT;
    label_info->address = address;
    label_info->addend = addend;
    add_list_entry_tail(LabelInfo)(label_info_list, label_info);

    return label_info;
}


/*
make a new relocatable symbol
*/
static RelocationInfo *new_relocation_info(const LabelInfo *label)
{
    // make a new relocation information
    RelocationInfo *reloc_info = calloc(1, sizeof(RelocationInfo));
    reloc_info->label = label;
    add_list_entry_tail(RelocationInfo)(reloc_info_list, reloc_info);

    // serch the existing symbols to avoid duplication
    bool new_symbol = true;
    for_each_entry(Symbol, cursor, unresolved_symbol_list)
    {
        Symbol *symbol = get_element(Symbol)(cursor);
        if(strcmp(label->body, symbol->body) == 0)
        {
            new_symbol = false;
            break;
        }
    }

    if(new_symbol)
    {
        // make a new symbol
        Symbol *symbol = calloc(1, sizeof(Symbol));
        symbol->kind = SY_GLOBAL;
        symbol->body = label->body;
        symbol->operation = NULL;
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
    section_info->shdr = *shdr;
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
static void set_section_header_table
(
    const char *section_name,
    Elf_Word sh_type,
    Elf_Xword sh_flags,
    Elf_Addr sh_addr,
    Elf_Xword sh_size,
    Elf_Word sh_link,
    Elf_Word sh_info,
    Elf_Xword sh_addralign,
    Elf_Xword sh_entsize,
    Elf_Shdr *shdr
)
{
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

    // put '\0' for invalid symbols at first
    append_bytes("\x00", 1, &strtab_body);
    Elf_Word st_name = 1;

    // local symbols
    for_each_entry(Symbol, cursor, local_symbol_list)
    {
        Symbol *symbol = get_element(Symbol)(cursor);
        const char *body = symbol->body;
        set_symbol_table(
            st_name,
            ELF_ST_INFO(STB_LOCAL, STT_NOTYPE),
            0,
            (symbol->operation != NULL) ? SHNDX_TEXT : SHNDX_DATA,
            (symbol->operation != NULL) ? symbol->operation->address : symbol->data->offset,
            0
        );
        append_bytes(body, strlen(body) + 1, &strtab_body);
        st_name += strlen(body) + 1;
    }

    // global symbols
    for_each_entry(Symbol, cursor, global_symbol_list)
    {
        Symbol *symbol = get_element(Symbol)(cursor);
        const char *body = symbol->body;
        set_symbol_table(
            st_name,
            ELF_ST_INFO(STB_GLOBAL, STT_NOTYPE),
            0,
            (symbol->operation != NULL) ? SHNDX_TEXT : SHNDX_DATA,
            (symbol->operation != NULL) ? symbol->operation->address : symbol->data->offset,
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
get index of symbol table entry for relocatable symbol
*/
static Elf_Xword get_symtab_index(size_t resolved_symbols, const LabelInfo *label)
{
    if(label->shndx == SHNDX_DATA)
    {
        const static Elf_Xword SYMTAB_INDEX_DATA = 2;
        return SYMTAB_INDEX_DATA;
    }

    Elf_Xword sym_index = RESERVED_SYMTAB_ENTRIES + resolved_symbols;
    for_each_entry(Symbol, cursor, unresolved_symbol_list)
    {
        Symbol *symbol = get_element(Symbol)(cursor);
        if(strcmp(label->body, symbol->body) == 0)
        {
            break;
        }
        sym_index++;
    }

    return sym_index;
}


/*
set entries of relocation table
*/
static void set_relocation_table_entries(size_t resolved_symbols)
{
    for_each_entry(RelocationInfo, cursor, reloc_info_list)
    {
        RelocationInfo *reloc_info = get_element(RelocationInfo)(cursor);
        const LabelInfo *label = reloc_info->label;
        set_relocation_table(
            label->address,
            ELF_R_INFO(get_symtab_index(resolved_symbols, label), R_X86_64_PC32),
            label->addend
        );
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

        generate_operation(operation, &text_body);
    }
}


/*
generate data list
*/
static void generate_data_list(const List(Data) *data_list)
{
    for_each_entry(Data, cursor, data_list)
    {
        Data *data = get_element(Data)(cursor);
        data->offset = data_body.size;

        //generate_data(data, &data_body);
        generate_data(data, &data_body);
    }
}


/*
resolve symbols
*/
static void resolve_symbols(const List(Symbol) *symbols)
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
                if(symbol->operation != NULL)
                {
                    uint32_t rel32 = symbol->operation->address - (label->address + sizeof(uint32_t));
                    *(uint32_t *)&text_body.body[label->address] = rel32;
                }
                if(symbol->data != NULL)
                {
                    uint32_t rel32 = symbol->data->offset;
                    *(uint32_t *)&data_body.body[label->address] = rel32;

                    LabelInfo *data_label = calloc(1, sizeof(LabelInfo));
                    data_label->body = "";
                    data_label->shndx = SHNDX_DATA;
                    data_label->address = label->address;
                    data_label->addend = label->addend;

                    RelocationInfo *reloc_info = calloc(1, sizeof(RelocationInfo));
                    reloc_info->label = data_label;
                    add_list_entry_tail(RelocationInfo)(reloc_info_list, reloc_info);
                }

                resolved = true;
                break;
            }
        }

        if(!resolved)
        {
            new_relocation_info(label);
        }
    }
}


/*
classify symbols
*/
static void classify_symbols(const List(Symbol) *symbols)
{
    for_each_entry(Symbol, cursor, symbols)
    {
        Symbol *symbol = get_element(Symbol)(cursor);
        if(symbol->kind == SY_LOCAL)
        {
            add_list_entry_tail(Symbol)(local_symbol_list, symbol);
        }
        else
        {
            add_list_entry_tail(Symbol)(global_symbol_list, symbol);
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
    generate_operations(program->operations);
    generate_data_list(program->data_list);
    resolve_symbols(program->symbols);
    set_relocation_table_entries(get_length(Symbol)(program->symbols));
    classify_symbols(program->symbols);
    set_symbol_table_entries(program->symbols);
}


/*
generate section header table entries
*/
static void generate_section_header_table_entries(void)
{
   // undefined section
    Elf_Shdr shdr_null;
    set_section_header_table(
        "",
        SHT_NULL,
        0,
        0,
        0,
        SHN_UNDEF,
        0,
        0,
        0,
        &shdr_null);
    new_section_info(NULL, &shdr_null);

    // .text section
    Elf_Shdr shdr_text;
    set_section_header_table(
        ".text",
        SHT_PROGBITS,
        SHF_ALLOC | SHF_EXECINSTR,
        0,
        text_body.size,
        SHN_UNDEF,
        DEFAULT_SECTION_INFO,
        DEFAULT_SECTION_ALIGNMENT,
        0,
        &shdr_text);
    new_section_info(&text_body, &shdr_text);

    // .rela.text section
    Elf_Shdr shdr_rela_text;
    set_section_header_table(
        ".rela.text",
        SHT_RELA,
        SHF_INFO_LINK,
        0,
        rela_text_body.size,
        SHNDX_SYMTAB, // sh_link holds section header index of the associated symbol table (i.e. .symtab section)
        SHNDX_TEXT, // sh_info holds section header index of the section to which the relocation applies (i.e. .text section)
        RELA_SECTION_ALIGNMENT,
        sizeof(Elf_Rela),
        &shdr_rela_text);
    new_section_info(&rela_text_body, &shdr_rela_text);

    // .data section
    Elf_Shdr shdr_data;
    set_section_header_table(
        ".data",
        SHT_PROGBITS,
        SHF_WRITE | SHF_ALLOC,
        0,
        data_body.size,
        SHN_UNDEF,
        DEFAULT_SECTION_INFO,
        DEFAULT_SECTION_ALIGNMENT,
        0,
        &shdr_data);
    new_section_info(&data_body, &shdr_data);

    // .bss section
    Elf_Shdr shdr_bss;
    set_section_header_table(
        ".bss",
        SHT_NOBITS,
        SHF_WRITE | SHF_ALLOC,
        0,
        0,
        SHN_UNDEF,
        DEFAULT_SECTION_INFO,
        DEFAULT_SECTION_ALIGNMENT,
        0,
        &shdr_bss);
    new_section_info(NULL, &shdr_bss);

    // .symtab section
    Elf_Shdr shdr_symtab;
    set_section_header_table(
        ".symtab",
        SHT_SYMTAB,
        0,
        0,
        symtab_body.size,
        SHNDX_STRTAB, // sh_link holds section header index of the associated string table (i.e. .strtab section)
        RESERVED_SYMTAB_ENTRIES + get_length(Symbol)(local_symbol_list), // sh_info holds one greater than the symbol table index of the laxt local symbol
        SYMTAB_SECTION_ALIGNMENT,
        sizeof(Elf_Sym),
        &shdr_symtab);
    new_section_info(&symtab_body, &shdr_symtab);

    // .strtab section
    Elf_Shdr shdr_strtab;
    set_section_header_table(
        ".strtab",
        SHT_STRTAB,
        0,
        0,
        strtab_body.size,
        SHN_UNDEF,
        DEFAULT_SECTION_INFO,
        DEFAULT_SECTION_ALIGNMENT,
        0,
        &shdr_strtab);
    new_section_info(&strtab_body, &shdr_strtab);

    // .shstrtab section
    Elf_Shdr shdr_shstrtab;
    set_section_header_table(
        ".shstrtab",
        SHT_STRTAB,
        0,
        0,
        sh_name + strlen(".shstrtab") + 1, // add 1 for the trailing '\0'
        SHN_UNDEF,
        DEFAULT_SECTION_INFO,
        DEFAULT_SECTION_ALIGNMENT,
        0,
        &shdr_shstrtab);
    new_section_info(&shstrtab_body, &shdr_shstrtab);
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
    label_info_list = new_list(LabelInfo)();
    local_symbol_list = new_list(Symbol)();
    global_symbol_list = new_list(Symbol)();
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
        size_t size = section_info->shdr.sh_size;
        if(size > 0)
        {
            size_t pad_size = section_info->shdr.sh_offset - end_pos;
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
        Elf_Shdr *shdr = &section_info->shdr;
        fwrite(shdr, sizeof(Elf_Shdr), 1, fp);
    }

    fclose(fp);
}
