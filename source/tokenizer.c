#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "processor.h"
#include "tokenizer.h"

typedef struct ReservedWordInfo ReservedWordInfo;

struct ReservedWordInfo

{
    const char **list; // list of reserved words (longer strings should be followed by shorter strings)
    size_t size;       // size of list
};

#include "list.h"
define_list(Token)
define_list_operations(Token)

// function prototype
static Token *new_token(TokenKind kind, char *str, int len);
static int is_space(const char *str);
static int is_comment(const char *str);
static int is_reserved(const char *str);
static int is_mnemonic(const char *str);
static int is_identifier(const char *str);
static int is_string(const char *str);
static int is_register(const char *str);
static int is_immediate(const char *str, uintmax_t *value);
static int is_octal_digit(int character);
static int is_hexadeciaml_digit(int character);
static int parse_escape_sequence(const char *str);
static uintmax_t convert_immediate(const char *start, int base);
static void report_position(const char *loc);


// global variable
// list of punctuators
static const char *punctuator_list[] = {
    "\n",
    "+",
    ",",
    "-",
    ":",
    "[",
    "]",
};
// list of size specifiers
static const char *size_specifier_list[] = {
    "byte ptr",
    "word ptr",
    "dword ptr",
    "qword ptr",
};
// list of directives
static const char *directive_list[] = {
    ".align",
    ".bss",
    ".byte",
    ".data",
    ".globl",
    ".intel_syntax noprefix",
    ".long",
    ".quad",
    ".string",
    ".text",
    ".word",
    ".zero",
};
// information on reserved words
static const ReservedWordInfo reserved_word_info[] = 
{
    {punctuator_list, sizeof(punctuator_list) / sizeof(punctuator_list[0])},
    {size_specifier_list, sizeof(size_specifier_list) / sizeof(size_specifier_list[0])},
    {directive_list, sizeof(directive_list) / sizeof(directive_list[0])},
};
static size_t RESERVED_WORD_INFO_SIZE = sizeof(reserved_word_info) / sizeof(reserved_word_info[0]); // size of information on reserved words
// map of simple escape sequences (excluding "\")
static const struct {int character; int value;} simple_escape_sequence_map[] = {
    {'\'', '\''},
    {'\"', '\"'},
    {'\?', '\?'},
    {'\\', '\\'},
    {'a', '\a'},
    {'b', '\b'},
    {'f', '\f'},
    {'n', '\n'},
    {'r', '\r'},
    {'t', '\t'},
    {'v', '\v'},
};
static const size_t SIMPLE_ESCAPE_SEQUENCE_SIZE = sizeof(simple_escape_sequence_map) / sizeof(simple_escape_sequence_map[0]); // number of simple escape sequences
static char *user_input; // input of assembler
static List(Token) *token_list; // list of tokens
static ListEntry(Token) *current_token; // currently parsing token
static const char *file_name; // name of source file


/*
make a new token and concatenate it to the current token
*/
static Token *new_token(TokenKind kind, char *str, int len)
{
    Token *token = calloc(1, sizeof(Token));
    token->kind = kind;
    token->str = str;
    token->len = len;
    token->value = 0;

    return token;
}


/*
peek a reserved string
* If the next token is a given string, this function returns true.
* Otherwise, it returns false.
*/
bool peek_reserved(const char *str)
{
    Token *current = get_element(Token)(current_token);

    return (
           (current->kind == TK_RESERVED)
        && (current->len == strlen(str))
        && (strncmp(current->str, str, current->len) == 0));
}


/*
peek a token
* If the next token is a given kind of token, this function returns the token by argument and true by value.
* Otherwise, it returns false.
*/
bool peek_token(TokenKind kind, Token **token)
{
    Token *current = get_element(Token)(current_token);

    if(current->kind == kind)
    {
        *token = current;
        return true;
    }
    else
    {
        return false;
    }
}


/*
consume a reserved string
* If the next token is a given string, this function parses the token and returns true.
* Otherwise, it returns false.
*/
bool consume_reserved(const char *str)
{
    if(!peek_reserved(str))
    {
        return false;
    }

    current_token = next_entry(Token, current_token);

    return true;
}


/*
consume a token
* If the next token is a given kind of token, this function parses and returns the token by argument and true by value.
* Otherwise, it returns false.
*/
bool consume_token(TokenKind kind, Token **token)
{
    if(!peek_token(kind, token))
    {
        return false;
    }

    current_token = next_entry(Token, current_token);

    return true;
}


/*
get the currently parsing token
*/
Token *get_token(void)
{
    return get_element(Token)(current_token);
}


/*
set the currently parsing token
*/
void set_token(Token *token)
{
    for_each_entry(Token, cursor, token_list)
    {
        if(get_element(Token)(cursor) == token)
        {
            current_token = cursor;
            return;
        }
    }
}


/*
parse a reserved string
* If the next token is a given string, this function parses the token.
* Otherwise, it reports an error.
*/
void expect_reserved(const char *str)
{
    if(!consume_reserved(str))
    {
        report_error(get_element(Token)(current_token)->str, "expected '%s'.", str);
    }
}


/*
parse a token
* If the next token is an expected kind, this function parses and returns the token.
* Otherwise, it reports an error.
*/
Token *expect_token(TokenKind kind)
{
    Token *current = get_element(Token)(current_token);

    if(current->kind != kind)
    {
        const char *message;
        switch(kind)
        {
        case TK_IDENTIFIER:
            message = "expected an identifier.";
            break;

        case TK_IMMEDIATE:
            message = "expected an immediate.";
            break;

        case TK_REGISTER:
            message = "expected a register.";
            break;

        case TK_STRING:
            message = "expected a string-literal.";
            break;

        default:
            break;
        }
        report_error(current->str, message);
    }

    current_token = next_entry(Token, current_token);

    return current;
}


/*
tokenize a given string
*/
void tokenize(char *str)
{
    // save input
    user_input = str;

    // initialize token stream
    token_list = new_list(Token)();

    while(*str)
    {
        int len;

        // ignore space
        len = is_space(str);
        if(len > 0)
        {
            str += len;
            continue;
        }

        // ignore comment
        len = is_comment(str);
        if(len > 0)
        {
            str += len;
            continue;
        }

        // parse a reserved string
        len = is_reserved(str);
        if(len > 0)
        {
            Token *token = new_token(TK_RESERVED, str, len);
            current_token = add_list_entry_tail(Token)(token_list, token);
            str += len;
            continue;
        }

        // parse a mnemonic
        len = is_mnemonic(str);
        if(len > 0)
        {
            Token *token = new_token(TK_MNEMONIC, str, len);
            current_token = add_list_entry_tail(Token)(token_list, token);
            str += len;
            continue;
        }

        // parse a register
        len = is_register(str);
        if(len > 0)
        {
            Token *token = new_token(TK_REGISTER, str, len);
            current_token = add_list_entry_tail(Token)(token_list, token);
            str += len;
            continue;
        }

        // parse a string-literal
        len = is_string(str);
        if(len > 0)
        {
            Token *token = new_token(TK_STRING, str + 1, len - 2);
            current_token = add_list_entry_tail(Token)(token_list, token);
            str += len;
            continue;
        }

        // parse an identifier
        len = is_identifier(str);
        if(len > 0)
        {
            Token *token = new_token(TK_IDENTIFIER, str, len);
            current_token = add_list_entry_tail(Token)(token_list, token);
            str += len;
            continue;
        }

        // parse an immediate
        uintmax_t value;
        len = is_immediate(str, &value);
        if(len > 0)
        {
            Token *token = new_token(TK_IMMEDIATE, str, len);
            current_token = add_list_entry_tail(Token)(token_list, token);
            token->value = value;
            str += len;
            continue;
        }

        // Other characters are not accepted as a token.
        report_error(str, "cannot tokenize.");
    }

    // reset the currently parsing token
    current_token = get_first_entry(Token)(token_list);
}


/*
check if the current token is the end of input
*/
bool at_eof(void)
{
    return end_iteration(Token)(token_list, current_token);
}


/*
make string of an identifier
*/
char *make_identifier(const Token *token)
{
    char *identifier = calloc(token->len + 1, sizeof(char));

    return strncpy(identifier, token->str, token->len);
}


/*
read source code from a file
*/
char *read_file(const char *path)
{
    // open source file
    FILE *stream = fopen(path, "r");
    if(stream == NULL)
    {
        fprintf(stderr, "cannot open %s: %s\n", path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    // get length of source code
    if(fseek(stream, 0, SEEK_END) == -1)
    {
        fprintf(stderr, "%s: fseek: %s\n", path, strerror(errno));
        exit(EXIT_FAILURE);
    }
    size_t size = ftell(stream);
    if(fseek(stream, 0, SEEK_SET) == -1)
    {
        fprintf(stderr, "%s: fseek: %s\n", path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    // read source code and make it end with "\n\0"
    char *buffer = calloc(size + 2, sizeof(char));
    fread(buffer, size, 1, stream);
    if((size == 0) || (buffer[size - 1] != '\n'))
    {
        buffer[size] = '\n';
        buffer[size + 1] = '\0';
    }
    else
    {
        buffer[size] = '\0';
    }

    // save name of source file
    file_name = path;

    return buffer;
}


/*
report a warning
*/
void report_warning(const char *loc, const char *fmt, ...)
{
    // report the position where an error is detected
    const char *pos = (loc == NULL ? get_token()->str : loc);
    report_position(pos);

    // print the message
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
}


/*
report an error
* This function never returns.
*/
void report_error(const char *loc, const char *fmt, ...)
{
    // report the position where an error is detected
    const char *pos = (loc == NULL ? get_token()->str : loc);
    report_position(pos);

    // print the message
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);

    // stop compiling
    exit(1);
}


/*
check if the following string is space
*/
static int is_space(const char *str)
{
    int len = 0;

    while(isspace(str[len]))
    {
        len++;
    }

    return len;
}


/*
check if the following string is comment
*/
static int is_comment(const char *str)
{
    int len = 0;

    if(str[len] == '#')
    {
        while(str[len] != '\n')
        {
            len++;
        }
    }

    return len;
}


/*
check if the following string is reserved
*/
static int is_reserved(const char *str)
{
    for(size_t i = 0; i < RESERVED_WORD_INFO_SIZE; i++)
    {
        const ReservedWordInfo *info = &reserved_word_info[i];
        for(size_t j = 0; j < info->size; j++)
        {
            const char *word = info->list[j];
            size_t len = strlen(word);
            if(strncmp(str, word, len) == 0)
            {
                return len;
            }
        }
    }

    return 0;
}


/*
check if the following string is a mnemonic
*/
static int is_mnemonic(const char *str)
{
    // check mnemonics
    for(size_t i = 0; i < MNEMONIC_INFO_LIST_SIZE; i++)
    {
        const char *mnemonic = mnemonic_info_list[i].name;
        size_t len = strlen(mnemonic);
        if((strncmp(str, mnemonic, len) == 0) && (!isalnum(str[len]) && (str[len] != '_')))
        {
            return len;
        }
    }

    return 0;
}


/*
check if the following string is an identifier
*/
static int is_identifier(const char *str)
{
    static const char *other_chars = "_.";
    int len = 0;

    if(isalpha(*str) || strchr(other_chars, *str))
    {
        len++;
        str++;

        // there may be a digit after second character
        while(isalnum(*str) || strchr(other_chars, *str))
        {
            len++;
            str++;
        }
    }

    return len;
}


/*
check if the following string is a string-literal
*/
static int is_string(const char *str)
{
    int len = 0;

    if(*str == '"')
    {
        len++;
        while((str[len] != '\0') && (str[len] != '"'))
        {
            if(str[len] == '\\')
            {
                len++;
                len += parse_escape_sequence(&str[len]);
            }
            else
            {
                len++;
            }
        }

        if(str[len] == '"')
        {
            len++;
        }
        else
        {
            report_error(str, "expected '\"'");
        }
    }

    return len;
}


/*
check if the following string is a register
*/
static int is_register(const char *str)
{
    for(size_t i = 0; i < REGISTER_INFO_LIST_SIZE; i++)
    {
        const char *reg = register_info_list[i].name;
        size_t len = strlen(reg);
        if((strncmp(str, reg, len) == 0) && (!isalnum(str[len]) && (str[len] != '_')))
        {
            return len;
        }
    }

    return 0;
}


/*
check if the following string is an immediate
*/
static int is_immediate(const char *str, uintmax_t *value)
{
    int len = 0;
    int base;
    const char *start;
    int (*check_digit)(int);

    // check sign
    if(strchr("+-", str[len]) != NULL)
    {
        len++;
    }

    // check prefix
    if((strncmp(&str[len], "0x", 2) == 0) || (strncmp(&str[len], "0X", 2) == 0))
    {
        len += 2;
        base = 16;
        check_digit = is_hexadeciaml_digit;
    }
    else if(str[len] == '0')
    {
        base = 8;
        check_digit = is_octal_digit;
    }
    else
    {
        base = 10;
        check_digit = isdigit;
    }

    // consume digits
    start = &str[len];
    while(check_digit(str[len]))
    {
        len++;
    }

    *value = convert_immediate(start, base);

    return len;
}



/*
check if the character is an octal digit
*/
static int is_octal_digit(int character)
{
    return (strchr("01234567", character) != NULL);
}


/*
check if the character is a hexadecimal digit
*/
static int is_hexadeciaml_digit(int character)
{
    return (strchr("0123456789abcdefABCDEF", character) != NULL);
}


/*
parse an escape sequence
*/
static int parse_escape_sequence(const char *str)
{
    int len = 0;

    // check simple escape sequence
    for(size_t i = 0; i < SIMPLE_ESCAPE_SEQUENCE_SIZE; i++)
    {
        if(str[len] == simple_escape_sequence_map[i].character)
        {
            len++;
            break;
        }
    }
    if(len > 0)
    {
        return len;
    }

    // check octal escape sequence
    // An octal escape sequence consists of up to 3 octal digits.
    for(size_t count = 0; count < 3; count++)
    {
        if(is_octal_digit(str[len]))
        {
            len++;
        }
    }
    if(len > 0)
    {
        return len;
    }

    report_error(str, "unknown escape sequence");
    return len;
}


/*
convert an escape sequence
*/
int convert_escape_sequence(const char *str, int *value)
{
    int len = 1;

    if(str[0] == '\\')
    {
        len++;
        int character = str[1];
        for(size_t i = 0; i < SIMPLE_ESCAPE_SEQUENCE_SIZE; i++)
        {
            if(character == simple_escape_sequence_map[i].character)
            {
                *value =simple_escape_sequence_map[i].value;
                break;
            }
        }
    }
    else
    {
        *value = str[0];
    }

    return len;
}


/*
convert an immediate
*/
static uintmax_t convert_immediate(const char *start, int base)
{
    // note that the converted string always represents positive value
    uintmax_t value = strtoumax(start, NULL, base);

    // handle invalid case
    if(errno == ERANGE)
    {
        report_warning(start, "immediate value is too large");
    }

    return value;
}


/*
report the position at which a warning or an error is detected
*/
static void report_position(const char *loc)
{
    // search the first and last character of the line including the given location
    int start_pos = 0;
    while((user_input < &loc[start_pos]) && (loc[start_pos-1] != '\n'))
    {
        start_pos--;
    }
    int end_pos = 0;
    while(loc[end_pos] != '\n')
    {
        end_pos++;
    }

    // search the line number including the given location
    int line_number = 1;
    for(char *p = user_input; p < &loc[start_pos]; p++)
    {
        if(*p == '\n')
        {
            line_number++;
        }
    }

    // output file name, line number and the line including the given location
    int indent = fprintf(stderr, "%s:%d: ", file_name, line_number);
    fprintf(stderr, "%.*s\n", end_pos - start_pos, &loc[start_pos]);
    int pos = indent + (-start_pos);

    // emphasize the position
    fprintf(stderr, "%*s^\n", pos, "");
}
