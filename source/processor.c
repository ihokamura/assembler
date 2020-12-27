#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "buffer.h"
#include "generator.h"
#include "parser.h"
#include "processor.h"

static void generate_op_call(const List(Operand) *operands, ByteBufferType *text_body);
static void generate_op_mov(const List(Operand) *operands, ByteBufferType *text_body);
static void generate_op_nop(const List(Operand) *operands, ByteBufferType *text_body);
static void generate_op_pop(const List(Operand) *operands, ByteBufferType *text_body);
static void generate_op_push(const List(Operand) *operands, ByteBufferType *text_body);
static void generate_op_ret(const List(Operand) *operands, ByteBufferType *text_body);
static void generate_op_sub(const List(Operand) *operands, ByteBufferType *text_body);
static uint8_t get_modrm_byte(uint8_t dst_encoding, uint8_t dst_index, uint8_t src_index);
static uint8_t get_register_field(RegisterKind kind);
static void append_binary_prefix(uint8_t prefix, ByteBufferType *text_body);
static void append_binary_opecode(uint8_t opecode, ByteBufferType *text_body);
static void append_binary_modrm(uint8_t dst_encoding, uint8_t dst_index, uint8_t src_index, ByteBufferType *text_body);
static void append_binary_imm32(uint32_t imm32, ByteBufferType *text_body);
static void append_binary_relocation(size_t size, const char *label, Elf_Addr address, Elf_Sxword addend, ByteBufferType *text_body);

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
        append_binary_opecode(0xe8, text_body);
        append_binary_relocation(sizeof(uint32_t), operand->label, text_body->size, -sizeof(uint32_t), text_body);
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
    if((operand1->kind == OP_R32) && (operand2->kind == OP_R32))
    {
        append_binary_opecode(0x89, text_body);
        append_binary_modrm(0x03, get_register_field(operand1->reg), get_register_field(operand2->reg), text_body);
    }
    else if((operand1->kind == OP_R32) && (operand2->kind == OP_M32))
    {
        append_binary_opecode(0x8b, text_body);
        append_binary_modrm(0x00, get_register_field(operand2->reg), get_register_field(operand1->reg), text_body);
    }
    else if((operand1->kind == OP_R64) && (operand2->kind == OP_R64))
    {
        append_binary_prefix(0x48, text_body);
        append_binary_opecode(0x89, text_body);
        append_binary_modrm(0x03, get_register_field(operand1->reg), get_register_field(operand2->reg), text_body);
    }
    else if((operand1->kind == OP_R64) && (operand2->kind == OP_M64))
    {
        append_binary_prefix(0x48, text_body);
        append_binary_opecode(0x8b, text_body);
        append_binary_modrm(0x00, get_register_field(operand2->reg), get_register_field(operand1->reg), text_body);
        if(operand2->reg == REG_RIP)
        {
            append_binary_relocation(sizeof(uint32_t), operand2->label, text_body->size, -sizeof(uint32_t), text_body);
        }
    }
    else if((operand1->kind == OP_R32) && (operand2->kind == OP_IMM32))
    {
        append_binary_opecode(0xb8 + get_register_field(operand1->reg), text_body);
        append_binary_imm32(operand2->immediate, text_body);
    }
    else if((operand1->kind == OP_M32) && (operand2->kind == OP_IMM32))
    {
        append_binary_opecode(0xc7, text_body);
        append_binary_modrm(0x00, get_register_field(operand1->reg), 0x00, text_body);
        append_binary_imm32(operand2->immediate, text_body);
    }
    else if((operand1->kind == OP_R64) && (operand2->kind == OP_IMM32))
    {
        append_binary_prefix(0x48, text_body);
        append_binary_opecode(0xc7, text_body);
        append_binary_modrm(0x03, get_register_field(operand1->reg), 0x00, text_body);
        append_binary_imm32(operand2->immediate, text_body);
    }
    else if((operand1->kind == OP_M64) && (operand2->kind == OP_IMM32))
    {
        append_binary_prefix(0x48, text_body);
        append_binary_opecode(0xc7, text_body);
        append_binary_modrm(0x00, get_register_field(operand1->reg), 0x00, text_body);
        if(operand1->reg == REG_RIP)
        {
            append_binary_relocation(sizeof(uint32_t), operand1->label, text_body->size, -(2 * sizeof(uint32_t)), text_body);
        }
        append_binary_imm32(operand2->immediate, text_body);
    }
}


/*
generate nop operation
*/
static void generate_op_nop(const List(Operand) *operands, ByteBufferType *text_body)
{
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
        append_binary_opecode(0x50 + get_register_field(operand->reg), text_body);
    }
}


/*
generate ret operation
*/
static void generate_op_ret(const List(Operand) *operands, ByteBufferType *text_body)
{
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
    if((operand1->kind == OP_R32) && (operand2->kind == OP_R32))
    {
        append_binary_opecode(0x29, text_body);
        append_binary_modrm(0x03, get_register_field(operand1->reg), get_register_field(operand2->reg), text_body);
    }
    else if((operand1->kind == OP_R64) && (operand2->kind == OP_R64))
    {
        append_binary_prefix(0x48, text_body);
        append_binary_opecode(0x29, text_body);
        append_binary_modrm(0x03, get_register_field(operand1->reg), get_register_field(operand2->reg), text_body);
    }
    else if((operand1->kind == OP_R32) && (operand2->kind == OP_IMM32))
    {
        append_binary_opecode(0x81, text_body);
        append_binary_modrm(0x03, get_register_field(operand1->reg), 0x05, text_body);
        append_binary_imm32(operand2->immediate, text_body);
    }
    else if((operand1->kind == OP_R64) && (operand2->kind == OP_IMM32))
    {
        append_binary_prefix(0x48, text_body);
        append_binary_opecode(0x81, text_body);
        append_binary_modrm(0x03, get_register_field(operand1->reg), 0x05, text_body);
        append_binary_imm32(operand2->immediate, text_body);
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
get value of register field
*/
static uint8_t get_register_field(RegisterKind kind)
{
    switch(kind)
    {
    case REG_EAX:
    case REG_RAX:
        return 0x00;

    case REG_ECX:
    case REG_RCX:
        return 0x01;

    case REG_EDX:
    case REG_RDX:
        return 0x02;

    case REG_EBX:
    case REG_RBX:
        return 0x03;

    case REG_ESP:
    case REG_RSP:
        return 0x04;

    case REG_EBP:
    case REG_RBP:
    case REG_RIP:
        return 0x05;

    case REG_ESI:
    case REG_RSI:
        return 0x06;

    case REG_EDI:
    case REG_RDI:
        return 0x07;

    default:
        return 0x00;
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
    case sizeof(uint32_t):
    default:
        append_binary_imm32(0, text_body); // imm32 is a temporal value to be replaced during resolving symbols or relocation
        break;
    }
}
