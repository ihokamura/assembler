#ifndef __PROCESSOR_H__
#define __PROCESSOR_H__

#include <stdbool.h>

#include "buffer.h"
#include "elf_wrap.h"

#define SIZEOF_8BIT     sizeof(uint8_t)
#define SIZEOF_16BIT    sizeof(uint16_t)
#define SIZEOF_32BIT    sizeof(uint32_t)
#define SIZEOF_64BIT    sizeof(uint64_t)

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
    OP_IMM16,  // 16-bit immediate
    OP_IMM32,  // 32-bit immediate
    OP_R8,     // 8-bit register
    OP_R16,    // 16-bit register
    OP_R32,    // 32-bit register
    OP_R64,    // 64-bit register
    OP_M8,     // 8-bit memory
    OP_M16,    // 16-bit memory
    OP_M32,    // 32-bit memory
    OP_M64,    // 64-bit memory
    OP_SYMBOL, // symbol
};

// kind of register
enum RegisterKind
{
    REG_AL,  // al
    REG_CL,  // cl
    REG_DL,  // dl
    REG_BL,  // bl
    REG_SPL, // spl
    REG_BPL, // bpl
    REG_SIL, // sil
    REG_DIL, // dil
    REG_AX,  // ax
    REG_CX,  // cx
    REG_DX,  // dx
    REG_BX,  // bx
    REG_SP,  // sp
    REG_BP,  // bp
    REG_SI,  // si
    REG_DI,  // di
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
    REG_R8,  // r8
    REG_R9,  // r9
    REG_R10, // r10
    REG_R11, // r11
    REG_R12, // r12
    REG_R13, // r13
    REG_R14, // r14
    REG_R15, // r15
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
