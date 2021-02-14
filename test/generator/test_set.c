#include <stddef.h>
#include <stdio.h>

#include "test_common.h"

typedef struct SetInfo SetInfo;

struct SetInfo
{
    const char *mnemonic;
    intmax_t lhs;
    intmax_t rhs;
    uint8_t result;
};

static const SetInfo set_info_list[] =
{
    {"seta", 0x01, 0x00, 0x01},
    {"seta", 0x01, 0x01, 0x00},
    {"seta", 0x00, 0x01, 0x00},
    {"setae", 0x01, 0x00, 0x01},
    {"setae", 0x01, 0x01, 0x01},
    {"setae", 0x00, 0x01, 0x00},
    {"setb", 0x01, 0x00, 0x00},
    {"setb", 0x01, 0x01, 0x00},
    {"setb", 0x00, 0x01, 0x01},
    {"setbe", 0x01, 0x00, 0x00},
    {"setbe", 0x01, 0x01, 0x01},
    {"setbe", 0x00, 0x01, 0x01},
    {"sete", 0x01, 0x00, 0x00},
    {"sete", 0x01, 0x01, 0x01},
    {"sete", 0x00, 0x01, 0x00},
    {"setg", 0x01, 0x00, 0x01},
    {"setg", 0x01, 0x01, 0x00},
    {"setg", 0x00, 0x01, 0x00},
    {"setge", 0x01, 0x00, 0x01},
    {"setge", 0x01, 0x01, 0x01},
    {"setge", 0x00, 0x01, 0x00},
    {"setl", 0x01, 0x00, 0x00},
    {"setl", 0x01, 0x01, 0x00},
    {"setl", 0x00, 0x01, 0x01},
    {"setle", 0x01, 0x00, 0x00},
    {"setle", 0x01, 0x01, 0x01},
    {"setle", 0x00, 0x01, 0x01},
    {"setna", 0x01, 0x00, 0x00},
    {"setna", 0x01, 0x01, 0x01},
    {"setna", 0x00, 0x01, 0x01},
    {"setnae", 0x01, 0x00, 0x00},
    {"setnae", 0x01, 0x01, 0x00},
    {"setnae", 0x00, 0x01, 0x01},
    {"setnb", 0x01, 0x00, 0x01},
    {"setnb", 0x01, 0x01, 0x01},
    {"setnb", 0x00, 0x01, 0x00},
    {"setnbe", 0x01, 0x00, 0x01},
    {"setnbe", 0x01, 0x01, 0x00},
    {"setnbe", 0x00, 0x01, 0x00},
    {"setne", 0x01, 0x00, 0x01},
    {"setne", 0x01, 0x01, 0x00},
    {"setne", 0x00, 0x01, 0x01},
};
static const size_t SET_INFO_LIST_SIZE = sizeof(set_info_list) / sizeof(set_info_list[0]);


static void generate_test_case_set_reg(FILE *fp, const SetInfo *set_info, const RegisterInfo *reg_info)
{
    size_t index_list[] = {reg_info->index};
    const char *reg = reg_info->name;

    const char *work_reg = generate_save_register(fp, index_list, sizeof(index_list) / sizeof(index_list[0]));
    put_line_with_tab(fp, "mov r8, 0x%llx", set_info->lhs);
    put_line_with_tab(fp, "mov r9, 0x%llx", set_info->rhs);
    put_line_with_tab(fp, "cmp r8, r9");
    put_line_with_tab(fp, "%s %s    # test target", set_info->mnemonic, reg);
    put_line_with_tab(fp, "mov sil, %s", reg);
    put_line_with_tab(fp, "mov dil, 0x%x", set_info->result);
    generate_restore_register(fp, work_reg);
    put_line_with_tab(fp, "call assert_equal_uint8");
}


static void generate_test_case_set_mem(FILE *fp, const SetInfo *set_info)
{
    put_line_with_tab(fp, "mov r8, 0x%llx", set_info->lhs);
    put_line_with_tab(fp, "mov r9, 0x%llx", set_info->rhs);
    put_line_with_tab(fp, "cmp r8, r9");
    put_line_with_tab(fp, "%s byte ptr [rbp-8]    # test target", set_info->mnemonic);
    put_line_with_tab(fp, "mov sil, byte ptr [rbp-8]");
    put_line_with_tab(fp, "mov dil, 0x%x", set_info->result);
    put_line_with_tab(fp, "call assert_equal_uint8");
}


static void generate_all_test_case_set(FILE *fp)
{
    // <mnemonic> reg8
    for(size_t i = 0; i < REG_LIST_SIZE; i++)
    {
        const RegisterInfo *reg_info = &reg_list[i];
        if(reg_info->size == sizeof(uint8_t))
        {
            for(size_t k = 0; k < SET_INFO_LIST_SIZE; k++)
            {
                const SetInfo *set_info = &set_info_list[k];
                generate_test_case_set_reg(fp, set_info, reg_info);
                put_line(fp, "");
            }
        }
    }

    // <mnemonic> mem8
    for(size_t k = 0; k < SET_INFO_LIST_SIZE; k++)
    {
        const SetInfo *set_info = &set_info_list[k];
        generate_test_case_set_mem(fp, set_info);
        put_line(fp, "");
    }
}


void generate_test_set(void)
{
    generate_test("test/test_set.s", 0, generate_all_test_case_set);
}
