#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "test_common.h"

#define RFLAGS_MASK_NONE  (0x00)
#define RFLAGS_MASK_CF    (0x01)
#define RFLAGS_MASK_ZF    (0x40)
#define RFLAGS_MASK_SF    (0x80)

typedef struct CmpInfo CmpInfo;

struct CmpInfo
{
    intmax_t lhs;
    intmax_t rhs;
    uint64_t flags;
};

static const CmpInfo cmp_info_list[] =
{
    {0x01, 0x00, RFLAGS_MASK_NONE},
    {0x01, 0x01, RFLAGS_MASK_ZF},
    {0x00, 0x01, RFLAGS_MASK_SF | RFLAGS_MASK_CF},
};
static const size_t CMP_INFO_LIST_SIZE = sizeof(cmp_info_list) / sizeof(cmp_info_list[0]);


static void generate_test_case_cmp_reg_imm(FILE *fp, const CmpInfo *cmp_info, const RegisterInfo *reg_info)
{
    size_t index_list[] = {reg_info->index};
    const char *reg = reg_info->name;
    uint64_t flags = cmp_info->flags;

    const char *work_reg = generate_save_register(fp, index_list, sizeof(index_list) / sizeof(index_list[0]));
    put_line_with_tab(fp, "mov %s, 0x%llx", reg, cmp_info->lhs);
    put_line_with_tab(fp, "cmp %s, 0x%llx    # test target", reg, cmp_info->rhs);
    generate_restore_register(fp, work_reg); // mov does not affect flags
    put_line_with_tab(fp, "pushfq");
    put_line_with_tab(fp, "pop rsi");
    put_line_with_tab(fp, "and rsi, 0x%llx", flags);
    put_line_with_tab(fp, "mov rdi, 0x%llx", flags);
    put_line_with_tab(fp, "call assert_equal_uint64");
}


static void generate_test_case_cmp_reg_reg(FILE *fp, const CmpInfo *cmp_info, const RegisterInfo *reg1_info, const RegisterInfo *reg2_info)
{
    assert(reg1_info->size == reg2_info->size);
    size_t index_list[] = {reg1_info->index, reg2_info->index};
    const char *reg1 = reg1_info->name;
    const char *reg2 = reg2_info->name;
    uint64_t flags = (reg1 == reg2) ? RFLAGS_MASK_ZF : cmp_info->flags;

    const char *work_reg = generate_save_register(fp, index_list, sizeof(index_list) / sizeof(index_list[0]));
    put_line_with_tab(fp, "mov %s, 0x%llx", reg1, cmp_info->lhs);
    put_line_with_tab(fp, "mov %s, 0x%llx", reg2, cmp_info->rhs);
    put_line_with_tab(fp, "cmp %s, %s    # test target", reg1, reg2);
    generate_restore_register(fp, work_reg); // mov does not affect flags
    put_line_with_tab(fp, "pushfq");
    put_line_with_tab(fp, "pop rsi");
    put_line_with_tab(fp, "and rsi, 0x%llx", flags);
    put_line_with_tab(fp, "mov rdi, 0x%llx", flags);
    put_line_with_tab(fp, "call assert_equal_uint64");
}


static void generate_test_case_cmp_reg_mem(FILE *fp, const CmpInfo *cmp_info, const RegisterInfo *reg_info)
{
    size_t size = reg_info->size;
    size_t index_list[] = {reg_info->index};
    const char *reg = reg_info->name;
    const char *size_spec = get_size_specifier(size);
    size_t offset = 16;
    uint64_t flags = cmp_info->flags;

    const char *work_reg = generate_save_register(fp, index_list, sizeof(index_list) / sizeof(index_list[0]));
    put_line_with_tab(fp, "mov %s, 0x%llx", reg, cmp_info->lhs);
    put_line_with_tab(fp, "mov %s [%s-%lu], 0x%llx", size_spec, work_reg, offset, cmp_info->rhs);
    put_line_with_tab(fp, "cmp %s, %s [%s-%lu]    # test target", reg, size_spec, work_reg, offset);
    generate_restore_register(fp, work_reg); // mov does not affect flags
    put_line_with_tab(fp, "pushfq");
    put_line_with_tab(fp, "pop rsi");
    put_line_with_tab(fp, "and rsi, 0x%llx", flags);
    put_line_with_tab(fp, "mov rdi, 0x%llx", flags);
    put_line_with_tab(fp, "call assert_equal_uint64");
}


static void generate_test_case_cmp_mem_imm(FILE *fp, const CmpInfo *cmp_info, size_t size)
{
    const char *size_spec = get_size_specifier(size);
    uint64_t flags = cmp_info->flags;

    put_line_with_tab(fp, "mov %s [rbp-8], 0x%llx", size_spec, cmp_info->lhs);
    put_line_with_tab(fp, "cmp %s [rbp-8], 0x%llx    # test target", size_spec, cmp_info->rhs);
    put_line_with_tab(fp, "pushfq");
    put_line_with_tab(fp, "pop rsi");
    put_line_with_tab(fp, "and rsi, 0x%llx", flags);
    put_line_with_tab(fp, "mov rdi, 0x%llx", flags);
    put_line_with_tab(fp, "call assert_equal_uint64");
}


static void generate_test_case_cmp_mem_reg(FILE *fp, const CmpInfo *cmp_info, const RegisterInfo *reg_info)
{
    size_t size = reg_info->size;
    size_t index_list[] = {reg_info->index};
    const char *reg = reg_info->name;
    const char *size_spec = get_size_specifier(size);
    size_t offset = 16;
    uint64_t flags = cmp_info->flags;

    const char *work_reg = generate_save_register(fp, index_list, sizeof(index_list) / sizeof(index_list[0]));
    put_line_with_tab(fp, "mov %s [%s-%lu], 0x%llx", size_spec, work_reg, offset, cmp_info->lhs);
    put_line_with_tab(fp, "mov %s, 0x%llx", reg, cmp_info->rhs);
    put_line_with_tab(fp, "cmp %s [%s-%lu], %s    # test target", size_spec, work_reg, offset, reg);
    generate_restore_register(fp, work_reg); // mov does not affect flags
    put_line_with_tab(fp, "pushfq");
    put_line_with_tab(fp, "pop rsi");
    put_line_with_tab(fp, "and rsi, 0x%llx", flags);
    put_line_with_tab(fp, "mov rdi, 0x%llx", flags);
    put_line_with_tab(fp, "call assert_equal_uint64");
}


static void generate_all_test_case_cmp(FILE *fp)
{
    // cmp reg, imm
    for(size_t i = 0; i < REG_LIST_SIZE; i++)
    {
        const RegisterInfo *reg_info = &reg_list[i];
        for(size_t j = 0; j < IMM_LIST_SIZE; j++)
        {
            const ImmediateInfo *imm_info = &imm_list[j];
            if((reg_info->size >= imm_info->size) && (imm_info->size <= sizeof(uint32_t)))
            {
                for(size_t k = 0; k < CMP_INFO_LIST_SIZE; k++)
                {
                    const CmpInfo *cmp_info = &cmp_info_list[k];
                    generate_test_case_cmp_reg_imm(fp, cmp_info, reg_info);
                    put_line(fp, "");
                }
            }
        }
    }

    // cmp reg, reg
    for(size_t i = 0; i < REG_LIST_SIZE; i++)
    {
        const RegisterInfo *reg1_info = &reg_list[i];
        for(size_t j = 0; j < REG_LIST_SIZE; j++)
        {
            const RegisterInfo *reg2_info = &reg_list[j];
            if(reg1_info->size == reg2_info->size)
            {
                for(size_t k = 0; k < CMP_INFO_LIST_SIZE; k++)
                {
                    const CmpInfo *cmp_info = &cmp_info_list[k];
                    generate_test_case_cmp_reg_reg(fp, cmp_info, reg1_info, reg2_info);
                    put_line(fp, "");
                }
            }
        }
    }

    // cmp reg, mem
    for(size_t i = 0; i < REG_LIST_SIZE; i++)
    {
        const RegisterInfo *reg_info = &reg_list[i];
        for(size_t k = 0; k < CMP_INFO_LIST_SIZE; k++)
        {
            const CmpInfo *cmp_info = &cmp_info_list[k];
            generate_test_case_cmp_reg_mem(fp, cmp_info, reg_info);
            put_line(fp, "");
        }
    }

    // cmp mem, imm
    for(size_t j = 0; j < IMM_LIST_SIZE; j++)
    {
        const ImmediateInfo *imm_info = &imm_list[j];
        size_t size = imm_info->size;
        if(size <= sizeof(uint32_t))
        {
            for(size_t k = 0; k < CMP_INFO_LIST_SIZE; k++)
            {
                const CmpInfo *cmp_info = &cmp_info_list[k];
                generate_test_case_cmp_mem_imm(fp, cmp_info, size);
                put_line(fp, "");
            }
        }
    }

    // cmp mem, reg
    for(size_t i = 0; i < REG_LIST_SIZE; i++)
    {
        const RegisterInfo *reg_info = &reg_list[i];
        for(size_t k = 0; k < CMP_INFO_LIST_SIZE; k++)
        {
            const CmpInfo *cmp_info = &cmp_info_list[k];
            generate_test_case_cmp_mem_reg(fp, cmp_info, reg_info);
            put_line(fp, "");
        }
    }
}


void generate_test_cmp(void)
{
    generate_test("test/test_cmp.s", STACK_ALIGNMENT, generate_all_test_case_cmp);
}
