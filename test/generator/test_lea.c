#include <stddef.h>
#include <stdio.h>

#include "test_common.h"

static const size_t mem_size_list[] = {sizeof(uint8_t), sizeof(uint16_t), sizeof(uint32_t), sizeof(uint64_t)};
static const size_t MEM_SIZE_LIST_SIZE = sizeof(mem_size_list) / sizeof(mem_size_list[0]);


static void generate_test_case_lea(FILE *fp, const RegisterInfo *reg_info, size_t mem_size)
{
    size_t index_list[] = {reg_info->index};
    const char *reg = reg_info->name;
    const char *work_reg = generate_save_register(fp, index_list, sizeof(index_list) / sizeof(index_list[0]));
    put_line_with_tab(fp, "lea %s, byte ptr [rip+test_external_data_uint%lu] # test target", reg, 8 * mem_size);
    put_line_with_tab(fp, "mov rdi, %s", reg);
    generate_restore_register(fp, work_reg);
    put_line_with_tab(fp, "call assert_pointer_to_external_data_uint%lu", 8 * mem_size);
}


static void generate_all_test_case_lea(FILE *fp)
{
    // LEA reg, mem
    for(size_t i = 0; i < REG_LIST_SIZE; i++)
    {
        const RegisterInfo *reg_info = &reg_list[i];
        if(reg_info->size == sizeof(void *))
        {
            for(size_t j = 0; j < MEM_SIZE_LIST_SIZE; j++)
            {
                generate_test_case_lea(fp, reg_info, mem_size_list[j]);
                put_line(fp, "");
            }
        }
    }
}


void generate_test_lea(void)
{
    generate_test("test/test_lea.s", STACK_ALIGNMENT, generate_all_test_case_lea);
}
