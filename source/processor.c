#include <stddef.h>

#include "processor.h"

const MnemonicMap mnemonic_maps[] = 
{
    {"call", MN_CALL},
    {"mov", MN_MOV},
    {"nop", MN_NOP},
    {"ret", MN_RET},
};
const size_t MNEMONIC_MAP_SIZE = sizeof(mnemonic_maps) / sizeof(mnemonic_maps[0]);

const RegisterMap register_maps[] = 
{
    {"rax", OP_R64, REG_RAX},
    {"rcx", OP_R64, REG_RCX},
    {"rdx", OP_R64, REG_RDX},
    {"rbx", OP_R64, REG_RBX},
    {"rsp", OP_R64, REG_RSP},
    {"rbp", OP_R64, REG_RBP},
    {"rsi", OP_R64, REG_RSI},
    {"rdi", OP_R64, REG_RDI},
};
const size_t REGISTER_MAP_SIZE = sizeof(register_maps) / sizeof(register_maps[0]);
