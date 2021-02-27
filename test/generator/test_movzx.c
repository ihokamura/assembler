#include <stddef.h>
#include <stdio.h>

#include "test_common.h"


static void generate_test_case_movzx_reg_reg(FILE *fp, const RegisterInfo *reg1_info, const RegisterInfo *reg2_info)
{
    size_t reg1_size = reg1_info->size;
    size_t reg2_size = reg2_info->size;
    size_t index_list[] = {reg1_info->index, reg2_info->index};
    const char *reg1 = reg1_info->name;
    const char *reg2 = reg2_info->name;
    const char *arg1 = get_1st_argument_register(reg1_size);
    const char *arg2 = get_2nd_argument_register(reg1_size);

    const char *work_reg = generate_save_register(fp, index_list, sizeof(index_list) / sizeof(index_list[0]));
    put_line_with_tab(fp, "mov %s, 0x%llx", reg2, convert_size_to_sint_max_plus_1(reg2_size));
    put_line_with_tab(fp, "movzx %s, %s    # test target", reg1, reg2);
    put_line_with_tab(fp, "mov %s, %s", arg2, reg1);
    put_line_with_tab(fp, "mov %s, 0x%llx", arg1, convert_size_to_sint_max_plus_1(reg2_size));
    generate_restore_register(fp, work_reg);
    put_line_with_tab(fp, "call assert_equal_uint%ld", convert_size_to_bit(reg1_size));
}


static void generate_test_case_movzx_reg_mem(FILE *fp, const RegisterInfo *reg_info, size_t mem_size)
{
    size_t reg_size = reg_info->size;
    size_t index_list[] = {reg_info->index};
    const char *reg = reg_info->name;
    const char *arg1 = get_1st_argument_register(reg_size);
    const char *arg2 = get_2nd_argument_register(reg_size);
    const char *reg_size_spec = get_size_specifier(reg_size);
    const char *mem_size_spec = get_size_specifier(mem_size);
    size_t offset = 16;

    const char *work_reg = generate_save_register(fp, index_list, sizeof(index_list) / sizeof(index_list[0]));
    put_line_with_tab(fp, "mov %s [%s-%lu], 0x%llx", reg_size_spec, work_reg, offset, convert_size_to_sint_max_plus_1(mem_size));
    put_line_with_tab(fp, "movzx %s, %s [%s-%lu]    # test target", reg, mem_size_spec, work_reg, offset);
    put_line_with_tab(fp, "mov %s, %s", arg2, reg);
    put_line_with_tab(fp, "mov %s, 0x%llx", arg1, convert_size_to_sint_max_plus_1(mem_size));
    generate_restore_register(fp, work_reg);
    put_line_with_tab(fp, "call assert_equal_uint%ld", convert_size_to_bit(reg_size));
}


static void generate_all_test_case_movzx(FILE *fp)
{
    // MOVZX reg, reg
    for(size_t i = 0; i < REG_LIST_SIZE; i++)
    {
        const RegisterInfo *reg1_info = &reg_list[i];
        for(size_t j = 0; j < REG_LIST_SIZE; j++)
        {
            const RegisterInfo *reg2_info = &reg_list[j];
            if((reg1_info->size > reg2_info->size) && (reg2_info->size <= sizeof(uint16_t)))
            {
                generate_test_case_movzx_reg_reg(fp, reg1_info, reg2_info);
                put_line(fp, "");
            }
        }
    }

    // MOVZX reg, mem
    for(size_t i = 0; i < REG_LIST_SIZE; i++)
    {
        const RegisterInfo *reg_info = &reg_list[i];
        for(size_t j = 0; j < IMM_LIST_SIZE; j++)
        {
            const ImmediateInfo *imm_info = &imm_list[j];
            size_t mem_size = imm_info->size;
            if((reg_info->size > mem_size) && (mem_size <= sizeof(uint16_t)))
            {
                generate_test_case_movzx_reg_mem(fp, reg_info, mem_size);
                put_line(fp, "");
            }
        }
    }
}


void generate_test_movzx(void)
{
    generate_test("test/test_movzx.s", 0, generate_all_test_case_movzx);
}
