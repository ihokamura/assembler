#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "test_common.h"

#define vfprintf_wrap(fp, format) \
{\
    va_list ap;\
    va_start(ap, format);\
    vfprintf(fp, format, ap);\
    va_end(ap);\
}

const RegisterInfo reg_list[] = 
{
    // 8-bit registers
    {"al", sizeof(uint8_t), 0},
    {"cl", sizeof(uint8_t), 1},
    {"dl", sizeof(uint8_t), 2},
    {"bl", sizeof(uint8_t), 3},
    {"spl", sizeof(uint8_t), 4},
    {"bpl", sizeof(uint8_t), 5},
    {"sil", sizeof(uint8_t), 6},
    {"dil", sizeof(uint8_t), 7},
    {"r8b", sizeof(uint8_t), 8},
    {"r9b", sizeof(uint8_t), 9},
    {"r10b", sizeof(uint8_t), 10},
    {"r11b", sizeof(uint8_t), 11},
    {"r12b", sizeof(uint8_t), 12},
    {"r13b", sizeof(uint8_t), 13},
    {"r14b", sizeof(uint8_t), 14},
    {"r15b", sizeof(uint8_t), 15},
    // 16-bit registers
    {"ax", sizeof(uint16_t), 0},
    {"cx", sizeof(uint16_t), 1},
    {"dx", sizeof(uint16_t), 2},
    {"bx", sizeof(uint16_t), 3},
    {"sp", sizeof(uint16_t), 4},
    {"bp", sizeof(uint16_t), 5},
    {"si", sizeof(uint16_t), 6},
    {"di", sizeof(uint16_t), 7},
    {"r8w", sizeof(uint16_t), 8},
    {"r9w", sizeof(uint16_t), 9},
    {"r10w", sizeof(uint16_t), 10},
    {"r11w", sizeof(uint16_t), 11},
    {"r12w", sizeof(uint16_t), 12},
    {"r13w", sizeof(uint16_t), 13},
    {"r14w", sizeof(uint16_t), 14},
    {"r15w", sizeof(uint16_t), 15},
    // 32-bit registers
    {"eax", sizeof(uint32_t), 0},
    {"ecx", sizeof(uint32_t), 1},
    {"edx", sizeof(uint32_t), 2},
    {"ebx", sizeof(uint32_t), 3},
    {"esp", sizeof(uint32_t), 4},
    {"ebp", sizeof(uint32_t), 5},
    {"esi", sizeof(uint32_t), 6},
    {"edi", sizeof(uint32_t), 7},
    {"r8d", sizeof(uint32_t), 8},
    {"r9d", sizeof(uint32_t), 9},
    {"r10d", sizeof(uint32_t), 10},
    {"r11d", sizeof(uint32_t), 11},
    {"r12d", sizeof(uint32_t), 12},
    {"r13d", sizeof(uint32_t), 13},
    {"r14d", sizeof(uint32_t), 14},
    {"r15d", sizeof(uint32_t), 15},
    // 64-bit registers
    {"rax", sizeof(uint64_t), 0},
    {"rcx", sizeof(uint64_t), 1},
    {"rdx", sizeof(uint64_t), 2},
    {"rbx", sizeof(uint64_t), 3},
    {"rsp", sizeof(uint64_t), 4},
    {"rbp", sizeof(uint64_t), 5},
    {"rsi", sizeof(uint64_t), 6},
    {"rdi", sizeof(uint64_t), 7},
    {"r8", sizeof(uint64_t), 8},
    {"r9", sizeof(uint64_t), 9},
    {"r10", sizeof(uint64_t), 10},
    {"r11", sizeof(uint64_t), 11},
    {"r12", sizeof(uint64_t), 12},
    {"r13", sizeof(uint64_t), 13},
    {"r14", sizeof(uint64_t), 14},
    {"r15", sizeof(uint64_t), 15},
};
const size_t REG_LIST_SIZE = sizeof(reg_list) / sizeof(reg_list[0]);

const ImmediateInfo imm_list[] = 
{
    {sizeof(uint8_t), INT8_MAX},
    {sizeof(uint16_t), INT16_MAX},
    {sizeof(uint32_t), INT32_MAX},
};
const size_t IMM_LIST_SIZE = sizeof(imm_list) / sizeof(imm_list[0]);


void put_line(FILE *fp, const char *format, ...)
{
    vfprintf_wrap(fp, format);
    fputc('\n', fp);
}


void put_line_with_tab(FILE *fp, const char *format, ...)
{
    fputc('\t', fp);
    vfprintf_wrap(fp, format);
    fputc('\n', fp);
}


size_t convert_size_to_index(size_t size)
{
    switch(size)
    {
    case sizeof(uint8_t):
        return 0;

    case sizeof(uint16_t):
        return 1;

    case sizeof(uint32_t):
        return 2;

    case sizeof(uint64_t):
        return 3;

    default:
        assert(0);
        return 0;
    }
}


size_t convert_size_to_bit(size_t size)
{
    return 8 * size;
}


uintmax_t convert_size_to_sint_max_value(size_t size)
{
    // use INT32_MAX for 64-bit registers
    static const uintmax_t max_values[] = {INT8_MAX, INT16_MAX, INT32_MAX, INT32_MAX};

    return max_values[convert_size_to_index(size)];
}


const char *get_size_specifier(size_t size)
{
    static const char *size_specs[] = {"byte ptr", "word ptr", "dword ptr", "qword ptr"};

    return size_specs[convert_size_to_index(size)];
}


const char *get_1st_argument_register(size_t size)
{
    static const char *regs_edi_set[] = {"dil", "di", "edi", "rdi"};

    return regs_edi_set[convert_size_to_index(size)];
}


const char *get_2nd_argument_register(size_t size)
{
    static const char *regs_esi_set[] = {"sil", "si", "esi", "rsi"};

    return regs_esi_set[convert_size_to_index(size)];
}


const char *get_working_register(const size_t *index_list, size_t list_size)
{
    static const size_t REGISTER_INDEX_EAX_SET = 0;
    static const size_t REGISTER_INDEX_ECX_SET = 1;

    bool use_rax = true;
    bool use_rcx = true;
    for(size_t i = 0; i < list_size; i++)
    {
        size_t index = index_list[i];

        if(index == REGISTER_INDEX_EAX_SET)
        {
            use_rax = false;
        }
        if(index == REGISTER_INDEX_ECX_SET)
        {
            use_rcx = false;
        }
    }

    if(use_rax)
    {
        return "rax";
    }
    else if(use_rcx)
    {
        return "rcx";
    }
    else
    {
        return "rdx";
    }
}


const char *generate_save_register(FILE *fp, const size_t *index_list, size_t list_size)
{
    // save rsp and rbp
    const char *work_reg = get_working_register(index_list, list_size);
    put_line_with_tab(fp, "mov %s, rbp", work_reg);
    put_line_with_tab(fp, "mov qword ptr [%s-8], rsp", work_reg);

    return work_reg;
}


void generate_restore_register(FILE *fp, const size_t *index_list, size_t list_size, const char *work_reg)
{
    // restore rsp and rbp
    put_line_with_tab(fp, "mov rsp, qword ptr [%s-8]", work_reg);
    put_line_with_tab(fp, "mov rbp, %s", work_reg);
}


static void generate_prologue(FILE *fp, size_t stack_size)
{
    put_line_with_tab(fp, ".intel_syntax noprefix");
    put_line(fp, "");
    put_line_with_tab(fp, ".globl main");
    put_line(fp, "main:");
    put_line_with_tab(fp, "push rbp");
    put_line_with_tab(fp, "mov rbp, rsp");
    put_line_with_tab(fp, "sub rsp, %lu", stack_size);
    put_line(fp, "");
}


static void generate_epilogue(FILE *fp)
{
    put_line_with_tab(fp, "mov rsp, rbp");
    put_line_with_tab(fp, "pop rbp");
    put_line_with_tab(fp, "ret");
}


void generate_test(const char *filename, size_t stack_size, void (*generate_all_test_case)(FILE *))
{
    FILE *fp = fopen(filename, "w");
    if(fp == NULL)
    {
        fprintf(stderr, "cannot open %s\n", filename);
        return;
    }

    generate_prologue(fp, stack_size);
    generate_all_test_case(fp);
    generate_epilogue(fp);

    fclose(fp);
}
