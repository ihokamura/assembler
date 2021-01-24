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
#include "section.h"

typedef struct RelocationInfo RelocationInfo;

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

#include "list.h"
define_list(Elf_Shdr)
define_list(RelocationInfo)
define_list(Symbol)
define_list_operations(Elf_Shdr)
define_list_operations(RelocationInfo)
define_list_operations(Symbol)

static RelocationInfo *new_relocation_info(SectionKind source, SectionKind destination, const char *body, Elf_Addr address, Elf_Sxword addend);
static RelocationInfo *new_relocation_info_undefined(Symbol *symbol);
static void set_elf_header
(
    Elf_Off e_shoff,
    Elf_Half e_shnum,
    Elf_Half e_shstrndx,
    Elf_Ehdr *ehdr
);
static Elf_Shdr *new_section_header_table
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
static Elf_Xword get_symtab_index(size_t resolved_symbols, const RelocationInfo *reloc_info);
static Elf_Word get_strtab_position(const ByteBufferType *strtab_body, const char *name);
static void set_relocation_table_entries(size_t resolved_symbols);
static void generate_statement_list(const List(Statement) *statement_list);
static void resolve_symbols(const List(Label) *label_list);
static void classify_label_list(const List(Label) *label_list);
static void fill_paddings(size_t pad_size, FILE *fp);
static size_t fwrite_section(const ByteBufferType *buffer, Elf_Off offset, size_t end_pos, FILE *fp);
static void generate_sections(const Program *program);
static void generate_section_header_table_entries(void);
static void generate_elf_header(Elf_Ehdr *ehdr);

static const Elf_Word DEFAULT_SECTION_INFO = 0;

static const size_t RESERVED_SYMTAB_ENTRIES = 4; // number of reserved symbol table entries (undefined, .text, .data, .bss)
static const Elf_Xword SYMTAB_INDEX_DATA = 2; // index of symbol table entry for .data section
static const Elf_Xword SYMTAB_INDEX_BSS = 3;  // index of symbol table entry for .bss section

static List(Elf_Shdr) *shdr_list;             // list of section header table entries
static List(Label) *local_label_list;         // list of local labels
static List(Label) *global_label_list;        // list of global labels
static List(Symbol) *symbol_list;             // list of symbols
static List(Symbol) *unresolved_symbol_list;  // list of unresolved symbols
static List(RelocationInfo) *reloc_info_list; // list of relocatable symbols

static ByteBufferType symtab_body = {NULL, 0, 0};    // buffer for section ".symtab"
static ByteBufferType strtab_body = {NULL, 0, 0};    // buffer for string containing names of symbols
static ByteBufferType shstrtab_body = {NULL, 0, 0};  // buffer for string containing names of sections


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
static Elf_Shdr *new_section_header_table
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
    Elf_Xword sh_entsize
)
{
    Elf_Shdr *shdr = calloc(1, sizeof(Elf_Shdr));

    // set members
    shdr->sh_name = get_strtab_position(&shstrtab_body, section_name);
    shdr->sh_type = sh_type;
    shdr->sh_flags = sh_flags;
    shdr->sh_addr = sh_addr;
    shdr->sh_offset = sh_offset;
    shdr->sh_size = sh_size;
    shdr->sh_link = sh_link;
    shdr->sh_info = sh_info;
    shdr->sh_addralign = sh_addralign;
    shdr->sh_entsize = sh_entsize;

    // update list of section header table entries
    add_list_entry_tail(Elf_Shdr)(shdr_list, shdr);

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
        get_base_section(SC_TEXT)->index,
        0,
        0
    );

    // .data section
    set_symbol_table(
        0,
        ELF_ST_INFO(STB_LOCAL, STT_SECTION),
        0,
        get_base_section(SC_DATA)->index,
        0,
        0
    );

    // .bss section
    set_symbol_table(
        0,
        ELF_ST_INFO(STB_LOCAL, STT_SECTION),
        0,
        get_base_section(SC_BSS)->index,
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
            get_base_section(label->statement->section)->index,
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
            get_base_section(label->statement->section)->index,
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
get string table index of name
*/
static Elf_Word get_strtab_position(const ByteBufferType *strtab_body, const char *name)
{
    size_t len = strlen(name);
    size_t range = strtab_body->size - len;

    for(Elf_Word pos = 0; pos < range; pos++)
    {
        const char *start = &strtab_body->body[pos];
        bool equal = true;
        for(size_t i = 0; i < len; i++)
        {
            if(start[i] != name[i])
            {
                equal = false;
                break;
            }
        }

        if(equal)
        {
            return pos;
        }
    }

    return range;
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
            get_base_section(reloc_info->source)->rela_body
        );
    }
}


/*
update section
*/
static void update_section(Statement *statement, BaseSection *base_section)
{
    size_t section_size = 0;
    statement->address = base_section->size;
    switch(statement->kind)
    {
    case ST_INSTRUCTION:
        {
        ByteBufferType *body = base_section->body;
        Operation *operation = statement->operation;
        generate_operation(operation, body);
        section_size = body->size;
        }
        break;

    case ST_VALUE:
        {
        ByteBufferType *body = base_section->body;
        Data *data = statement->data;
        append_bytes((char *)&data->value, data->size, body);
        section_size = body->size;
        }
        break;

    case ST_ZERO:
        section_size = base_section->size + statement->bss->size;
        break;

    default:
        assert(0);
        break;
    }

    base_section->size = section_size;
}


/*
generate statement list
*/
static void generate_statement_list(const List(Statement) *statement_list)
{
    for_each_entry(Statement, cursor, statement_list)
    {
        Statement *statement = get_element(Statement)(cursor);
        BaseSection *base_section = get_base_section(statement->section);
        update_section(statement, base_section);
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
                    *(uint32_t *)&get_base_section(SC_TEXT)->body->body[symbol->address] = label->statement->address - (symbol->address + sizeof(uint32_t));
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
output section body
*/
static size_t fwrite_section(const ByteBufferType *buffer, Elf_Off offset, size_t end_pos, FILE *fp)
{
    size_t size = buffer->size;
    if(size > 0)
    {
        size_t pad_size = offset - end_pos;
        fill_paddings(pad_size, fp);

        fwrite(buffer->body, sizeof(char), size, fp);

        return end_pos + pad_size + size;
    }
    else
    {
        return end_pos;
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
    make_shstrtab(&shstrtab_body);
    make_metadata_sections(&symtab_body, &strtab_body, &shstrtab_body);
    classify_label_list(program->label_list);
    set_symbol_table_entries();
    set_offset_of_sections();
}


/*
generate section header table entries
*/
static void generate_section_header_table_entries(void)
{
    BaseSection *base_section_und = get_base_section(SC_UND);
    BaseSection *base_section_text = get_base_section(SC_TEXT);
    BaseSection *base_section_data = get_base_section(SC_DATA);
    BaseSection *base_section_bss = get_base_section(SC_BSS);
    BaseSection *base_section_symtab = get_base_section(SC_SYMTAB);
    BaseSection *base_section_strtab = get_base_section(SC_STRTAB);
    BaseSection *base_section_shstrtab = get_base_section(SC_SHSTRTAB);

    // undefined section
    new_section_header_table(
        "",
        base_section_und->type,
        base_section_und->flags,
        0,
        base_section_und->offset,
        base_section_und->size,
        SHN_UNDEF,
        0,
        base_section_und->alignment,
        0);

    // .text section
    new_section_header_table(
        ".text",
        base_section_text->type,
        base_section_text->flags,
        0,
        base_section_text->offset,
        base_section_text->size,
        SHN_UNDEF,
        DEFAULT_SECTION_INFO,
        base_section_text->alignment,
        0);

    // .rela.text section
    if(base_section_text->rela_body->size > 0)
    {
        new_section_header_table(
            ".rela.text",
            SHT_RELA,
            SHF_INFO_LINK,
            0,
            base_section_text->rela_offset,
            base_section_text->rela_body->size,
            base_section_symtab->index, // sh_link holds section header index of the associated symbol table (i.e. .symtab section)
            base_section_text->index, // sh_info holds section header index of the section to which the relocation applies (i.e. .text section)
            RELA_SECTION_ALIGNMENT,
            sizeof(Elf_Rela));
    }

    // .data section
    new_section_header_table(
        ".data",
        base_section_data->type,
        base_section_data->flags,
        0,
        base_section_data->offset,
        base_section_data->size,
        SHN_UNDEF,
        DEFAULT_SECTION_INFO,
        base_section_data->alignment,
        0);

    // .bss section
    new_section_header_table(
        ".bss",
        base_section_bss->type,
        base_section_bss->flags,
        0,
        base_section_bss->offset,
        base_section_bss->size,
        SHN_UNDEF,
        DEFAULT_SECTION_INFO,
        base_section_bss->alignment,
        0);

    // .symtab section
    new_section_header_table(
        ".symtab",
        base_section_symtab->type,
        base_section_symtab->flags,
        0,
        base_section_symtab->offset,
        symtab_body.size,
        base_section_strtab->index, // sh_link holds section header index of the associated string table (i.e. .strtab section)
        RESERVED_SYMTAB_ENTRIES + get_length(Label)(local_label_list), // sh_info holds one greater than the symbol table index of the laxt local symbol
        base_section_symtab->alignment,
        sizeof(Elf_Sym));

    // .strtab section
    new_section_header_table(
        ".strtab",
        base_section_strtab->type,
        base_section_strtab->flags,
        0,
        base_section_strtab->offset,
        strtab_body.size,
        SHN_UNDEF,
        DEFAULT_SECTION_INFO,
        base_section_strtab->alignment,
        0);

    // .shstrtab section
    new_section_header_table(
        ".shstrtab",
        base_section_shstrtab->type,
        base_section_shstrtab->flags,
        0,
        base_section_shstrtab->offset,
        shstrtab_body.size,
        SHN_UNDEF,
        DEFAULT_SECTION_INFO,
        base_section_shstrtab->alignment,
        0);
}


/*
generate ELF header
*/
static void generate_elf_header(Elf_Ehdr *ehdr)
{
    BaseSection *base_section_shstrtab = get_base_section(SC_SHSTRTAB);
    Elf_Off e_shoff = align_to(base_section_shstrtab->offset + base_section_shstrtab->body->size, sizeof(Elf_Xword));
    Elf_Half e_shnum = base_section_shstrtab->index + 1;
    Elf_Half e_shstrndx = base_section_shstrtab->index;

    set_elf_header(
        e_shoff,
        e_shnum,
        e_shstrndx,
        ehdr
    );
}


/*
generate an object file
*/
void generate(const char *output_file, const Program *program)
{
    // initialize lists
    shdr_list = new_list(Elf_Shdr)();
    symbol_list = new_list(Symbol)();
    local_label_list = new_list(Label)();
    global_label_list = new_list(Label)();
    unresolved_symbol_list = new_list(Symbol)();
    reloc_info_list = new_list(RelocationInfo)();

    // generate contents
    generate_sections(program);
    generate_section_header_table_entries();

    // output an object file
    FILE *fp = fopen(output_file, "wb");

    // output ELF header
    Elf_Ehdr elf_header;
    generate_elf_header(&elf_header);
    fwrite(&elf_header, sizeof(Elf_Ehdr), 1, fp);

    // output sections
    size_t end_pos = sizeof(Elf_Ehdr);
    for_each_entry(BaseSection, cursor, base_section_list)
    {
        BaseSection *base_section = get_element(BaseSection)(cursor);
        if(base_section->kind != SC_SHSTRTAB)
        {
            end_pos = fwrite_section(base_section->body, base_section->offset, end_pos, fp);
        }
    }
    for_each_entry(BaseSection, cursor, base_section_list)
    {
        BaseSection *base_section = get_element(BaseSection)(cursor);
        end_pos = fwrite_section(base_section->rela_body, base_section->rela_offset, end_pos, fp);
    }
    {
        BaseSection *base_section = get_base_section(SC_SHSTRTAB);
        end_pos = fwrite_section(base_section->body, base_section->offset, end_pos, fp);
    }

    // output section header table entries
    fill_paddings(elf_header.e_shoff - end_pos, fp);
    for_each_entry(Elf_Shdr, cursor, shdr_list)
    {
        const Elf_Shdr *shdr = get_element(Elf_Shdr)(cursor);
        fwrite(shdr, sizeof(Elf_Shdr), 1, fp);
    }

    fclose(fp);
}
