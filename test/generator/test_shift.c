#include <stddef.h>
#include <stdio.h>

#include "test_common.h"

typedef struct ShiftOperationInfo ShiftOperationInfo;

struct ShiftOperationInfo
{
    const char *mnemonic;
    uintmax_t lhs;
    uint8_t rhs;
    uintmax_t result;
};


static void generate_test_case_shift_reg_imm(FILE *fp, const ShiftOperationInfo *op_info, const RegisterInfo *reg_info)
{
    size_t size = reg_info->size;
    size_t index_list[] = {reg_info->index};
    const char *reg = reg_info->name;
    const char *arg1 = get_1st_argument_register(size);
    const char *arg2 = get_2nd_argument_register(size);

    const char *work_reg = generate_save_register(fp, index_list, sizeof(index_list) / sizeof(index_list[0]));
    put_line_with_tab(fp, "mov %s, 0x%llx", reg, op_info->lhs);
    put_line_with_tab(fp, "%s %s, 0x%lx    # test target", op_info->mnemonic, reg, op_info->rhs);
    put_line_with_tab(fp, "mov %s, %s", arg2, reg);
    put_line_with_tab(fp, "mov %s, 0x%llx", arg1, op_info->result);
    generate_restore_register(fp, work_reg);
    put_line_with_tab(fp, "call assert_equal_uint%ld", convert_size_to_bit(size));
}


static void generate_test_case_shift_mem_imm(FILE *fp, const ShiftOperationInfo *op_info)
{
    put_line_with_tab(fp, "mov qword ptr [rbp-8], 0x%llx", op_info->lhs);
    put_line_with_tab(fp, "%s qword ptr [rbp-8], 0x%lx    # test target", op_info->mnemonic, op_info->rhs);
    put_line_with_tab(fp, "mov rsi, qword ptr [rbp-8]");
    put_line_with_tab(fp, "mov rdi, 0x%llx", op_info->result);
    put_line_with_tab(fp, "call assert_equal_uint64");
}


static void generate_test_case_shift_reg_reg(FILE *fp, const ShiftOperationInfo *op_info, const RegisterInfo *reg_info)
{
    size_t size = reg_info->size;
    size_t index_list[] = {reg_info->index, REGISTER_INDEX_ECX};
    const char *reg = reg_info->name;
    const char *arg1 = get_1st_argument_register(size);
    const char *arg2 = get_2nd_argument_register(size);

    const char *work_reg = generate_save_register(fp, index_list, sizeof(index_list) / sizeof(index_list[0]));
    put_line_with_tab(fp, "mov %s, 0x%llx", reg, op_info->lhs);
    put_line_with_tab(fp, "mov cl, 0x%llx", op_info->rhs);
    put_line_with_tab(fp, "%s %s, cl    # test target", op_info->mnemonic, reg);
    put_line_with_tab(fp, "mov %s, %s", arg2, reg);
    put_line_with_tab(fp, "mov %s, 0x%llx", arg1, op_info->result);
    generate_restore_register(fp, work_reg);
    put_line_with_tab(fp, "call assert_equal_uint%ld", convert_size_to_bit(size));
}


static void generate_test_case_shift_mem_reg(FILE *fp, const ShiftOperationInfo *op_info)
{
    put_line_with_tab(fp, "mov qword ptr [rbp-8], 0x%llx", op_info->lhs);
    put_line_with_tab(fp, "mov cl, 0x%llx", op_info->rhs);
    put_line_with_tab(fp, "%s qword ptr [rbp-8], cl    # test target", op_info->mnemonic);
    put_line_with_tab(fp, "mov rsi, qword ptr [rbp-8]");
    put_line_with_tab(fp, "mov rdi, 0x%llx", op_info->result);
    put_line_with_tab(fp, "call assert_equal_uint64");
}


static void generate_all_test_case_shift(FILE *fp)
{
    // SAL reg, 1
    for(size_t i = 0; i < REG_LIST_SIZE; i++)
    {
        const RegisterInfo *reg_info = &reg_list[i];
        uintmax_t lhs = 1;
        uint8_t rhs = 1;
        ShiftOperationInfo *op_info = &(ShiftOperationInfo){"sal", lhs, rhs, lhs << rhs};
        generate_test_case_shift_reg_imm(fp, op_info, reg_info);
        put_line(fp, "");
    }

    // SAL mem, 1
    {
        uintmax_t lhs = 1;
        uint8_t rhs = 1;
        ShiftOperationInfo *op_info = &(ShiftOperationInfo){"sal", lhs, rhs, lhs << rhs};
        generate_test_case_shift_mem_imm(fp, op_info);
        put_line(fp, "");
    }

    // SAL reg, cl
    for(size_t i = 0; i < REG_LIST_SIZE; i++)
    {
        const RegisterInfo *reg_info = &reg_list[i];
        uintmax_t lhs = 1;
        uint8_t rhs = 2;
        ShiftOperationInfo *op_info = &(ShiftOperationInfo){"sal", lhs, rhs, (reg_info->index == REGISTER_INDEX_ECX) ? (rhs << rhs) : (lhs << rhs)};
        generate_test_case_shift_reg_reg(fp, op_info, reg_info);
        put_line(fp, "");
    }

    // SAL mem, cl
    {
        uintmax_t lhs = 1;
        uint8_t rhs = 2;
        ShiftOperationInfo *op_info = &(ShiftOperationInfo){"sal", lhs, rhs, lhs << rhs};
        generate_test_case_shift_mem_reg(fp, op_info);
        put_line(fp, "");
    }

    // SAL reg, imm
    for(size_t i = 0; i < REG_LIST_SIZE; i++)
    {
        const RegisterInfo *reg_info = &reg_list[i];
        uintmax_t lhs = 1;
        uint8_t rhs = 2;
        ShiftOperationInfo *op_info = &(ShiftOperationInfo){"sal", lhs, rhs, lhs << rhs};
        generate_test_case_shift_reg_imm(fp, op_info, reg_info);
        put_line(fp, "");
    }

    // SAL mem, imm
    {
        uintmax_t lhs = 1;
        uint8_t rhs = 2;
        ShiftOperationInfo *op_info = &(ShiftOperationInfo){"sal", lhs, rhs, lhs << rhs};
        generate_test_case_shift_mem_imm(fp, op_info);
        put_line(fp, "");
    }
}


void generate_test_shift(void)
{
    generate_test("test/test_shift.s", 0, generate_all_test_case_shift);
}
