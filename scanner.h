#ifndef GURU_SCANNER
#define GURU_SCANNER

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct scanner {
    const char *buf;

    const char *st;
    const char *crnt;

    int line;
};

enum tokent {
    GURU_EOF, GURU_LEFT_PAREN, GURU_RIGHT_PAREN,
    GURU_LEFT_BRACE, GURU_RIGHT_BRACE,
    GURU_COMMA, GURU_DOT, GURU_MINUS, GURU_PLUS,
    GURU_SEMICOLON, GURU_SLASH, GURU_STAR,
    // One or two character tokens.
    GURU_BANG, GURU_BANG_EQUAL,
    GURU_EQUAL, GURU_EQUAL_EQUAL,
    GURU_GREATER, GURU_GREATER_EQUAL,
    GURU_LESS, GURU_LESS_EQUAL,
    // Literals.
    GURU_IDENTIFIER, GURU_STRING, GURU_NUMBER, GURU_INTEGER,
    // Keywords.
    GURU_AND, GURU_CLASS, GURU_ELSE, GURU_FALSE,
    GURU_FOR, GURU_FUN, GURU_IF, GURU_NOTHING, GURU_VOID, GURU_OR,
    GURU_PRINT, GURU_RETURN, GURU_SUPER, GURU_THIS,
    GURU_TRUE, GURU_VAR, GURU_WHILE,
    GURU_ERROR, GURU_CONST, GURU_FROM,
    GURU_STRING_CONCAT, GURU_STRING_EQ,

    GURU_LINE,
};

struct token {
    enum tokent type;
    const uint32_t line;
    const uint16_t pos;

    const size_t len;
    const char *lexeme;
};

struct scanner *scalloc(const char *source);
void scgett(struct scanner *s, struct token *t);
void init_token(struct scanner *s, struct token *t, enum tokent tt);
void init_err_token(struct scanner *s, struct token *t, const char *msg);
#define at_end(s) (*(s)->crnt == '\0')

static void ident_scan_dfa(struct scanner *s, struct token *t);
static void ident_check_kw(struct token *t, const char *rest, uint16_t offset, enum tokent tt);
static uint8_t match(struct scanner *s, char c);
#endif
