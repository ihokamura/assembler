#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "test_common.h"

typedef struct JccInfo JccInfo;

struct JccInfo
{
    const char *mnemonic;
    intmax_t lhs;
    intmax_t rhs;
    bool result;
};

static const JccInfo jcc_info_list[] =
{
    {"jb", 0x01, 0x00, false},
    {"jb", 0x01, 0x01, false},
    {"jb", 0x00, 0x01, true},
    {"jbe", 0x01, 0x00, false},
    {"jbe", 0x01, 0x01, true},
    {"jbe", 0x00, 0x01, true},
    {"je", 0x01, 0x00, false},
    {"je", 0x01, 0x01, true},
    {"je", 0x00, 0x01, false},
    {"jnae", 0x01, 0x00, false},
    {"jnae", 0x01, 0x01, false},
    {"jnae", 0x00, 0x01, true},
    {"jne", 0x01, 0x00, true},
    {"jne", 0x01, 0x01, false},
    {"jne", 0x00, 0x01, true},
};
static const size_t JCC_INFO_LIST_SIZE = sizeof(jcc_info_list) / sizeof(jcc_info_list[0]);


static size_t get_jmp_dest_index(void)
{
    static size_t index = 0;

    return index++;
}


static void generate_test_case_jcc_rel(FILE *fp, size_t rel_size, const JccInfo *jcc_info)
{
    static const uint64_t expected = 1;
    static const uint64_t wrong = 2;
    const size_t index = get_jmp_dest_index();

    put_line_with_tab(fp, "mov rdi, 0x%llu", expected);
    put_line_with_tab(fp, "mov rsi, 0x%llu", jcc_info->result ? wrong : expected);
    put_line_with_tab(fp, "mov rax, 0x%llu", jcc_info->lhs);
    put_line_with_tab(fp, "mov rcx, 0x%llu", jcc_info->rhs);
    put_line_with_tab(fp, "cmp rax, rcx");
    put_line_with_tab(fp, "%s jmp_dest_%lu    # test target", jcc_info->mnemonic, index);
    put_line_with_tab(fp, "call assert_equal_uint64");
    put_line_with_tab(fp, "jmp jmp_end_%lu", index);
    // adjust offset
    for(size_t i = 0; i < rel_size; i++)
    {
        put_line_with_tab(fp, "nop");
    }
    put_line(fp, "jmp_dest_%lu:", index);
    put_line_with_tab(fp, "mov rdi, 0x%llu", expected);
    put_line_with_tab(fp, "mov rsi, 0x%llu", jcc_info->result ? expected : wrong);
    put_line_with_tab(fp, "call assert_equal_uint64");
    put_line(fp, "jmp_end_%lu:", index);
}


static void generate_all_test_case_jcc(FILE *fp)
{
    // <mnemonic> rel
    for(size_t i = 0; i < JCC_INFO_LIST_SIZE; i++)
    {
        size_t rel_size = 256;
        const JccInfo *jcc_info = &jcc_info_list[i];
        generate_test_case_jcc_rel(fp, rel_size, jcc_info);
        put_line(fp, "");
    }
}


void generate_test_jcc(void)
{
    generate_test("test/test_jcc.s", STACK_ALIGNMENT, generate_all_test_case_jcc);
}
