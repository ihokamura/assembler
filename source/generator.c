#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "elf_wrap.h"
#include "generator.h"
#include "output.h"
#include "parser.h"
#include "processor.h"
#include "section.h"
#include "symbol.h"

typedef struct RelocationInfo RelocationInfo;

struct RelocationInfo
{
    SectionKind source;      // section of relocation source
    SectionKind destination; // section of relocation destination
    const char *body;        // label body
    Elf_Addr address;        // address to be replaced
    Elf_Sxword addend;       // addend
};

#include "list.h"
define_list(RelocationInfo)
define_list_operations(RelocationInfo)

static RelocationInfo *new_relocation_info(SectionKind source, SectionKind destination, const char *body, Elf_Addr address, Elf_Sxword addend);
static void set_elf_header
(
    Elf_Off e_shoff,
    Elf_Half e_shnum,
    Elf_Half e_shstrndx,
    Elf_Ehdr *ehdr
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
static Elf_Xword get_symtab_index(const RelocationInfo *reloc_info);
static void set_relocation_table_entries(void);
static void generate_statement_list(const List(Statement) *statement_list);
static void classify_symbol_list(const List(Symbol) *symbol_list, const List(Label) *label_list);
static void resolve_symbols(const List(Symbol) *symbol_list, const List(Label) *label_list);
static void generate_sections(const Program *program);
static void generate_elf_header(Elf_Ehdr *ehdr);

static const size_t RESERVED_SYMTAB_ENTRIES = 4; // number of reserved symbol table entries (undefined, .text, .data, .bss)
static const Elf_Xword SYMTAB_INDEX_DATA = 2; // index of symbol table entry for .data section
static const Elf_Xword SYMTAB_INDEX_BSS = 3;  // index of symbol table entry for .bss section

static List(Symbol) *local_symbol_list;       // list of local symbols
static List(Symbol) *global_symbol_list;      // list of global symbols
static List(RelocationInfo) *reloc_info_list; // list of relocatable symbols

static ByteBufferType symtab_body = {NULL, 0, 0};    // buffer for section ".symtab"
static ByteBufferType strtab_body = {NULL, 0, 0};    // buffer for string containing names of symbols
static ByteBufferType shstrtab_body = {NULL, 0, 0};  // buffer for string containing names of sections


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
        get_section(SC_TEXT)->index,
        0,
        0
    );

    // .data section
    set_symbol_table(
        0,
        ELF_ST_INFO(STB_LOCAL, STT_SECTION),
        0,
        get_section(SC_DATA)->index,
        0,
        0
    );

    // .bss section
    set_symbol_table(
        0,
        ELF_ST_INFO(STB_LOCAL, STT_SECTION),
        0,
        get_section(SC_BSS)->index,
        0,
        0
    );

    // put '\0' for invalid symbols at first
    append_bytes("\x00", 1, &strtab_body);
    Elf_Word st_name = 1;

    const List(Symbol) *symbol_lists[] = {local_symbol_list, global_symbol_list};
    const size_t size = sizeof(symbol_lists) / sizeof(symbol_lists[0]);
    for(size_t i = 0; i < size; i++)
    {
        const List(Symbol) *symbol_list = symbol_lists[i];
        for_each_entry(Symbol, cursor, symbol_list)
        {
            const Symbol *symbol = get_element(Symbol)(cursor);
            const char *body = symbol->body;
            set_symbol_table(
                st_name,
                ELF_ST_INFO(symbol->bind, STT_NOTYPE),
                0,
                get_section(symbol->destination)->index,
                symbol->value,
                0
            );
            append_bytes(body, strlen(body) + 1, &strtab_body);
            st_name += strlen(body) + 1;
        }
    }
}


/*
get index of symbol table entry for relocatable symbol
*/
static Elf_Xword get_symtab_index(const RelocationInfo *reloc_info)
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
        Elf_Xword sym_index = RESERVED_SYMTAB_ENTRIES + get_length(Symbol)(local_symbol_list);
        for_each_entry(Symbol, cursor, global_symbol_list)
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
static void set_relocation_table_entries(void)
{
    for_each_entry(RelocationInfo, cursor, reloc_info_list)
    {
        RelocationInfo *reloc_info = get_element(RelocationInfo)(cursor);
        Elf_Xword sym = get_symtab_index(reloc_info);
        Elf_Xword type = (reloc_info->source == SC_TEXT) ? R_X86_64_PC32 : R_X86_64_64;
        set_relocation_table(
            reloc_info->address,
            ELF_R_INFO(sym, type),
            reloc_info->addend,
            get_section(reloc_info->source)->rela_body
        );
    }
}


/*
update section
*/
static void update_section(Statement *statement, Section *section)
{
    size_t section_size = 0;
    statement->address = section->size;
    switch(statement->kind)
    {
    case ST_INSTRUCTION:
        {
        ByteBufferType *body = section->body;
        Operation *operation = statement->operation;
        generate_operation(operation, body);
        section_size = body->size;
        }
        break;

    case ST_VALUE:
        {
        ByteBufferType *body = section->body;
        Data *data = statement->data;
        generate_data(data, body);
        section_size = body->size;
        }
        break;

    case ST_ZERO:
        section_size = section->size + statement->bss->size;
        break;

    default:
        assert(0);
        break;
    }

    section->size = section_size;
}


/*
generate statement list
*/
static void generate_statement_list(const List(Statement) *statement_list)
{
    for_each_entry(Statement, cursor, statement_list)
    {
        Statement *statement = get_element(Statement)(cursor);
        Section *section = get_section(statement->section);
        update_section(statement, section);
    }
}


/*
classify list of symbols
*/
static void classify_symbol_list(const List(Symbol) *symbol_list, const List(Label) *label_list)
{
    for_each_entry(Symbol, cursor, symbol_list)
    {
        Symbol *symbol = get_element(Symbol)(cursor);
        const char *body = symbol->body;
        if((search_symbol(local_symbol_list, body) != NULL) || (search_symbol(global_symbol_list, body) != NULL))
        {
            continue;
        }

        if(!(symbol->labeled || symbol->declared))
        {
            if(search_label(label_list, symbol) == NULL)
            {
                symbol->bind = STB_GLOBAL;
                add_list_entry_tail(Symbol)(global_symbol_list, symbol);
            }
        }
        else
        {
            unsigned char bind = STB_GLOBAL;
            if(symbol->labeled)
            {
                bool found = false;
                for_each_entry(Symbol, cursor, symbol_list)
                {
                    Symbol *decl_symbol = get_element(Symbol)(cursor);
                    if(decl_symbol->declared && (strcmp(decl_symbol->body, body) == 0))
                    {
                        bind = decl_symbol->bind;
                        found = true;
                        break;
                    }
                }
                if(!found)
                {
                    bind = STB_LOCAL;
                }
            }
            else
            {
                bind = symbol->bind;
            }

            Label *label = search_label(label_list, symbol);
            assert(label != NULL);
            symbol->value = label->statement->address;
            symbol->destination = label->statement->section;

            List(Symbol) *list = (bind == STB_LOCAL) ? local_symbol_list : global_symbol_list;
            add_list_entry_tail(Symbol)(list, symbol);
        }
    }
}


/*
resolve symbols
*/
static void resolve_symbols(const List(Symbol) *symbol_list, const List(Label) *label_list)
{
    for_each_entry(Symbol, symbol_cursor, symbol_list)
    {
        Symbol *symbol = get_element(Symbol)(symbol_cursor);
        if(symbol->labeled || symbol->declared)
        {
            continue;
        }

        for_each_entry(Label, label_cursor, label_list)
        {
            Label *label = get_element(Label)(label_cursor);
            if(strcmp(symbol->body, label->symbol->body) == 0)
            {
                switch(label->statement->section)
                {
                case SC_TEXT:
                    if(search_symbol(global_symbol_list, symbol->body) != NULL)
                    {
                        new_relocation_info(symbol->section, label->statement->section, symbol->body, symbol->address, -sizeof(uint32_t));
                    }
                    else
                    {
                        *(uint32_t *)&get_section(SC_TEXT)->body->body[symbol->address] = label->statement->address - (symbol->address + sizeof(uint32_t));
                    }
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
            new_relocation_info(symbol->section, SC_UND, symbol->body, symbol->address, symbol->addend);
        }
    }
}


/*
generate section bodies
*/
static void generate_sections(const Program *program)
{
    generate_statement_list(program->statement_list);
    classify_symbol_list(program->symbol_list, program->label_list);
    resolve_symbols(program->symbol_list, program->label_list);
    set_relocation_table_entries();
    make_shstrtab(&shstrtab_body);
    make_metadata_sections(&symtab_body, &strtab_body, &shstrtab_body);
    set_symbol_table_entries();
    set_offset_of_sections();
    generate_section_header_table_entries(RESERVED_SYMTAB_ENTRIES + get_length(Symbol)(local_symbol_list));
}


/*
generate ELF header
*/
static void generate_elf_header(Elf_Ehdr *ehdr)
{
    Section *section_shstrtab = get_section(SC_SHSTRTAB);
    Elf_Off e_shoff = align_to(section_shstrtab->offset + section_shstrtab->body->size, sizeof(Elf_Xword));
    Elf_Half e_shnum = section_shstrtab->index + 1;
    Elf_Half e_shstrndx = section_shstrtab->index;

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
    local_symbol_list = new_list(Symbol)();
    global_symbol_list = new_list(Symbol)();
    reloc_info_list = new_list(RelocationInfo)();

    // generate contents
    generate_sections(program);

    // output an object file
    FILE *fp = fopen(output_file, "wb");

    // output ELF header
    Elf_Ehdr elf_header;
    generate_elf_header(&elf_header);
    fwrite(&elf_header, sizeof(Elf_Ehdr), 1, fp);

    // output section bodies
    size_t end_pos = output_section_bodies(sizeof(Elf_Ehdr), fp);

    // output section header table entries
    fill_paddings(elf_header.e_shoff - end_pos, fp);
    output_section_header_table_entries(fp);

    fclose(fp);
}
