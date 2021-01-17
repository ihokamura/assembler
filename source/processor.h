#ifndef __PROCESSOR_H__
#define __PROCESSOR_H__

#include <stdbool.h>

#include "buffer.h"
#include "elf_wrap.h"
#include "section.h"

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
    MN_ADD,
    MN_AND,
    MN_CALL,
    MN_MOV,
    MN_NOP,
    MN_OR,
    MN_POP,
    MN_PUSH,
    MN_RET,
    MN_SUB,
    MN_XOR,
};

// kind of operand
enum OperandKind
{
    OP_IMM8,   // 8-bit immediate
    OP_IMM16,  // 16-bit immediate
    OP_IMM32,  // 32-bit immediate
    OP_IMM64,  // 64-bit immediate
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
    REG_AL,
    REG_CL,
    REG_DL,
    REG_BL,
    REG_SPL,
    REG_BPL,
    REG_SIL,
    REG_DIL,
    REG_R8B,
    REG_R9B,
    REG_R10B,
    REG_R11B,
    REG_R12B,
    REG_R13B,
    REG_R14B,
    REG_R15B,
    REG_AX,
    REG_CX,
    REG_DX,
    REG_BX,
    REG_SP,
    REG_BP,
    REG_SI,
    REG_DI,
    REG_R8W,
    REG_R9W,
    REG_R10W,
    REG_R11W,
    REG_R12W,
    REG_R13W,
    REG_R14W,
    REG_R15W,
    REG_EAX,
    REG_ECX,
    REG_EDX,
    REG_EBX,
    REG_ESP,
    REG_EBP,
    REG_ESI,
    REG_EDI,
    REG_R8D,
    REG_R9D,
    REG_R10D,
    REG_R11D,
    REG_R12D,
    REG_R13D,
    REG_R14D,
    REG_R15D,
    REG_RAX,
    REG_RCX,
    REG_RDX,
    REG_RBX,
    REG_RSP,
    REG_RBP,
    REG_RSI,
    REG_RDI,
    REG_R8,
    REG_R9,
    REG_R10,
    REG_R11,
    REG_R12,
    REG_R13,
    REG_R14,
    REG_R15,
    REG_RIP,
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
        uintmax_t immediate; // immediate value
        RegisterKind reg;    // kind of register
        const char *symbol;  // body of symbol
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

void generate_data(const Data *data, ByteBufferType *data_body);
void generate_operation(const Operation *operation, ByteBufferType *text_body);

#endif /* !__PROCESSOR_H__ */
