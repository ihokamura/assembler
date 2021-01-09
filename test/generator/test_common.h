#ifndef __TEST_COMMON_H__
#define __TEST_COMMON_H__

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef struct ImmediateInfo ImmediateInfo;
typedef struct RegisterInfo RegisterInfo;

struct ImmediateInfo
{
    size_t size;
    intmax_t sint_max_value;
};

struct RegisterInfo
{
    const char *name;
    size_t size;
    size_t index;
};

extern const RegisterInfo reg_list[];
extern const size_t REG_LIST_SIZE;
extern const ImmediateInfo imm_list[];
extern const size_t IMM_LIST_SIZE;

void put_line(FILE *fp, const char *format, ...);
void put_line_with_tab(FILE *fp, const char *format, ...);
size_t convert_size_to_index(size_t size);
size_t convert_size_to_bit(size_t size);
uintmax_t convert_size_to_sint_max_value(size_t size);
const char *get_size_specifier(size_t size);
const char *get_1st_argument_register(size_t size);
const char *get_2nd_argument_register(size_t size);
const char *get_working_register(const size_t *index_list, size_t list_size);
const char *generate_save_register(FILE *fp, const size_t *index_list, size_t list_size);
void generate_restore_register(FILE *fp, const size_t *index_list, size_t list_size, const char *work_reg);
void generate_test(const char *filename, size_t stack_size, void (*generate_all_test_case)(FILE *));

#endif /* !__TEST_COMMON_H__ */
