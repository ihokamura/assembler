#ifndef __PROCESSOR_H__
#define __PROCESSOR_H__

typedef enum OperandKind OperandKind;
typedef enum MnemonicKind MnemonicKind;
typedef enum RegisterKind RegisterKind;
typedef struct MnemonicMap MnemonicMap;
typedef struct RegisterMap RegisterMap;

// kind of mnemonic
enum MnemonicKind
{
    MN_CALL, // call
    MN_MOV,  // mov
    MN_NOP,  // nop
    MN_RET,  // ret
};

// kind of operand
enum OperandKind
{
    OP_IMM32,  // 32-bit immediate
    OP_R64,    // 64-bit register
    OP_SYMBOL, // symbol
};

// kind of register
enum RegisterKind
{
    REG_RAX, // rax
    REG_RCX, // rcx
    REG_RDX, // rdx
    REG_RBX, // rbx
    REG_RSP, // rsp
    REG_RBP, // rbp
    REG_RSI, // rsi
    REG_RDI, // rdi
};

// mapping from string to kind of mnemonic
struct MnemonicMap
{
    const char *name;
    MnemonicKind kind;
};

// mapping from string to kind of register
struct RegisterMap
{
    const char *name;
    OperandKind op_kind;
    RegisterKind reg_kind;
};

extern const MnemonicMap mnemonic_maps[];
extern const size_t MNEMONIC_MAP_SIZE;

extern const RegisterMap register_maps[];
extern const size_t REGISTER_MAP_SIZE;

#endif /* !__PROCESSOR_H__ */
