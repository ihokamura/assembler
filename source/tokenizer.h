#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stddef.h>

typedef enum TokenKind TokenKind;
typedef struct Token Token;

// kind of token
enum TokenKind
{
    TK_RESERVED,   // reserved token (punctuator, size-specifier or directive)
    TK_IDENTIFIER, // identifier
    TK_MNEMONIC,   // mnemonic
    TK_IMMEDIATE,  // immediate
    TK_REGISTER,   // register
    TK_STRING,     // string-literal
};

// structure for token
struct Token
{
    TokenKind kind;  // kind of token
    char *str;       // pointer to token string
    size_t len;      // length of token string
    uintmax_t value; // value of token (only for TK_IMMEDIATE)
};

bool peek_reserved(const char *str);
bool peek_token(TokenKind kind, Token **token);
bool consume_reserved(const char *str);
bool consume_token(TokenKind kind, Token **token);
Token *get_token(void);
void set_token(Token *token);
void expect_reserved(const char *str);
Token *expect_token(TokenKind kind);
void tokenize(char *str);
bool at_eof(void);
char *make_identifier(const Token *token);
char *read_file(const char *path);
void report_warning(const char *loc, const char *fmt, ...);
void report_error(const char *loc, const char *fmt, ...);
int convert_escape_sequence(const char *str, int *value);

#endif /* !TOKENIZER_H */
