#ifndef __PROCESSOR_H__
#define __PROCESSOR_H__

#include <stdbool.h>

#include "buffer.h"
#include "elf_wrap.h"

typedef enum OperandKind OperandKind;
typedef enum MnemonicKind MnemonicKind;
typedef enum RegisterKind RegisterKind;
typedef struct MnemonicInfo MnemonicInfo;
typedef struct Operand Operand;
typedef struct Operation Operation;
typedef struct RegisterInfo RegisterInfo;

#include "list.h"
define_list(Operand)
define_list(Operation)

// kind of mnemonic
enum MnemonicKind
{
    MN_CALL, // call
    MN_MOV,  // mov
    MN_NOP,  // nop
    MN_POP,  // pop
    MN_PUSH, // push
    MN_RET,  // ret
    MN_SUB,  // sub
};

// kind of operand
enum OperandKind
{
    OP_IMM8,   // 8-bit immediate
    OP_IMM32,  // 32-bit immediate
    OP_R32,    // 32-bit register
    OP_R64,    // 64-bit register
    OP_M32,    // 32-bit memory
    OP_M64,    // 64-bit memory
    OP_SYMBOL, // symbol
};

// kind of register
enum RegisterKind
{
    REG_EAX, // eax
    REG_ECX, // ecx
    REG_EDX, // edx
    REG_EBX, // ebx
    REG_ESP, // esp
    REG_EBP, // ebp
    REG_ESI, // esi
    REG_EDI, // edi
    REG_RAX, // rax
    REG_RCX, // rcx
    REG_RDX, // rdx
    REG_RBX, // rbx
    REG_RSP, // rsp
    REG_RBP, // rbp
    REG_RSI, // rsi
    REG_RDI, // rdi
    REG_RIP, // rip
};

// structure for mapping from string to kind of mnemonic
struct MnemonicInfo
{
    MnemonicKind kind;                                                        // kind of mnemonic
    const char *name;                                                         // name of mnemonic
    bool take_operands;                                                       // flag indicating that the mnemonic takes operands
    const void (*generate_function)(const List(Operand) *, ByteBufferType *); // function to generate operation
};

// structure for operand
struct Operand
{
    OperandKind kind;       // kind of operand
    struct
    {
        uint32_t immediate; // immediate value
        RegisterKind reg;   // kind of register
        const char *label;  // label
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
struct RegisterInfo
{
    RegisterKind reg_kind; // kind of register
    const char *name;      // name of register
    OperandKind op_kind;   // kind of operand
};

extern const MnemonicInfo mnemonic_info_list[];
extern const size_t MNEMONIC_INFO_LIST_SIZE;

extern const RegisterInfo register_info_list[];
extern const size_t REGISTER_INFO_LIST_SIZE;

void generate_operation(const Operation *operation, ByteBufferType *text_body);

#endif /* !__PROCESSOR_H__ */
