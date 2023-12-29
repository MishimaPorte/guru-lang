#include "scanner.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void init_token(struct scanner *s, struct token *t, enum tokent tt) {
    t->type = tt;
    t->lexeme = s->st;
    *(uint16_t*)&t->pos = (uint16_t) (s->st - s->buf);
    *(size_t*)&t->len = (size_t) (s->crnt - s->st);
    *(uint32_t*)&t->line = s->line;
};

static uint8_t match(struct scanner *s, char c) {
    if (at_end(s)) return 0;


    if (*s->crnt != c) return 0;
    return *s->crnt++;
}

struct scanner *scalloc(const char *src) {
    struct scanner *s = malloc(sizeof(struct scanner));

    s->crnt = s->st = s->buf = src;
    s->line = 1;
    return s;
};
void init_err_token(struct scanner *s, struct token *t, const char *msg) {
    t->type = GURU_ERROR;
    t->lexeme = msg;
    *(uint16_t*)&t->pos = (uint16_t) (s->st - s->buf);
    *(size_t*)&t->len = strlen(msg);
    *(uint32_t*)&t->line = s->line;
};

void scgett(struct scanner *s, struct token *t) {
    for (char c = *s->crnt;;)
        if (c == ' ' || c == '\r' || c == '\t') c = *++s->crnt;
        else if (c == '\n') s->line++, c = *++s->crnt;
        else if (c == '/' && *(s->crnt + 1) == '/')
            for (char c = *s->crnt; c != '\n' && !at_end(s); c = *++s->crnt);
        else break;

    s->st = s->crnt;
    if (at_end(s)) return init_token(s, t, GURU_EOF);

    char c = *s->crnt++;

    if (isdigit(c)) for (char c = *s->crnt; ; c = *++s->crnt) {
        uint8_t is_float = 1;
        if (c == '.') {
            if (is_float && !isdigit(*(s->crnt + 1)))
                return ++s->crnt, init_err_token(s, t, "unterminated float literal");
            continue;
            // is_float = 1;
        } else if (!isdigit(c)) return is_float ? init_token(s, t, GURU_NUMBER) : init_token(s, t, GURU_INTEGER);

    };

    if (isalpha(c) || c=='_') for (char c = *s->crnt; ; c = *++s->crnt)
        if (!isalnum(c) && c!='_') {init_token(s, t, GURU_IDENTIFIER); ident_scan_dfa(s, t); return;};

    switch (c) {
    case '(': return init_token(s, t, GURU_LEFT_PAREN);
    case ')': return init_token(s, t, GURU_RIGHT_PAREN);
    case '[': return init_token(s, t, GURU_LEFT_SQ);
    case ']': return init_token(s, t, GURU_RIGHT_SQ);
    case '{': return init_token(s, t, GURU_LEFT_BRACE);
    case '}': return init_token(s, t, GURU_RIGHT_BRACE);
    case ';': return init_token(s, t, GURU_SEMICOLON);
    case ',': return init_token(s, t, GURU_COMMA);
    case '#':{
        switch (*s->crnt++) {
            case '=': return init_token(s, t, GURU_STRING_EQ);
            case '#': return init_token(s, t, GURU_STRING_CONCAT);
            default: return init_err_token(s, t, "erroneous token");
        };
     }
    case '.': return init_token(s, t, GURU_DOT);
    case '-': return init_token(s, t, GURU_MINUS);
    case '+': return init_token(s, t, GURU_PLUS);
    case '/': return init_token(s, t, GURU_SLASH);
    case '*': return init_token(s, t, GURU_STAR);
    case '!': return init_token(s, t, match(s, '=') ? GURU_BANG_EQUAL : GURU_BANG);
    case '=': return init_token(s, t, match(s, '=') ? GURU_EQUAL_EQUAL : GURU_EQUAL);
    case '>': return init_token(s, t, match(s, '=') ? GURU_GREATER_EQUAL : GURU_GREATER);
    case '<': return init_token(s, t, match(s, '=') ? GURU_LESS_EQUAL : GURU_LESS);
    case '"': {
        for (char c = *s->crnt; !at_end(s); c = *++s->crnt)
            if (match(s, '"')) return init_token(s, t, GURU_STRING);
            else if (match(s, '\n')) return init_err_token(s, t, "unterminated string literal");
        }
    }

    return init_err_token(s, t, "unknown token!");
};

static void ident_check_kw(struct token *t, const char *rest, uint16_t offset, enum tokent tt) {
    if (t->len - offset != strlen(rest)) t->type = GURU_IDENTIFIER;
    else if (memcmp(t->lexeme + offset, rest, t->len - offset) == 0) t->type = tt;
    else t->type = GURU_IDENTIFIER;
};

static void ident_scan_dfa(struct scanner *s, struct token *t) {
    switch (*t->lexeme) {
    case 'a': {ident_check_kw(t, "nd", 1, GURU_AND) ; break;};
    case 'c':
        if (t->len > 1) {
            switch (t->lexeme[1]) {
            case 'l': ident_check_kw(t, "ass", 2, GURU_CLASS); break;
            case 'o': ident_check_kw(t, "nst", 2, GURU_CONST); break;
            }
        }
        break;
    case 'e': {ident_check_kw(t, "lse", 1, GURU_ELSE); break;};
    case 'i': {ident_check_kw(t, "f", 1, GURU_IF); break;};
    case 'n': {ident_check_kw(t, "othing", 1, GURU_NOTHING); break;};
    case 'o': {ident_check_kw(t, "r", 1, GURU_OR); break;};
    // case 'p': if (t->len > 5) {
    //         switch (t->lexeme[5]) {
    //         case 'o': ident_check_kw(t, "ut", 6, GURU_P_OUT); break;
    //         case 'e': ident_check_kw(t, "rr", 6, GURU_P_ERR); break;
    //     }
    //     break;
    // };
    case 'r': {ident_check_kw(t, "eturn", 1, GURU_RETURN); break;};
    case 's': {ident_check_kw(t, "uper", 1, GURU_SUPER); break;};
    case 't': {ident_check_kw(t, "rue", 1, GURU_TRUE); break;};
    case 'v':
        if (t->len > 1) {
            switch (t->lexeme[1]) {
            case 'a': ident_check_kw(t, "r", 2, GURU_VAR); break;
            case 'o': ident_check_kw(t, "id", 2, GURU_VOID); break;
            }
        }
        break;
    case 'w': {ident_check_kw(t, "hile", 1, GURU_WHILE); break;};
    case 'f':
        if (t->len > 1) {
            switch (t->lexeme[1]) {
            case 'a': ident_check_kw(t, "lse", 2, GURU_FALSE); break;
            case 'o': ident_check_kw(t, "r", 2, GURU_FOR); break;
            case 'u': ident_check_kw(t, "n", 2, GURU_FUN); break;
            }
        }
        break;
    }
};
