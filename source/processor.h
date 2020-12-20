#ifndef __PROCESSOR_H__
#define __PROCESSOR_H__

#include "buffer.h"
#include "elf_wrap.h"

typedef enum OperandKind OperandKind;
typedef enum MnemonicKind MnemonicKind;
typedef enum RegisterKind RegisterKind;
typedef struct MnemonicMap MnemonicMap;
typedef struct Operand Operand;
typedef struct Operation Operation;
typedef struct RegisterMap RegisterMap;

#include "list.h"
define_list(Operand)
define_list(Operation)

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

// structure for mapping from string to kind of mnemonic
struct MnemonicMap
{
    const char *name;                                                         // name of mnemonic
    MnemonicKind kind;                                                        // kind of mnemonic
    const void (*generate_function)(const List(Operand) *, ByteBufferType *); // function to generate operation
};

// structure for operand
struct Operand
{
    OperandKind kind;      // kind of operand
    union
    {
        long immediate;    // immediate value
        RegisterKind reg;  // kind of register
        const char *label; // label
    };
};

// structure for operation
struct Operation
{
    MnemonicKind kind;             // kind of operation
    const List(Operand) *operands; // list of operands
    Elf_Addr address;              // address of operation
};

// structure for mapping from string to kind of register
struct RegisterMap
{
    const char *name;      // name of register
    RegisterKind reg_kind; // kind of register
    OperandKind op_kind;   // kind of operand
};

extern const MnemonicMap mnemonic_maps[];
extern const size_t MNEMONIC_MAP_SIZE;

extern const RegisterMap register_maps[];
extern const size_t REGISTER_MAP_SIZE;

void generate_operation(const Operation *operation, ByteBufferType *text_body);

#endif /* !__PROCESSOR_H__ */
