#include <stddef.h>
#include <stdio.h>

#include "test_common.h"

typedef struct ConvertInfo ConvertInfo;

struct ConvertInfo
{
    const char *mnemonic;
    size_t size;
    intmax_t eax_value;
    intmax_t edx_value;
};

static const ConvertInfo convert_info_list[] = 
{
    {"cwd", sizeof(uint16_t), 0x0000, 0x0000},
    {"cwd", sizeof(uint16_t), 0x8000, 0xffff},
    {"cdq", sizeof(uint32_t), 0x00000000, 0x00000000},
    {"cdq", sizeof(uint32_t), 0x80000000, 0xffffffff},
    {"cqo", sizeof(uint64_t), 0x0000000000000000, 0x0000000000000000},
    {"cqo", sizeof(uint64_t), 0x8000000000000000, 0xffffffffffffffff},
};
static const size_t CONVERT_INFO_LIST_SIZE = sizeof(convert_info_list) / sizeof(convert_info_list[0]);


static void generate_test_case_cwd(FILE *fp, const ConvertInfo *convert_info)
{
    size_t size = convert_info->size;
    const char *arg1 = get_1st_argument_register(size);
    const char *arg2 = get_2nd_argument_register(size);
    const char *eax_reg = get_register_by_index_and_size(REGISTER_INDEX_EAX, size);

    put_line_with_tab(fp, "mov %s, 0x%llx", eax_reg, convert_info->eax_value);
    put_line_with_tab(fp, "%s    # test target", convert_info->mnemonic);
    put_line_with_tab(fp, "push rdx");
    put_line_with_tab(fp, "mov %s, %s", arg2, eax_reg);
    put_line_with_tab(fp, "mov %s, 0x%llx", arg1, convert_info->eax_value);
    put_line_with_tab(fp, "call assert_equal_uint%ld", convert_size_to_bit(size));
    put_line_with_tab(fp, "pop rsi");
    put_line_with_tab(fp, "mov %s, 0x%llx", arg1, convert_info->edx_value);
    put_line_with_tab(fp, "call assert_equal_uint%ld", convert_size_to_bit(size));
}


static void generate_all_test_case_convert(FILE *fp)
{
    // CWD/CDQ/CQO
    for(size_t i = 0; i < CONVERT_INFO_LIST_SIZE; i++)
    {
        const ConvertInfo *convert_info = &convert_info_list[i];
        generate_test_case_cwd(fp, convert_info);
        put_line(fp, "");
    }
}


void generate_test_convert(void)
{
    generate_test("test/test_convert.s", 0, generate_all_test_case_convert);
}
