#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "buffer.h"
#include "generator.h"
#include "parser.h"
#include "processor.h"

static void generate_op_call(const List(Operand) *operands, ByteBufferType *text_body);
static void generate_op_nop(const List(Operand) *operands, ByteBufferType *text_body);
static void generate_op_mov(const List(Operand) *operands, ByteBufferType *text_body);
static void generate_op_ret(const List(Operand) *operands, ByteBufferType *text_body);
static uint8_t get_modrm_byte(uint8_t dst_encoding, uint8_t dst_index, uint8_t src_index);
static uint8_t get_encoding_index_rm(RegisterKind kind);

const MnemonicInfo mnemonic_info_list[] = 
{
    {MN_CALL, "call", true,  generate_op_call},
    {MN_MOV,  "mov",  true,  generate_op_mov},
    {MN_NOP,  "nop",  false, generate_op_nop},
    {MN_RET,  "ret",  false, generate_op_ret},
};
const size_t MNEMONIC_INFO_LIST_SIZE = sizeof(mnemonic_info_list) / sizeof(mnemonic_info_list[0]);

const RegisterInfo register_info_list[] = 
{
    {REG_RAX, "rax", OP_R64},
    {REG_RCX, "rcx", OP_R64},
    {REG_RDX, "rdx", OP_R64},
    {REG_RBX, "rbx", OP_R64},
    {REG_RSP, "rsp", OP_R64},
    {REG_RBP, "rbp", OP_R64},
    {REG_RSI, "rsi", OP_R64},
    {REG_RDI, "rdi", OP_R64},
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
        new_label_info(operand->label, text_body->size + 1);

        const char *opecode = "\xe8";
        append_bytes(opecode, 1, text_body);

        uint32_t rel32 = 0; // temporal value
        append_bytes((char *)&rel32, sizeof(rel32), text_body);
    }
}


/*
generate mov operation
*/
static void generate_op_mov(const List(Operand) *operands, ByteBufferType *text_body)
{
    ListEntry(Operand) *entry = get_first_entry(Operand)(operands);
    Operand *first = get_element(Operand)(entry);
    Operand *second = get_element(Operand)(next_entry(Operand, entry));
    if((first->kind == OP_R64) && (second->kind == OP_R64))
    {
        const char *prefixes = "\x48";
        append_bytes(prefixes, 1, text_body);

        const char *opecode = "\x89";
        append_bytes(opecode, 1, text_body);

        uint8_t dst_encoding = 0x03;
        uint8_t dst_index = get_encoding_index_rm(first->reg);
        uint8_t src_index = get_encoding_index_rm(second->reg);
        uint8_t modrm = get_modrm_byte(dst_encoding, dst_index, src_index);
        append_bytes((char *)&modrm, sizeof(modrm), text_body);
    }
    else if((first->kind == OP_R64) && (second->kind == OP_IMM32))
    {
        const char *prefixes = "\x48";
        append_bytes(prefixes, 1, text_body);

        const char *opecode = "\xc7";
        append_bytes(opecode, 1, text_body);

        uint8_t dst_encoding = 0x03;
        uint8_t dst_index = get_encoding_index_rm(first->reg);
        uint8_t src_index = 0x00;
        uint8_t modrm = get_modrm_byte(dst_encoding, dst_index, src_index);
        append_bytes((char *)&modrm, sizeof(modrm), text_body);

        uint32_t immediate = second->immediate;
        append_bytes((char *)&immediate, sizeof(immediate), text_body);
    }
}


/*
generate nop operation
*/
static void generate_op_nop(const List(Operand) *operands, ByteBufferType *text_body)
{
    uint8_t mnemonic = 0x90;
    append_bytes((char *)&mnemonic, sizeof(mnemonic), text_body);
}


/*
generate ret operation
*/
static void generate_op_ret(const List(Operand) *operands, ByteBufferType *text_body)
{
    uint8_t mnemonic = 0xc3;
    append_bytes((char *)&mnemonic, sizeof(mnemonic), text_body);
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
