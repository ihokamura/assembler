#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "buffer.h"
#include "generator.h"
#include "parser.h"
#include "processor.h"

#define min(a, b)    ((a) < (b) ? (a) : (b))

static void generate_op_call(const List(Operand) *operands, ByteBufferType *text_body);
static void generate_op_mov(const List(Operand) *operands, ByteBufferType *text_body);
static void generate_op_nop(const List(Operand) *operands, ByteBufferType *text_body);
static void generate_op_pop(const List(Operand) *operands, ByteBufferType *text_body);
static void generate_op_push(const List(Operand) *operands, ByteBufferType *text_body);
static void generate_op_ret(const List(Operand) *operands, ByteBufferType *text_body);
static void generate_op_sub(const List(Operand) *operands, ByteBufferType *text_body);
static bool is_immediate(OperandKind kind);
static bool is_register(OperandKind kind);
static bool is_memory(OperandKind kind);
static size_t get_operand_size(OperandKind kind);
static uint8_t get_modrm_byte(uint8_t dst_encoding, uint8_t dst_index, uint8_t src_index);
static uint8_t get_mod_field(uint32_t immediate);
static uint8_t get_register_field(RegisterKind kind);
static void append_binary_prefix(uint8_t prefix, ByteBufferType *text_body);
static void append_binary_opecode(uint8_t opecode, ByteBufferType *text_body);
static void append_binary_modrm(uint8_t dst_encoding, uint8_t dst_index, uint8_t src_index, ByteBufferType *text_body);
static void append_binary_imm(uint32_t imm, size_t size, ByteBufferType *text_body);
static void append_binary_imm_least(uint32_t imm, ByteBufferType *text_body);
static void append_binary_imm32(uint32_t imm32, ByteBufferType *text_body);
static void append_binary_relocation(size_t size, const char *label, Elf_Addr address, Elf_Sxword addend, ByteBufferType *text_body);
static void may_append_binary_instruction_prefix(OperandKind kind, uint8_t prefix, ByteBufferType *text_body);
static void may_append_binary_rex_prefix(const Operand *operand, uint8_t prefix, ByteBufferType *text_body);
static void may_append_binary_relocation(const Operand *operand, Elf_Addr address, Elf_Sxword addend, ByteBufferType *text_body);

const MnemonicInfo mnemonic_info_list[] = 
{
    {MN_CALL, "call", true,  generate_op_call},
    {MN_MOV,  "mov",  true,  generate_op_mov},
    {MN_NOP,  "nop",  false, generate_op_nop},
    {MN_POP,  "pop",  true,  generate_op_pop},
    {MN_PUSH, "push", true,  generate_op_push},
    {MN_RET,  "ret",  false, generate_op_ret},
    {MN_SUB,  "sub",  true,  generate_op_sub},
};
const size_t MNEMONIC_INFO_LIST_SIZE = sizeof(mnemonic_info_list) / sizeof(mnemonic_info_list[0]);

const RegisterInfo register_info_list[] = 
{
    {REG_AL,  "al",  OP_R8},
    {REG_CL,  "cl",  OP_R8},
    {REG_DL,  "dl",  OP_R8},
    {REG_BL,  "bl",  OP_R8},
    {REG_SPL, "spl", OP_R8},
    {REG_BPL, "bpl", OP_R8},
    {REG_SIL, "sil", OP_R8},
    {REG_DIL, "dil", OP_R8},
    {REG_AX,  "ax",  OP_R16},
    {REG_CX,  "cx",  OP_R16},
    {REG_DX,  "dx",  OP_R16},
    {REG_BX,  "bx",  OP_R16},
    {REG_SP,  "sp",  OP_R16},
    {REG_BP,  "bp",  OP_R16},
    {REG_SI,  "si",  OP_R16},
    {REG_DI,  "di",  OP_R16},
    {REG_EAX, "eax", OP_R32},
    {REG_ECX, "ecx", OP_R32},
    {REG_EDX, "edx", OP_R32},
    {REG_EBX, "ebx", OP_R32},
    {REG_ESP, "esp", OP_R32},
    {REG_EBP, "ebp", OP_R32},
    {REG_ESI, "esi", OP_R32},
    {REG_EDI, "edi", OP_R32},
    {REG_RAX, "rax", OP_R64},
    {REG_RCX, "rcx", OP_R64},
    {REG_RDX, "rdx", OP_R64},
    {REG_RBX, "rbx", OP_R64},
    {REG_RSP, "rsp", OP_R64},
    {REG_RBP, "rbp", OP_R64},
    {REG_RSI, "rsi", OP_R64},
    {REG_RDI, "rdi", OP_R64},
    {REG_RIP, "rip", OP_R64},
};
const size_t REGISTER_INFO_LIST_SIZE = sizeof(register_info_list) / sizeof(register_info_list[0]);

static const uint8_t PREFIX_OPERAND_SIZE_OVERRIDE = 0x66;
static const uint8_t PREFIX_REX = 0x40;
static const uint8_t PREFIX_REX_W = 0x48;
static const uint8_t REGISTER_INDEX_EAX = 0x00;
static const uint8_t REGISTER_INDEX_ECX = 0x01;
static const uint8_t REGISTER_INDEX_EDX = 0x02;
static const uint8_t REGISTER_INDEX_EBX = 0x03;
static const uint8_t REGISTER_INDEX_ESP = 0x04;
static const uint8_t REGISTER_INDEX_EBP = 0x05;
static const uint8_t REGISTER_INDEX_ESI = 0x06;
static const uint8_t REGISTER_INDEX_EDI = 0x07;
static const uint8_t REGISTER_INDEX_INVALID = 0xff;
static const uint8_t MOD_MEM = 0;
static const uint8_t MOD_MEM_DISP8 = 1;
static const uint8_t MOD_MEM_DISP32 = 2;
static const uint8_t MOD_REG = 3;

/*
generate an operation
*/
void generate_operation(const Operation *operation, ByteBufferType *text_body)
{
    mnemonic_info_list[operation->kind].generate_function(operation->operands, text_body);
}


/*
generate call operation
*/
static void generate_op_call(const List(Operand) *operands, ByteBufferType *text_body)
{
    Operand *operand = get_first_element(Operand)(operands);
    if(operand->kind == OP_SYMBOL)
    {
        /*
        handle the following instructions
        * CALL rel32
        */
        append_binary_opecode(0xe8, text_body);
        append_binary_relocation(SIZEOF_32BIT, operand->label, text_body->size, -SIZEOF_32BIT, text_body);
    }
}


/*
generate mov operation
*/
static void generate_op_mov(const List(Operand) *operands, ByteBufferType *text_body)
{
    ListEntry(Operand) *entry = get_first_entry(Operand)(operands);
    Operand *operand1 = get_element(Operand)(entry);
    Operand *operand2 = get_element(Operand)(next_entry(Operand, entry));
    if(is_register(operand1->kind) && is_register(operand2->kind))
    {
        /*
        handle the following instructions
        * MOV r8,r8
        * MOV r16,r16
        * MOV r32,r32
        * MOV r64,r64
        */
        assert(get_operand_size(operand1->kind) == get_operand_size(operand2->kind));
        may_append_binary_instruction_prefix(operand1->kind, PREFIX_OPERAND_SIZE_OVERRIDE, text_body);
        may_append_binary_rex_prefix(operand1, PREFIX_REX, text_body);
        may_append_binary_rex_prefix(operand1, PREFIX_REX_W, text_body);
        uint8_t opecode = (get_operand_size(operand1->kind) == SIZEOF_8BIT) ? 0x88 : 0x89;
        append_binary_opecode(opecode, text_body);
        append_binary_modrm(MOD_REG, get_register_field(operand1->reg), get_register_field(operand2->reg), text_body);
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
        may_append_binary_instruction_prefix(operand1->kind, PREFIX_OPERAND_SIZE_OVERRIDE, text_body);
        may_append_binary_rex_prefix(operand1, PREFIX_REX, text_body);
        may_append_binary_rex_prefix(operand1, PREFIX_REX_W, text_body);
        uint8_t opecode = (get_operand_size(operand1->kind) == SIZEOF_8BIT) ? 0x8a : 0x8b;
        append_binary_opecode(opecode, text_body);
        append_binary_modrm(get_mod_field(operand2->immediate), get_register_field(operand2->reg), get_register_field(operand1->reg), text_body);
        may_append_binary_relocation(operand2, text_body->size, -SIZEOF_32BIT, text_body);
        append_binary_imm_least(operand2->immediate, text_body);
    }
    else if(is_register(operand1->kind) && is_immediate(operand2->kind))
    {
        if((get_operand_size(operand1->kind) == SIZEOF_64BIT) && (get_operand_size(operand2->kind) < SIZEOF_64BIT))
        {
            /*
            handle the following instructions
            * MOV r64, imm32
            */
            may_append_binary_rex_prefix(operand1, PREFIX_REX_W, text_body);
            append_binary_opecode(0xc7, text_body);
            append_binary_modrm(is_register(operand1->kind) ? MOD_REG : get_mod_field(operand1->immediate), get_register_field(operand1->reg), 0x00, text_body);
            append_binary_imm32(operand2->immediate, text_body);
        }
        else
        {
            /*
            handle the following instructions
            * MOV r8, imm8
            * MOV r16, imm16
            * MOV r32, imm32
            */
            assert(get_operand_size(operand1->kind) >= get_operand_size(operand2->kind));
            may_append_binary_instruction_prefix(operand1->kind, PREFIX_OPERAND_SIZE_OVERRIDE, text_body);
            may_append_binary_rex_prefix(operand1, PREFIX_REX, text_body);
            size_t opecode = (get_operand_size(operand1->kind) == SIZEOF_8BIT) ? 0xb0 : 0xb8;
            append_binary_opecode(opecode + get_register_field(operand1->reg), text_body);
            append_binary_imm(operand2->immediate, get_operand_size(operand1->kind), text_body);
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
        may_append_binary_instruction_prefix(operand1->kind, PREFIX_OPERAND_SIZE_OVERRIDE, text_body);
        may_append_binary_rex_prefix(operand1, PREFIX_REX, text_body);
        may_append_binary_rex_prefix(operand1, PREFIX_REX_W, text_body);
        uint8_t opecode = (get_operand_size(operand1->kind) == SIZEOF_8BIT) ? 0xc6 : 0xc7;
        append_binary_opecode(opecode, text_body);
        append_binary_modrm(is_register(operand1->kind) ? MOD_REG : get_mod_field(operand1->immediate), get_register_field(operand1->reg), 0x00, text_body);
        size_t imm_size = min(get_operand_size(operand1->kind), SIZEOF_32BIT);
        may_append_binary_relocation(operand1, text_body->size, -(SIZEOF_32BIT + imm_size), text_body);
        append_binary_imm_least(operand1->immediate, text_body);
        append_binary_imm(operand2->immediate, imm_size, text_body);
    }
}


/*
generate nop operation
*/
static void generate_op_nop(const List(Operand) *operands, ByteBufferType *text_body)
{
    /*
    handle the following instructions
    * NOP
    */
    append_binary_opecode(0x90, text_body);
}


/*
generate pop operation
*/
static void generate_op_pop(const List(Operand) *operands, ByteBufferType *text_body)
{
    const Operand *operand = get_first_element(Operand)(operands);

    if(operand->kind == OP_R64)
    {
        /*
        handle the following instructions
        * POP r64
        */
        append_binary_opecode(0x58 + get_register_field(operand->reg), text_body);
    }
}


/*
generate push operation
*/
static void generate_op_push(const List(Operand) *operands, ByteBufferType *text_body)
{
    const Operand *operand = get_first_element(Operand)(operands);

    if(operand->kind == OP_R64)
    {
        /*
        handle the following instructions
        * PUSH r64
        */
        append_binary_opecode(0x50 + get_register_field(operand->reg), text_body);
    }
}


/*
generate ret operation
*/
static void generate_op_ret(const List(Operand) *operands, ByteBufferType *text_body)
{
    /*
    handle the following instructions
    * RET
    */
    append_binary_opecode(0xc3, text_body);
}


/*
generate sub operation
*/
static void generate_op_sub(const List(Operand) *operands, ByteBufferType *text_body)
{
    ListEntry(Operand) *entry = get_first_entry(Operand)(operands);
    Operand *operand1 = get_element(Operand)(entry);
    Operand *operand2 = get_element(Operand)(next_entry(Operand, entry));
    if(is_register(operand1->kind) && is_register(operand2->kind))
    {
        /*
        handle the following instructions
        * SUB r16, r16
        * SUB r32, r32
        * SUB r64, r64
        */
        assert(get_operand_size(operand1->kind) == get_operand_size(operand2->kind));
        may_append_binary_instruction_prefix(operand1->kind, PREFIX_OPERAND_SIZE_OVERRIDE, text_body);
        may_append_binary_rex_prefix(operand1, PREFIX_REX_W, text_body);
        append_binary_opecode(0x29, text_body);
        append_binary_modrm(MOD_REG, get_register_field(operand1->reg), get_register_field(operand2->reg), text_body);
    }
    else if(is_register(operand1->kind) && is_immediate(operand2->kind))
    {
        assert(get_operand_size(operand1->kind) >= get_operand_size(operand2->kind));
        if(
               ((operand1->reg == REG_AX) && (get_operand_size(operand2->kind) == SIZEOF_16BIT))
            || ((operand1->reg == REG_EAX) && (get_operand_size(operand2->kind) == SIZEOF_32BIT))
            || ((operand1->reg == REG_RAX) && (get_operand_size(operand2->kind) == SIZEOF_32BIT))
            )
        {
            /*
            handle the following instructions
            * SUB ax, imm16
            * SUB eax, imm32
            * SUB rax, imm32
            */
            may_append_binary_instruction_prefix(operand1->kind, PREFIX_OPERAND_SIZE_OVERRIDE, text_body);
            may_append_binary_rex_prefix(operand1, PREFIX_REX_W, text_body);
            append_binary_opecode(0x2d, text_body);
            append_binary_imm_least(operand2->immediate, text_body);
        }
        else
        {
            /*
            handle the following instructions
            * SUB r16, imm8
            * SUB r32, imm8
            * SUB r64, imm8
            * SUB r16, imm32
            * SUB r32, imm32
            * SUB r64, imm32
            */
            may_append_binary_instruction_prefix(operand1->kind, PREFIX_OPERAND_SIZE_OVERRIDE, text_body);
            may_append_binary_rex_prefix(operand1, PREFIX_REX_W, text_body);
            uint8_t opecode = (get_operand_size(operand2->kind) == SIZEOF_8BIT) ? 0x83 : 0x81;
            append_binary_opecode(opecode, text_body);
            append_binary_modrm(MOD_REG, get_register_field(operand1->reg), 0x05, text_body);
            append_binary_imm(operand2->immediate, get_operand_size(operand2->kind), text_body);
        }
    }
    else if(is_memory(operand1->kind) && is_immediate(operand2->kind))
    {
        /*
        handle the following instructions
        * SUB m16, imm8
        * SUB m32, imm8
        * SUB m64, imm8
        * SUB m16, imm32
        * SUB m32, imm32
        * SUB m64, imm32
        */
        assert(get_operand_size(operand1->kind) >= get_operand_size(operand2->kind));
        may_append_binary_instruction_prefix(operand1->kind, PREFIX_OPERAND_SIZE_OVERRIDE, text_body);
        may_append_binary_rex_prefix(operand1, PREFIX_REX_W, text_body);
        uint8_t opecode = (get_operand_size(operand2->kind) == SIZEOF_8BIT) ? 0x83 : 0x81;
        append_binary_opecode(opecode, text_body);
        append_binary_modrm(MOD_MEM, get_register_field(operand1->reg), 0x05, text_body);
        append_binary_imm(operand2->immediate, get_operand_size(operand2->kind), text_body);
    }
}


/*
check if operand is immediate
*/
static bool is_immediate(OperandKind kind)
{
    return (kind == OP_IMM8) || (kind == OP_IMM16) || (kind == OP_IMM32);
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

    case OP_R64:
    case OP_M64:
        return SIZEOF_64BIT;

    default:
        return 0;
    }
}


/*
get value of ModR/M byte
*/
static uint8_t get_modrm_byte(uint8_t dst_encoding, uint8_t dst_index, uint8_t src_index)
{
    return (dst_encoding << 6) + (src_index << 3) + dst_index;
}


/*
get value of Mod field
*/
static uint8_t get_mod_field(uint32_t immediate)
{
    switch(get_least_size(immediate))
    {
    case 0:
        return MOD_MEM;

    case SIZEOF_8BIT:
        return MOD_MEM_DISP8;

    case SIZEOF_32BIT:
    default:
        return MOD_MEM_DISP32;
    }
}


/*
get value of register field
*/
static uint8_t get_register_field(RegisterKind kind)
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

    default:
        return REGISTER_INDEX_INVALID;
    }
}


/*
append binary for prefix
*/
static void append_binary_prefix(uint8_t prefix, ByteBufferType *text_body)
{
    append_bytes((char *)&prefix, sizeof(prefix), text_body);
}


/*
append binary for opecode
*/
static void append_binary_opecode(uint8_t opecode, ByteBufferType *text_body)
{
    append_bytes((char *)&opecode, sizeof(opecode), text_body);
}


/*
append binary for ModR/M byte
*/
static void append_binary_modrm(uint8_t dst_encoding, uint8_t dst_index, uint8_t src_index, ByteBufferType *text_body)
{
    uint8_t modrm = get_modrm_byte(dst_encoding, dst_index, src_index);
    append_bytes((char *)&modrm, sizeof(modrm), text_body);
}


/*
append binary for immediate with a given size
*/
static void append_binary_imm(uint32_t imm, size_t size, ByteBufferType *text_body)
{
    append_bytes((char *)&imm, size, text_body);
}


/*
append binary for immediate with least size
*/
static void append_binary_imm_least(uint32_t imm, ByteBufferType *text_body)
{
    size_t size = get_least_size(imm);
    if(size > 0)
    {
        append_bytes((char *)&imm, size, text_body);
    }
}


/*
append binary for 32-bit immediate
*/
static void append_binary_imm32(uint32_t imm32, ByteBufferType *text_body)
{
    append_bytes((char *)&imm32, sizeof(imm32), text_body);
}


/*
append binary for relocation value
*/
static void append_binary_relocation(size_t size, const char *label, Elf_Addr address, Elf_Sxword addend, ByteBufferType *text_body)
{
    new_label_info(label, address, addend);
    switch(size)
    {
    case SIZEOF_32BIT:
    default:
        append_binary_imm32(0, text_body); // imm32 is a temporal value to be replaced during resolving symbols or relocation
        break;
    }
}


/*
append binary for instruction prefix if necessary
*/
static void may_append_binary_instruction_prefix(OperandKind kind, uint8_t prefix, ByteBufferType *text_body)
{
    if(get_operand_size(kind) == SIZEOF_16BIT)
    {
        append_binary_prefix(prefix, text_body);
    }
}


/*
append binary for REX prefix if necessary
*/
static void may_append_binary_rex_prefix(const Operand *operand, uint8_t prefix, ByteBufferType *text_body)
{
    bool append = false;
    switch(get_operand_size(operand->kind))
    {
    case SIZEOF_8BIT:
        switch(operand->reg)
        {
        case REG_SPL:
        case REG_BPL:
        case REG_SIL:
        case REG_DIL:
            append = (prefix == PREFIX_REX);
            break;

        default:
            break;
        }
        break;
    
    case SIZEOF_64BIT:
        append = (prefix == PREFIX_REX_W);
        break;

    default:
        break;
    }

    if(append)
    {
        append_binary_prefix(prefix, text_body);
    }
}


/*
append binary for relocation value if the operand is rip-relative memory
*/
static void may_append_binary_relocation(const Operand *operand, Elf_Addr address, Elf_Sxword addend, ByteBufferType *text_body)
{
    if(operand->reg == REG_RIP)
    {
        append_binary_relocation(SIZEOF_32BIT, operand->label, address, addend, text_body);
    }
}
