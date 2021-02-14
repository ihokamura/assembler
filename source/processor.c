#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "buffer.h"
#include "processor.h"
#include "section.h"
#include "symbol.h"

#define min(a, b)    ((a) < (b) ? (a) : (b))

typedef enum ConditionCode ConditionCode;
typedef struct BinaryOperationOpecode BinaryOperationOpecode;

enum ConditionCode
{
    CC_B   = 0x02,
    CC_BE  = 0x06,
    CC_E   = 0x04,
    CC_G   = 0x0f,
    CC_L   = 0x0c,
    CC_LE  = 0x0e,
    CC_NB  = 0x03,
    CC_NBE = 0x07,
    CC_NE  = 0x05,
    CC_NL  = 0x0d,
};

struct BinaryOperationOpecode
{
    uint8_t i_byte;       // encoding of type I for 8-bit operands
    uint8_t i;            // encoding of type I
    uint8_t reg_field_mi; // reg field for encoding of type MI
    uint8_t mi_byte;      // encoding of type MI for 8-bit operands
    uint8_t mi;           // encoding of type MI
    uint8_t mi_imm8;      // encoding of type MI for 8-bit immediate operand
    uint8_t mr_byte;      // encoding of type MR for 8-bit operands
    uint8_t mr;           // encoding of type MR
    uint8_t rm_byte;      // encoding of type RM for 8-bit operands
    uint8_t rm;           // encoding of type RM
};

static void generate_op_add(const List(Operand) *operands, ByteBufferType *buffer);
static void generate_op_and(const List(Operand) *operands, ByteBufferType *buffer);
static void generate_op_call(const List(Operand) *operands, ByteBufferType *buffer);
static void generate_op_cmp(const List(Operand) *operands, ByteBufferType *buffer);
static void generate_op_lea(const List(Operand) *operands, ByteBufferType *buffer);
static void generate_op_mov(const List(Operand) *operands, ByteBufferType *buffer);
static void generate_op_nop(const List(Operand) *operands, ByteBufferType *buffer);
static void generate_op_or(const List(Operand) *operands, ByteBufferType *buffer);
static void generate_op_pop(const List(Operand) *operands, ByteBufferType *buffer);
static void generate_op_push(const List(Operand) *operands, ByteBufferType *buffer);
static void generate_op_pushfq(const List(Operand) *operands, ByteBufferType *buffer);
static void generate_op_ret(const List(Operand) *operands, ByteBufferType *buffer);
static void generate_op_setb(const List(Operand) *operands, ByteBufferType *buffer);
static void generate_op_setbe(const List(Operand) *operands, ByteBufferType *buffer);
static void generate_op_sete(const List(Operand) *operands, ByteBufferType *buffer);
static void generate_op_setg(const List(Operand) *operands, ByteBufferType *buffer);
static void generate_op_setl(const List(Operand) *operands, ByteBufferType *buffer);
static void generate_op_setle(const List(Operand) *operands, ByteBufferType *buffer);
static void generate_op_setnb(const List(Operand) *operands, ByteBufferType *buffer);
static void generate_op_setnbe(const List(Operand) *operands, ByteBufferType *buffer);
static void generate_op_setne(const List(Operand) *operands, ByteBufferType *buffer);
static void generate_op_setnl(const List(Operand) *operands, ByteBufferType *buffer);
static void generate_op_sub(const List(Operand) *operands, ByteBufferType *buffer);
static void generate_op_xor(const List(Operand) *operands, ByteBufferType *buffer);
static void generate_binary_arithmetic_operation(const BinaryOperationOpecode *opecode, const List(Operand) *operands, ByteBufferType *buffer);
static void generate_op_setcc(const List(Operand) *operands, uint32_t opecode, ByteBufferType *buffer);
static bool is_immediate(OperandKind kind);
static bool is_register(OperandKind kind);
static bool is_memory(OperandKind kind);
static bool is_register_or_memory(OperandKind kind);
static bool is_eax_register(RegisterKind kind);
static bool is_type_i_encoding(const Operand *operand1, const Operand *operand2);
static bool is_signed_8bit_immediate(const Operand *operand);
static size_t get_operand_size(OperandKind kind);
static uint8_t get_register_index(RegisterKind kind);
static uint8_t get_rex_prefix(const Operand *operand, size_t prefix_position, bool specify_size);
static uint8_t get_rex_prefix_for_size(const Operand *operand);
static uint8_t get_rex_prefix_from_position(size_t prefix_position);
static uint8_t get_modrm_byte(uint8_t mod, uint8_t reg, uint8_t rm);
static uint8_t get_sib_byte(uint8_t ss, uint8_t index, uint8_t base);
static uint8_t get_mod_field(const Operand *operand);
static uint8_t get_reg_field(RegisterKind kind);
static uint8_t get_rm_field(RegisterKind kind);
static void append_binary_prefix(uint8_t prefix, ByteBufferType *buffer);
static void append_binary_opecode(uint32_t opecode, ByteBufferType *buffer);
static void append_binary_modrm(uint8_t mod, uint8_t reg, uint8_t rm, ByteBufferType *buffer);
static void append_binary_sib(uint8_t ss, uint8_t index, uint8_t base, ByteBufferType *buffer);
static void append_binary_disp(const Operand *operand, Elf_Addr address, Elf_Sxword addend, ByteBufferType *buffer);
static void append_binary_imm(uintmax_t imm, size_t size, ByteBufferType *buffer);
static void append_binary_imm_least(uintmax_t imm, ByteBufferType *buffer);
static void append_binary_imm32(uint32_t imm32, ByteBufferType *buffer);
static void append_binary_relocation(size_t size, Symbol *symbol, Elf_Addr address, Elf_Sxword addend, ByteBufferType *buffer);
static void may_append_binary_instruction_prefix(OperandKind kind, uint8_t prefix, ByteBufferType *buffer);
static void may_append_binary_rex_prefix_reg_rm(const Operand *operand_reg, const Operand *operand_rm, bool specify_size, ByteBufferType *buffer);
static void may_append_binary_rex_prefix_reg(const Operand *operand, bool specify_size, ByteBufferType *buffer);

const MnemonicInfo mnemonic_info_list[] = 
{
    {MN_ADD,    "add",    true,  generate_op_add},
    {MN_AND,    "and",    true,  generate_op_and},
    {MN_CALL,   "call",   true,  generate_op_call},
    {MN_CMP,    "cmp",    true,  generate_op_cmp},
    {MN_LEA,    "lea",    true,  generate_op_lea},
    {MN_MOV,    "mov",    true,  generate_op_mov},
    {MN_NOP,    "nop",    false, generate_op_nop},
    {MN_OR,     "or",     true,  generate_op_or},
    {MN_POP,    "pop",    true,  generate_op_pop},
    {MN_PUSH,   "push",   true,  generate_op_push},
    {MN_PUSHFQ, "pushfq", false, generate_op_pushfq},
    {MN_RET,    "ret",    false, generate_op_ret},
    {MN_SETA,   "seta",   true,  generate_op_setnbe},
    {MN_SETAE,  "setae",  true,  generate_op_setnb},
    {MN_SETB,   "setb",   true,  generate_op_setb},
    {MN_SETBE,  "setbe",  true,  generate_op_setbe},
    {MN_SETE,   "sete",   true,  generate_op_sete},
    {MN_SETG,   "setg",   true,  generate_op_setg},
    {MN_SETGE,  "setge",  true,  generate_op_setnl},
    {MN_SETL,   "setl",   true,  generate_op_setl},
    {MN_SETLE,  "setle",  true,  generate_op_setle},
    {MN_SETNA,  "setna",  true,  generate_op_setbe},
    {MN_SETNAE, "setnae", true,  generate_op_setb},
    {MN_SETNB,  "setnb",  true,  generate_op_setnb},
    {MN_SETNBE, "setnbe", true,  generate_op_setnbe},
    {MN_SETNE,  "setne",  true,  generate_op_setne},
    {MN_SETNG,  "setng",  true,  generate_op_setle},
    {MN_SETNGE, "setnge", true,  generate_op_setl},
    {MN_SETNL,  "setnl",  true,  generate_op_setnl},
    {MN_SUB,    "sub",    true,  generate_op_sub},
    {MN_XOR,    "xor",    true,  generate_op_xor},
};
const size_t MNEMONIC_INFO_LIST_SIZE = sizeof(mnemonic_info_list) / sizeof(mnemonic_info_list[0]);

const RegisterInfo register_info_list[] = 
{
    {REG_AL,   "al",   OP_R8},
    {REG_CL,   "cl",   OP_R8},
    {REG_DL,   "dl",   OP_R8},
    {REG_BL,   "bl",   OP_R8},
    {REG_SPL,  "spl",  OP_R8},
    {REG_BPL,  "bpl",  OP_R8},
    {REG_SIL,  "sil",  OP_R8},
    {REG_DIL,  "dil",  OP_R8},
    {REG_R8B,  "r8b",  OP_R8},
    {REG_R9B,  "r9b",  OP_R8},
    {REG_R10B, "r10b", OP_R8},
    {REG_R11B, "r11b", OP_R8},
    {REG_R12B, "r12b", OP_R8},
    {REG_R13B, "r13b", OP_R8},
    {REG_R14B, "r14b", OP_R8},
    {REG_R15B, "r15b", OP_R8},
    {REG_AX,   "ax",   OP_R16},
    {REG_CX,   "cx",   OP_R16},
    {REG_DX,   "dx",   OP_R16},
    {REG_BX,   "bx",   OP_R16},
    {REG_SP,   "sp",   OP_R16},
    {REG_BP,   "bp",   OP_R16},
    {REG_SI,   "si",   OP_R16},
    {REG_DI,   "di",   OP_R16},
    {REG_R8W,  "r8w",  OP_R16},
    {REG_R9W,  "r9w",  OP_R16},
    {REG_R10W, "r10w", OP_R16},
    {REG_R11W, "r11w", OP_R16},
    {REG_R12W, "r12w", OP_R16},
    {REG_R13W, "r13w", OP_R16},
    {REG_R14W, "r14w", OP_R16},
    {REG_R15W, "r15w", OP_R16},
    {REG_EAX,  "eax",  OP_R32},
    {REG_ECX,  "ecx",  OP_R32},
    {REG_EDX,  "edx",  OP_R32},
    {REG_EBX,  "ebx",  OP_R32},
    {REG_ESP,  "esp",  OP_R32},
    {REG_EBP,  "ebp",  OP_R32},
    {REG_ESI,  "esi",  OP_R32},
    {REG_EDI,  "edi",  OP_R32},
    {REG_R8D,  "r8d",  OP_R32},
    {REG_R9D,  "r9d",  OP_R32},
    {REG_R10D, "r10d", OP_R32},
    {REG_R11D, "r11d", OP_R32},
    {REG_R12D, "r12d", OP_R32},
    {REG_R13D, "r13d", OP_R32},
    {REG_R14D, "r14d", OP_R32},
    {REG_R15D, "r15d", OP_R32},
    {REG_RAX,  "rax",  OP_R64},
    {REG_RCX,  "rcx",  OP_R64},
    {REG_RDX,  "rdx",  OP_R64},
    {REG_RBX,  "rbx",  OP_R64},
    {REG_RSP,  "rsp",  OP_R64},
    {REG_RBP,  "rbp",  OP_R64},
    {REG_RSI,  "rsi",  OP_R64},
    {REG_RDI,  "rdi",  OP_R64},
    {REG_R8,   "r8",   OP_R64},
    {REG_R9,   "r9",   OP_R64},
    {REG_R10,  "r10",  OP_R64},
    {REG_R11,  "r11",  OP_R64},
    {REG_R12,  "r12",  OP_R64},
    {REG_R13,  "r13",  OP_R64},
    {REG_R14,  "r14",  OP_R64},
    {REG_R15,  "r15",  OP_R64},
    {REG_RIP,  "rip",  OP_R64},
};
const size_t REGISTER_INFO_LIST_SIZE = sizeof(register_info_list) / sizeof(register_info_list[0]);

static const uint8_t PREFIX_OPERAND_SIZE_OVERRIDE = 0x66;

static const uint8_t PREFIX_NONE = 0x00;
static const uint8_t PREFIX_REX = 0x40;
static const size_t PREFIX_POSITION_REX_W = 3;
static const size_t PREFIX_POSITION_REX_R = 2;
static const size_t PREFIX_POSITION_REX_B = 0;

static const uint8_t MOD_MEM = 0;
static const uint8_t MOD_MEM_DISP8 = 1;
static const uint8_t MOD_MEM_DISP32 = 2;
static const uint8_t MOD_REG = 3;
static const uint8_t MOD_INVALID = 0xff;

static const uint8_t REGISTER_INDEX_EAX = 0;
static const uint8_t REGISTER_INDEX_ECX = 1;
static const uint8_t REGISTER_INDEX_EDX = 2;
static const uint8_t REGISTER_INDEX_EBX = 3;
static const uint8_t REGISTER_INDEX_ESP = 4;
static const uint8_t REGISTER_INDEX_EBP = 5;
static const uint8_t REGISTER_INDEX_ESI = 6;
static const uint8_t REGISTER_INDEX_EDI = 7;
static const uint8_t REGISTER_INDEX_R8D = 8;
static const uint8_t REGISTER_INDEX_R9D = 9;
static const uint8_t REGISTER_INDEX_R10D = 10;
static const uint8_t REGISTER_INDEX_R11D = 11;
static const uint8_t REGISTER_INDEX_R12D = 12;
static const uint8_t REGISTER_INDEX_R13D = 13;
static const uint8_t REGISTER_INDEX_R14D = 14;
static const uint8_t REGISTER_INDEX_R15D = 15;
static const uint8_t REGISTER_INDEX_INVALID = 0xff;
static const uint8_t REG_FIELD_MASK = 0x07;

static const size_t MODRM_POSITION_MOD = 6;
static const size_t MODRM_POSITION_REG = 3;
static const size_t MODRM_POSITION_RM = 0;

static const size_t SIB_POSITION_SS = 6;
static const size_t SIB_POSITION_INDEX = 3;
static const size_t SIB_POSITION_BASE = 0;

static const size_t OPECODE_SIZE_MAX = 3;
static const uint8_t UINT8_T_MASK = 0xff;
static const size_t SIGN_BIT_MASK_8BIT = 0x80;

static const uint32_t OPECODE_SET_CC = 0x0f90;


/*
generate data
*/
void generate_data(const Data *data, ByteBufferType *buffer)
{
    if(data->kind == DT_SYMBOL)
    {
        set_symbol(buffer->size, 0, SC_DATA, data->symbol);
    }
    append_binary_imm(data->value, data->size, buffer);
}


/*
generate an operation
*/
void generate_operation(const Operation *operation, ByteBufferType *buffer)
{
    mnemonic_info_list[operation->kind].generate_function(operation->operands, buffer);
}


/*
generate add operation
*/
static void generate_op_add(const List(Operand) *operands, ByteBufferType *buffer)
{
    const BinaryOperationOpecode opecode = {0x04, 0x05, 0x00, 0x80, 0x81, 0x83, 0x00, 0x01, 0x02, 0x03};
    generate_binary_arithmetic_operation(&opecode, operands, buffer);
}


/*
generate and operation
*/
static void generate_op_and(const List(Operand) *operands, ByteBufferType *buffer)
{
    const BinaryOperationOpecode opecode = {0x24, 0x25, 0x04, 0x80, 0x81, 0x83, 0x20, 0x21, 0x22, 0x23};
    generate_binary_arithmetic_operation(&opecode, operands, buffer);
}


/*
generate call operation
*/
static void generate_op_call(const List(Operand) *operands, ByteBufferType *buffer)
{
    Operand *operand = get_first_element(Operand)(operands);
    if(operand->kind == OP_SYMBOL)
    {
        /*
        handle the following instructions
        * CALL rel32
        */
        append_binary_opecode(0xe8, buffer);
        append_binary_relocation(SIZEOF_32BIT, operand->symbol, buffer->size, -SIZEOF_32BIT, buffer);
    }
}


/*
generate cmp operation
*/
static void generate_op_cmp(const List(Operand) *operands, ByteBufferType *buffer)
{
    const BinaryOperationOpecode opecode = {0x3c, 0x3d, 0x07, 0x80, 0x81, 0x83, 0x38, 0x39, 0x3a, 0x3b};
    generate_binary_arithmetic_operation(&opecode, operands, buffer);
}


/*
generate lea operation
*/
static void generate_op_lea(const List(Operand) *operands, ByteBufferType *buffer)
{
    ListEntry(Operand) *entry = get_first_entry(Operand)(operands);
    Operand *operand1 = get_element(Operand)(entry);
    Operand *operand2 = get_element(Operand)(next_entry(Operand, entry));

    /*
    handle the following instructions
    * LEA r64, m
    */
    may_append_binary_rex_prefix_reg_rm(operand1, operand2, true, buffer);
    append_binary_opecode(0x8d, buffer);
    append_binary_modrm(get_mod_field(operand2), get_reg_field(operand1->reg), get_rm_field(operand2->reg), buffer);
    append_binary_disp(operand2, buffer->size, operand2->immediate - SIZEOF_32BIT, buffer);
}


/*
generate mov operation
*/
static void generate_op_mov(const List(Operand) *operands, ByteBufferType *buffer)
{
    ListEntry(Operand) *entry = get_first_entry(Operand)(operands);
    Operand *operand1 = get_element(Operand)(entry);
    Operand *operand2 = get_element(Operand)(next_entry(Operand, entry));

    if(is_register_or_memory(operand1->kind) && is_register(operand2->kind))
    {
        /*
        handle the following instructions
        * MOV r/m8,r8
        * MOV r/m16,r16
        * MOV r/m32,r32
        * MOV r/m64,r64
        */
        assert(get_operand_size(operand1->kind) == get_operand_size(operand2->kind));
        may_append_binary_instruction_prefix(operand1->kind, PREFIX_OPERAND_SIZE_OVERRIDE, buffer);
        may_append_binary_rex_prefix_reg_rm(operand2, operand1, true, buffer);
        uint32_t opecode = (get_operand_size(operand1->kind) == SIZEOF_8BIT) ? 0x88 : 0x89;
        append_binary_opecode(opecode, buffer);
        append_binary_modrm(get_mod_field(operand1), get_reg_field(operand2->reg), get_rm_field(operand1->reg), buffer);
        if(is_memory(operand1->kind))
        {
            append_binary_disp(operand1, buffer->size, -SIZEOF_32BIT, buffer);
        }
    }
    else if(is_register(operand1->kind) && is_memory(operand2->kind))
    {
        /*
        handle the following instructions
        * MOV r8,m8
        * MOV r16,m16
        * MOV r32,m32
        * MOV r64,m64
        */
        assert(get_operand_size(operand1->kind) == get_operand_size(operand2->kind));
        may_append_binary_instruction_prefix(operand1->kind, PREFIX_OPERAND_SIZE_OVERRIDE, buffer);
        may_append_binary_rex_prefix_reg_rm(operand1, operand2, true, buffer);
        uint32_t opecode = (get_operand_size(operand1->kind) == SIZEOF_8BIT) ? 0x8a : 0x8b;
        append_binary_opecode(opecode, buffer);
        append_binary_modrm(get_mod_field(operand2), get_reg_field(operand1->reg), get_rm_field(operand2->reg), buffer);
        append_binary_disp(operand2, buffer->size, operand2->immediate - SIZEOF_32BIT, buffer);
    }
    else if(is_register(operand1->kind) && is_immediate(operand2->kind))
    {
        if((get_operand_size(operand1->kind) == SIZEOF_64BIT) && (get_operand_size(operand2->kind) < SIZEOF_64BIT))
        {
            /*
            handle the following instructions
            * MOV r64, imm32
            */
            append_binary_prefix(get_rex_prefix(operand1, PREFIX_POSITION_REX_B, true), buffer);
            append_binary_opecode(0xc7, buffer);
            append_binary_modrm(MOD_REG, 0x00, get_rm_field(operand1->reg), buffer);
            append_binary_imm32(operand2->immediate, buffer);
        }
        else
        {
            /*
            handle the following instructions
            * MOV r8, imm8
            * MOV r16, imm16
            * MOV r32, imm32
            * MOV r64, imm64
            */
            assert(get_operand_size(operand1->kind) >= get_operand_size(operand2->kind));
            may_append_binary_instruction_prefix(operand1->kind, PREFIX_OPERAND_SIZE_OVERRIDE, buffer);
            may_append_binary_rex_prefix_reg(operand1, true, buffer);
            uint32_t opecode = (get_operand_size(operand1->kind) == SIZEOF_8BIT) ? 0xb0 : 0xb8;
            append_binary_opecode(opecode + get_reg_field(operand1->reg), buffer);
            append_binary_imm(operand2->immediate, get_operand_size(operand1->kind), buffer);
        }
    }
    else if(is_memory(operand1->kind) && is_immediate(operand2->kind))
    {
        /*
        handle the following instructions
        * MOV m8, imm8
        * MOV m16, imm16
        * MOV m32, imm32
        * MOV m64, imm32
        */
        assert(get_operand_size(operand1->kind) >= get_operand_size(operand2->kind));
        may_append_binary_instruction_prefix(operand1->kind, PREFIX_OPERAND_SIZE_OVERRIDE, buffer);
        may_append_binary_rex_prefix_reg_rm(operand2, operand1, true, buffer);
        uint32_t opecode = (get_operand_size(operand1->kind) == SIZEOF_8BIT) ? 0xc6 : 0xc7;
        append_binary_opecode(opecode, buffer);
        append_binary_modrm(get_mod_field(operand1), 0x00, get_rm_field(operand1->reg), buffer);
        size_t imm_size = min(get_operand_size(operand1->kind), SIZEOF_32BIT);
        append_binary_disp(operand1, buffer->size, -(SIZEOF_32BIT + imm_size), buffer);
        append_binary_imm(operand2->immediate, imm_size, buffer);
    }
}


/*
generate nop operation
*/
static void generate_op_nop(const List(Operand) *operands, ByteBufferType *buffer)
{
    /*
    handle the following instructions
    * NOP
    */
    append_binary_opecode(0x90, buffer);
}


/*
generate or operation
*/
static void generate_op_or(const List(Operand) *operands, ByteBufferType *buffer)
{
    const BinaryOperationOpecode opecode = {0x0c, 0x0d, 0x01, 0x80, 0x81, 0x83, 0x08, 0x09, 0x0a, 0x0b};
    generate_binary_arithmetic_operation(&opecode, operands, buffer);
}


/*
generate pop operation
*/
static void generate_op_pop(const List(Operand) *operands, ByteBufferType *buffer)
{
    const Operand *operand = get_first_element(Operand)(operands);

    if(is_register(operand->kind))
    {
        /*
        handle the following instructions
        * POP r16
        * POP r64
        */
        may_append_binary_instruction_prefix(operand->kind, PREFIX_OPERAND_SIZE_OVERRIDE, buffer);
        may_append_binary_rex_prefix_reg(operand, false, buffer);
        append_binary_opecode(0x58 + get_reg_field(operand->reg), buffer);
    }
    else if(is_memory(operand->kind))
    {
        /*
        handle the following instructions
        * POP m16
        * POP m64
        */
        may_append_binary_instruction_prefix(operand->kind, PREFIX_OPERAND_SIZE_OVERRIDE, buffer);
        may_append_binary_rex_prefix_reg(operand, false, buffer);
        append_binary_opecode(0x8f, buffer);
        append_binary_modrm(get_mod_field(operand), 0x00, get_rm_field(operand->reg), buffer);
        append_binary_disp(operand, buffer->size, -SIZEOF_32BIT, buffer);
    }
}


/*
generate push operation
*/
static void generate_op_push(const List(Operand) *operands, ByteBufferType *buffer)
{
    const Operand *operand = get_first_element(Operand)(operands);

    if(is_register(operand->kind))
    {
        /*
        handle the following instructions
        * PUSH r16
        * PUSH r64
        */
        may_append_binary_instruction_prefix(operand->kind, PREFIX_OPERAND_SIZE_OVERRIDE, buffer);
        may_append_binary_rex_prefix_reg(operand, false, buffer);
        append_binary_opecode(0x50 + get_reg_field(operand->reg), buffer);
    }
    else if(is_memory(operand->kind))
    {
        /*
        handle the following instructions
        * PUSH m16
        * PUSH m64
        */
        may_append_binary_instruction_prefix(operand->kind, PREFIX_OPERAND_SIZE_OVERRIDE, buffer);
        may_append_binary_rex_prefix_reg(operand, false, buffer);
        append_binary_opecode(0xff, buffer);
        append_binary_modrm(get_mod_field(operand), 0x06, get_rm_field(operand->reg), buffer);
        append_binary_disp(operand, buffer->size, -SIZEOF_32BIT, buffer);
    }
    else if(is_immediate(operand->kind))
    {
        /*
        handle the following instructions
        * PUSH imm8
        * PUSH imm16
        * PUSH imm32
        */
        if(get_operand_size(operand->kind) == SIZEOF_8BIT)
        {
            append_binary_opecode(0x6a, buffer);
            append_binary_imm(operand->immediate, SIZEOF_8BIT, buffer);
        }
        else
        {
            append_binary_opecode(0x68, buffer);
            append_binary_imm32(operand->immediate, buffer);
        }
    }
}


/*
generate pushfq operation
*/
static void generate_op_pushfq(const List(Operand) *operands, ByteBufferType *buffer)
{
    append_binary_opecode(0x9c, buffer);
}


/*
generate ret operation
*/
static void generate_op_ret(const List(Operand) *operands, ByteBufferType *buffer)
{
    /*
    handle the following instructions
    * RET
    */
    append_binary_opecode(0xc3, buffer);
}


/*
generate setb operation
*/
static void generate_op_setb(const List(Operand) *operands, ByteBufferType *buffer)
{
    generate_op_setcc(operands, OPECODE_SET_CC + CC_B, buffer);
}


/*
generate setbe operation
*/
static void generate_op_setbe(const List(Operand) *operands, ByteBufferType *buffer)
{
    generate_op_setcc(operands, OPECODE_SET_CC + CC_BE, buffer);
}


/*
generate sete operation
*/
static void generate_op_sete(const List(Operand) *operands, ByteBufferType *buffer)
{
    generate_op_setcc(operands, OPECODE_SET_CC + CC_E, buffer);
}


/*
generate setg operation
*/
static void generate_op_setg(const List(Operand) *operands, ByteBufferType *buffer)
{
    generate_op_setcc(operands, OPECODE_SET_CC + CC_G, buffer);
}


/*
generate setl operation
*/
static void generate_op_setl(const List(Operand) *operands, ByteBufferType *buffer)
{
    generate_op_setcc(operands, OPECODE_SET_CC + CC_L, buffer);
}


/*
generate setle operation
*/
static void generate_op_setle(const List(Operand) *operands, ByteBufferType *buffer)
{
    generate_op_setcc(operands, OPECODE_SET_CC + CC_LE, buffer);
}


/*
generate setnb operation
*/
static void generate_op_setnb(const List(Operand) *operands, ByteBufferType *buffer)
{
    generate_op_setcc(operands, OPECODE_SET_CC + CC_NB, buffer);
}


/*
generate setnbe operation
*/
static void generate_op_setnbe(const List(Operand) *operands, ByteBufferType *buffer)
{
    generate_op_setcc(operands, OPECODE_SET_CC + CC_NBE, buffer);
}


/*
generate setne operation
*/
static void generate_op_setne(const List(Operand) *operands, ByteBufferType *buffer)
{
    generate_op_setcc(operands, OPECODE_SET_CC + CC_NE, buffer);
}


/*
generate setnl operation
*/
static void generate_op_setnl(const List(Operand) *operands, ByteBufferType *buffer)
{
    generate_op_setcc(operands, OPECODE_SET_CC + CC_NL, buffer);
}


/*
generate sub operation
*/
static void generate_op_sub(const List(Operand) *operands, ByteBufferType *buffer)
{
    const BinaryOperationOpecode opecode = {0x2c, 0x2d, 0x05, 0x80, 0x81, 0x83, 0x28, 0x29, 0x2a, 0x2b};
    generate_binary_arithmetic_operation(&opecode, operands, buffer);
}


/*
generate xor operation
*/
static void generate_op_xor(const List(Operand) *operands, ByteBufferType *buffer)
{
    const BinaryOperationOpecode opecode = {0x34, 0x35, 0x06, 0x80, 0x81, 0x83, 0x30, 0x31, 0x32, 0x33};
    generate_binary_arithmetic_operation(&opecode, operands, buffer);
}


/*
generate binary arithmetic operation
*/
static void generate_binary_arithmetic_operation(const BinaryOperationOpecode *opecode, const List(Operand) *operands, ByteBufferType *buffer)
{
    ListEntry(Operand) *entry = get_first_entry(Operand)(operands);
    Operand *operand1 = get_element(Operand)(entry);
    Operand *operand2 = get_element(Operand)(next_entry(Operand, entry));

    if(is_type_i_encoding(operand1, operand2))
    {
        /*
        handle the following instructions
        * <mnemonic> al, imm8
        * <mnemonic> ax, imm16
        * <mnemonic> eax, imm32
        * <mnemonic> rax, imm32
        */
        may_append_binary_instruction_prefix(operand1->kind, PREFIX_OPERAND_SIZE_OVERRIDE, buffer);
        may_append_binary_rex_prefix_reg(operand1, true, buffer);
        uint32_t op = (get_operand_size(operand1->kind) == SIZEOF_8BIT) ? opecode->i_byte : opecode->i;
        append_binary_opecode(op, buffer);
        append_binary_imm(operand2->immediate, get_operand_size(operand2->kind), buffer);
    }
    else if(is_register_or_memory(operand1->kind) && is_immediate(operand2->kind))
    {
        /*
        handle the following instructions
        * <mnemonic> r/m8, imm8
        * <mnemonic> r/m16, imm8
        * <mnemonic> r/m32, imm8
        * <mnemonic> r/m64, imm8
        * <mnemonic> r/m16, imm16
        * <mnemonic> r/m32, imm32
        * <mnemonic> r/m64, imm32
        */
        size_t operand1_size = get_operand_size(operand1->kind);
        size_t operand2_size = get_operand_size(operand2->kind);
        size_t imm_size = is_signed_8bit_immediate(operand2) ? min(operand1_size, SIZEOF_32BIT) : operand2_size;
        assert(operand1_size >= operand2_size);
        may_append_binary_instruction_prefix(operand1->kind, PREFIX_OPERAND_SIZE_OVERRIDE, buffer);
        may_append_binary_rex_prefix_reg_rm(operand2, operand1, true, buffer);
        uint32_t op = (operand1_size == SIZEOF_8BIT) ? opecode->mi_byte : (imm_size == SIZEOF_8BIT ? opecode->mi_imm8 : opecode->mi);
        append_binary_opecode(op, buffer);
        append_binary_modrm(get_mod_field(operand1), opecode->reg_field_mi, get_reg_field(operand1->reg), buffer);
        if(is_memory(operand1->kind))
        {
            append_binary_disp(operand1, buffer->size, -SIZEOF_32BIT, buffer);
        }
        append_binary_imm(operand2->immediate, imm_size, buffer);
    }
    else if(is_register_or_memory(operand1->kind) && is_register(operand2->kind))
    {
        /*
        handle the following instructions
        * <mnemonic> r/m8, r8
        * <mnemonic> r/m16, r16
        * <mnemonic> r/m32, r32
        * <mnemonic> r/m64, r64
        */
        assert(get_operand_size(operand1->kind) == get_operand_size(operand2->kind));
        may_append_binary_instruction_prefix(operand1->kind, PREFIX_OPERAND_SIZE_OVERRIDE, buffer);
        may_append_binary_rex_prefix_reg_rm(operand2, operand1, true, buffer);
        uint32_t op = (get_operand_size(operand1->kind) == SIZEOF_8BIT) ? opecode->mr_byte : opecode->mr;
        append_binary_opecode(op, buffer);
        append_binary_modrm(get_mod_field(operand1), get_reg_field(operand2->reg), get_rm_field(operand1->reg), buffer);
        if(is_memory(operand1->kind))
        {
            append_binary_disp(operand1, buffer->size, -SIZEOF_32BIT, buffer);
        }
    }
    else if(is_register(operand1->kind) && is_memory(operand2->kind))
    {
        /*
        handle the following instructions
        * <mnemonic> r8, m8
        * <mnemonic> r16, m16
        * <mnemonic> r32, m32
        * <mnemonic> r64, m64
        */
        assert(get_operand_size(operand1->kind) == get_operand_size(operand2->kind));
        may_append_binary_instruction_prefix(operand1->kind, PREFIX_OPERAND_SIZE_OVERRIDE, buffer);
        may_append_binary_rex_prefix_reg_rm(operand1, operand2, true, buffer);
        uint32_t op = (get_operand_size(operand1->kind) == SIZEOF_8BIT) ? opecode->rm_byte : opecode->rm;
        append_binary_opecode(op, buffer);
        append_binary_modrm(get_mod_field(operand2), get_reg_field(operand1->reg), get_rm_field(operand2->reg), buffer);
        append_binary_disp(operand2, buffer->size, -SIZEOF_32BIT, buffer);
    }
}


/*
generate setcc operation
*/
static void generate_op_setcc(const List(Operand) *operands, uint32_t opecode, ByteBufferType *buffer)
{
    const Operand *operand = get_first_element(Operand)(operands);

    may_append_binary_rex_prefix_reg(operand, true, buffer);
    append_binary_opecode(opecode, buffer);
    append_binary_modrm(get_mod_field(operand), 0x00, get_rm_field(operand->reg), buffer); // reg field of the ModR/M byte is not used
    append_binary_disp(operand, buffer->size, -SIZEOF_32BIT, buffer);
}


/*
check if operand is immediate
*/
static bool is_immediate(OperandKind kind)
{
    return (kind == OP_IMM8) || (kind == OP_IMM16) || (kind == OP_IMM32) || (kind == OP_IMM64);
}


/*
check if operand is register
*/
static bool is_register(OperandKind kind)
{
    return (kind == OP_R8) || (kind == OP_R16) || (kind == OP_R32) || (kind == OP_R64);
}


/*
check if operand is memory
*/
static bool is_memory(OperandKind kind)
{
    return (kind == OP_M8) || (kind == OP_M16) || (kind == OP_M32) || (kind == OP_M64);
}


/*
check if operand is register or memory
*/
static bool is_register_or_memory(OperandKind kind)
{
    return is_register(kind) || is_memory(kind);
}


/*
check if register is in eax register set
*/
static bool is_eax_register(RegisterKind kind)
{
    return (kind == REG_AL) || (kind == REG_AX) || (kind == REG_EAX) || (kind == REG_RAX);
}


/*
check if operand encoding is of type I
*/
static bool is_type_i_encoding(const Operand *operand1, const Operand *operand2)
{
    if(is_immediate(operand2->kind))
    {
        if(is_register(operand1->kind) && is_eax_register(operand1->reg))
        {
            return get_operand_size(operand1->kind) == get_operand_size(operand2->kind);
        }
    }

    return false;
}


/*
check if operand is signed 8-bit immediate
*/
static bool is_signed_8bit_immediate(const Operand *operand)
{
    if(get_operand_size(operand->kind) == SIZEOF_8BIT)
    {
        return (operand->immediate & SIGN_BIT_MASK_8BIT) == SIGN_BIT_MASK_8BIT;
    }
    else
    {
        return false;
    }
}


/*
get the least number of bytes to represent an immediate value
*/
size_t get_least_size(uintmax_t value)
{
    if(value == 0)
    {
        return 0;
    }
    else if((value <= UINT8_MAX) || (-value <= UINT8_MAX))
    {
        return SIZEOF_8BIT;
    }
    else if((value <= UINT16_MAX) || (-value <= UINT16_MAX))
    {
        return SIZEOF_16BIT;
    }
    else if((value <= UINT32_MAX) || (-value <= UINT32_MAX))
    {
        return SIZEOF_32BIT;
    }
    else
    {
        return SIZEOF_64BIT;
    }
}


/*
get size of operand
*/
static size_t get_operand_size(OperandKind kind)
{
    switch(kind)
    {
    case OP_IMM8:
    case OP_R8:
    case OP_M8:
        return SIZEOF_8BIT;

    case OP_IMM16:
    case OP_R16:
    case OP_M16:
        return SIZEOF_16BIT;

    case OP_IMM32:
    case OP_R32:
    case OP_M32:
        return SIZEOF_32BIT;

    case OP_IMM64:
    case OP_R64:
    case OP_M64:
        return SIZEOF_64BIT;

    default:
        return 0;
    }
}


/*
get register index
*/
static uint8_t get_register_index(RegisterKind kind)
{
    switch(kind)
    {
    case REG_AL:
    case REG_AX:
    case REG_EAX:
    case REG_RAX:
        return REGISTER_INDEX_EAX;

    case REG_CL:
    case REG_CX:
    case REG_ECX:
    case REG_RCX:
        return REGISTER_INDEX_ECX;

    case REG_DL:
    case REG_DX:
    case REG_EDX:
    case REG_RDX:
        return REGISTER_INDEX_EDX;

    case REG_BL:
    case REG_BX:
    case REG_EBX:
    case REG_RBX:
        return REGISTER_INDEX_EBX;

    case REG_SPL:
    case REG_SP:
    case REG_ESP:
    case REG_RSP:
        return REGISTER_INDEX_ESP;

    case REG_BPL:
    case REG_BP:
    case REG_EBP:
    case REG_RBP:
    case REG_RIP:
        return REGISTER_INDEX_EBP;

    case REG_SIL:
    case REG_SI:
    case REG_ESI:
    case REG_RSI:
        return REGISTER_INDEX_ESI;

    case REG_DIL:
    case REG_DI:
    case REG_EDI:
    case REG_RDI:
        return REGISTER_INDEX_EDI;

    case REG_R8B:
    case REG_R8W:
    case REG_R8D:
    case REG_R8:
        return REGISTER_INDEX_R8D;

    case REG_R9B:
    case REG_R9W:
    case REG_R9D:
    case REG_R9:
        return REGISTER_INDEX_R9D;

    case REG_R10B:
    case REG_R10W:
    case REG_R10D:
    case REG_R10:
        return REGISTER_INDEX_R10D;

    case REG_R11B:
    case REG_R11W:
    case REG_R11D:
    case REG_R11:
        return REGISTER_INDEX_R11D;

    case REG_R12B:
    case REG_R12W:
    case REG_R12D:
    case REG_R12:
        return REGISTER_INDEX_R12D;

    case REG_R13B:
    case REG_R13W:
    case REG_R13D:
    case REG_R13:
        return REGISTER_INDEX_R13D;

    case REG_R14B:
    case REG_R14W:
    case REG_R14D:
    case REG_R14:
        return REGISTER_INDEX_R14D;

    case REG_R15B:
    case REG_R15W:
    case REG_R15D:
    case REG_R15:
        return REGISTER_INDEX_R15D;

    default:
        return REGISTER_INDEX_INVALID;
    }
}


/*
get REX prefix of register
*/
static uint8_t get_rex_prefix(const Operand *operand, size_t prefix_position, bool specify_size)
{
    uint8_t prefix = 0x00;

    if(specify_size)
    {
        prefix = get_rex_prefix_for_size(operand);
    }

    if((get_register_index(operand->reg) & ~REG_FIELD_MASK) != PREFIX_NONE)
    {
        prefix |= get_rex_prefix_from_position(prefix_position);
    }

    return prefix;
}


/*
get REX prefix of register to specify operand size
*/
static uint8_t get_rex_prefix_for_size(const Operand *operand)
{
    uint8_t prefix = PREFIX_NONE;

    if(get_operand_size(operand->kind) == SIZEOF_8BIT)
    {
        switch(operand->reg)
        {
        case REG_SPL:
        case REG_BPL:
        case REG_SIL:
        case REG_DIL:
            prefix = PREFIX_REX;
            break;

        default:
            break;
        }
    }
    else if(get_operand_size(operand->kind) == SIZEOF_64BIT)
    {
        prefix = get_rex_prefix_from_position(PREFIX_POSITION_REX_W);
    }

    return prefix;
}


/*
get REX prefix from bit position
*/
static uint8_t get_rex_prefix_from_position(size_t prefix_position)
{
    return PREFIX_REX | (1 << prefix_position);
}


/*
get value of ModR/M byte
*/
static uint8_t get_modrm_byte(uint8_t mod, uint8_t reg, uint8_t rm)
{
    return (mod << MODRM_POSITION_MOD) + (reg << MODRM_POSITION_REG) + (rm << MODRM_POSITION_RM);
}


/*
get value of SIB byte
*/
static uint8_t get_sib_byte(uint8_t ss, uint8_t index, uint8_t base)
{
    return (ss << SIB_POSITION_SS) + (index << SIB_POSITION_INDEX) + (base << SIB_POSITION_BASE);
}


/*
get value of mod field
*/
static uint8_t get_mod_field(const Operand *operand)
{
    if(is_register(operand->kind))
    {
        return MOD_REG;
    }
    else
    {
        if(operand->reg == REG_RIP)
        {
            return MOD_MEM;
        }
        else
        {
            switch(get_least_size(operand->immediate))
            {
            case 0:
                return MOD_MEM;

            case SIZEOF_8BIT:
                return MOD_MEM_DISP8;

            case SIZEOF_32BIT:
                return MOD_MEM_DISP32;

            default:
                return MOD_INVALID;
            }
        }
    }
}


/*
get value of reg field
*/
static uint8_t get_reg_field(RegisterKind kind)
{
    return get_register_index(kind) & REG_FIELD_MASK;
}


/*
get value of r/m field
*/
static uint8_t get_rm_field(RegisterKind kind)
{
    return get_reg_field(kind);
}


/*
append binary for prefix
*/
static void append_binary_prefix(uint8_t prefix, ByteBufferType *buffer)
{
    append_bytes((char *)&prefix, sizeof(prefix), buffer);
}


/*
append binary for opecode
*/
static void append_binary_opecode(uint32_t opecode, ByteBufferType *buffer)
{
    // append binary for opecode which is more than 1 byte
    for(size_t i = OPECODE_SIZE_MAX - 1; i > 0; i--)
    {
        uint8_t byte = (opecode >> (8 * i)) & UINT8_T_MASK;
        if(byte != 0x00)
        {
            append_bytes((char *)&byte, sizeof(byte), buffer);
        }
    }

    uint8_t byte = opecode & UINT8_T_MASK;
    append_bytes((char *)&byte, sizeof(byte), buffer);
}


/*
append binary for ModR/M byte
*/
static void append_binary_modrm(uint8_t mod, uint8_t reg, uint8_t rm, ByteBufferType *buffer)
{
    uint8_t modrm = get_modrm_byte(mod, reg, rm);
    append_bytes((char *)&modrm, sizeof(modrm), buffer);

    if((mod != MOD_REG) && (rm == REGISTER_INDEX_ESP))
    {
        append_binary_sib(0x00, REGISTER_INDEX_ESP, REGISTER_INDEX_ESP, buffer);
    }
}


/*
append binary for SIB byte
*/
static void append_binary_sib(uint8_t ss, uint8_t index, uint8_t base, ByteBufferType *buffer)
{
    uint8_t sib = get_sib_byte(ss, index, base);
    append_bytes((char *)&sib, sizeof(sib), buffer);
}


/*
append binary for displacement
*/
static void append_binary_disp(const Operand *operand, Elf_Addr address, Elf_Sxword addend, ByteBufferType *buffer)
{
    if(operand->reg == REG_RIP)
    {
        append_binary_relocation(SIZEOF_32BIT, operand->symbol, address, addend, buffer);
    }
    else
    {
        append_binary_imm_least(operand->immediate, buffer);
    }
}


/*
append binary for immediate with a given size
*/
static void append_binary_imm(uintmax_t imm, size_t size, ByteBufferType *buffer)
{
    append_bytes((char *)&imm, size, buffer);
}


/*
append binary for immediate with least size
*/
static void append_binary_imm_least(uintmax_t imm, ByteBufferType *buffer)
{
    size_t size = get_least_size(imm);
    if(size > 0)
    {
        append_bytes((char *)&imm, size, buffer);
    }
}


/*
append binary for 32-bit immediate
*/
static void append_binary_imm32(uint32_t imm32, ByteBufferType *buffer)
{
    append_bytes((char *)&imm32, sizeof(imm32), buffer);
}


/*
append binary for relocation value
*/
static void append_binary_relocation(size_t size, Symbol *symbol, Elf_Addr address, Elf_Sxword addend, ByteBufferType *buffer)
{
    set_symbol(address, addend, SC_TEXT, symbol);
    switch(size)
    {
    case SIZEOF_32BIT:
    default:
        append_binary_imm32(0, buffer); // imm32 is a temporal value to be replaced during resolving symbols or relocation
        break;
    }
}


/*
append binary for instruction prefix if necessary
*/
static void may_append_binary_instruction_prefix(OperandKind kind, uint8_t prefix, ByteBufferType *buffer)
{
    if(get_operand_size(kind) == SIZEOF_16BIT)
    {
        append_binary_prefix(prefix, buffer);
    }
}


/*
append binary for REX prefix for instructions with reg and r/m fields if necessary
*/
static void may_append_binary_rex_prefix_reg_rm(const Operand *operand_reg, const Operand *operand_rm, bool specify_size, ByteBufferType *buffer)
{
    uint8_t prefix = 0x00;

    if(!is_immediate(operand_reg->kind))
    {
        prefix |= get_rex_prefix(operand_reg, PREFIX_POSITION_REX_R, specify_size);
    }
    if(!is_immediate(operand_rm->kind))
    {
        prefix |= get_rex_prefix(operand_rm, PREFIX_POSITION_REX_B, specify_size);
    }

    if(prefix != 0x00)
    {
        append_binary_prefix(prefix, buffer);
    }
}


/*
append binary for REX prefix for instructions with reg field if necessary
*/
static void may_append_binary_rex_prefix_reg(const Operand *operand, bool specify_size, ByteBufferType *buffer)
{
    uint8_t prefix = get_rex_prefix(operand, PREFIX_POSITION_REX_B, specify_size);

    if(prefix != 0x00)
    {
        append_binary_prefix(prefix, buffer);
    }
}
