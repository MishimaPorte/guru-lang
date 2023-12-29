#include "compiler.h"
#include "bytecode.h"
#include "hashmap.h"
#include "object.h"
#include "scanner.h"
#include "value.h"
#include "compiler_utils.h"
#include "vm.h"
#include <bits/pthreadtypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static void fin_chunk(struct chunk *c) {
    chput(c, OP_EXIT);
}

static void __synchronize(struct compiler *comp) {
    comp->state &= (~PANIC_MODE);
    comp->state |= ROTTEN;
    while (comp->now.type != GURU_EOF) {
        if (comp->then.type == GURU_SEMICOLON) goto synched;
        switch (comp->now.type) {
            case GURU_VAR: case GURU_P_OUT:
            case GURU_P_ERR: case GURU_RETURN:
                goto synched;
            default:;
        };
synched:
        advance(comp);
        break;
    }
};
enum exec_code compile(const char *source, struct chunk *c) {

    struct compiler comp;
    comp.scanner = scalloc(source);
    comp.state = 0x0000;
    comp.line = 1;
    comp.compiled_chunk = c;
    comp.scope_depth = 0;
    comp.local_count = 0;
    comp.stack.head = 0;


    advance(&comp);
    while (comp.now.type != GURU_EOF) {
        __comp_statement(&comp);
        if (comp.state & PANIC_MODE) {
            __synchronize(&comp);
        }
    }
    advance(&comp);
    fin_chunk(c);
    return had_err(&comp) ? RESULT_COMPERR : RESULT_OK;
}

static void __comp_statement(struct compiler *comp) {
    switch (comp->now.type) {
        case GURU_CONST:
            advance(comp);
            panic_if_not_this(comp, GURU_IDENTIFIER, "vars are initialized with identifiers");

            if (comp->scope_depth != 0) {
                uint8_t i = __loc_var_decl(comp);
                panic_if_not_this(comp, GURU_EQUAL, "must init a constant");
                // advance(comp);
                __comp_expression(comp);
                chput(comp->compiled_chunk, OP_PUT_8_WITH_POP);
                chput(comp->compiled_chunk, i);
                __loc_const_init(comp);
            } else {
                struct __blob_header *blob = get_int_str(comp->then.lexeme, comp->then.len);
                panic_if_not_this(comp, GURU_EQUAL, "must init a constant");
                __comp_expression(comp);
                __insert_op(comp, &blob, sizeof(blob), BLOB_STRING, OP_DEFINE_GLOBAL, OP_DEFINE_GLOBAL_16);
            } 
            break;
        case GURU_VAR:
            advance(comp);
            panic_if_not_this(comp, GURU_IDENTIFIER, "vars are initialized with identifiers");

            if (comp->scope_depth != 0) {
                uint8_t i = __loc_var_decl(comp);
                if (comp->now.type == GURU_EQUAL) {
                    advance(comp);
                    __comp_expression(comp);
                    chput(comp->compiled_chunk, OP_PUT_8_WITH_POP);
                    chput(comp->compiled_chunk, i);
                } else {
                    chput(comp->compiled_chunk, OP_PUT_8_VOID);
                    chput(comp->compiled_chunk, i);
                    chput(comp->compiled_chunk, OP_OP);
                }
                __loc_var_init(comp);
            } else {
                struct __blob_header *blob = get_int_str(comp->then.lexeme, comp->then.len);
                if (comp->now.type == GURU_EQUAL) {
                    advance(comp);
                    __comp_expression(comp);
                } else {
                    chput(comp->compiled_chunk, OP_LOAD_VOID);
                }
                __insert_op(comp, &blob, sizeof(blob), BLOB_STRING, OP_DEFINE_GLOBAL, OP_DEFINE_GLOBAL_16);
            } 
            break;
        case GURU_RETURN:
            if (comp->block_stack == 0 || comp->stack.head == 0)
                comp_err_report(comp, &comp->now, "return only inside a block functions");
            advance(comp);
            __comp_expression(comp);
            chput(comp->compiled_chunk, OP_COLLECT_LOCALS);
            uint16_t loc_count = __count_locals(comp);
            chputn(comp->compiled_chunk, &loc_count, sizeof(loc_count));
            chput(comp->compiled_chunk, OP_RETURN);
            break;
        case GURU_FOR:
            // chput(comp->compiled_chunk, OP_FOR_LOOP);
            advance(comp);
            __begin_scope(comp);
            panic_if_not_this(comp, GURU_LEFT_PAREN, "for loop clauses should be parenthesized");
            //INFO: initializer clause
            if (comp->now.type == GURU_VAR) {
                advance(comp);
                panic_if_not_this(comp, GURU_IDENTIFIER, "vars are initialized with identifiers");
                uint8_t i = __loc_var_decl(comp);
                if (comp->now.type == GURU_EQUAL) {
                    advance(comp);
                    __comp_expression(comp);
                    chput(comp->compiled_chunk, OP_PUT_8);
                } else {
                    chput(comp->compiled_chunk, OP_PUT_8_VOID);
                }
                chput(comp->compiled_chunk, i);
                __loc_var_init(comp);
            } else if (comp->now.type != GURU_SEMICOLON) {
                __comp_expression(comp);
                chput(comp->compiled_chunk, OP_OP);
            }
            panic_if_not_this(comp, GURU_SEMICOLON, "unterminated for loop initializer");

            //INFO: condition clause
            uint32_t ex = 0;
            uint32_t cond_loop = comp->compiled_chunk->opcount;
            if (comp->now.type != GURU_SEMICOLON) {
                __comp_expression(comp);
                ex = emit_jump(comp, OP_JUMP_IF_FALSE);
            }
            panic_if_not_this(comp, GURU_SEMICOLON, "unterminated for loop condition");
            //INFO: increment clause
            uint32_t incr_loop = 0;
            if (comp->now.type != GURU_RIGHT_PAREN) {
                uint32_t increment = emit_jump(comp, OP_JUMP);
                incr_loop = comp->compiled_chunk->opcount;
                __comp_expression(comp);
                chput(comp->compiled_chunk, OP_OP);
                jump_to(comp, OP_JUMP_BACK, cond_loop);
                confirm_jump(comp, increment);
            }
            panic_if_not_this(comp, GURU_RIGHT_PAREN, "for loop clauses should be parenthesized");

            //INFO: loop body
            __comp_statement(comp);

            if (incr_loop != 0) jump_to(comp, OP_JUMP_BACK, incr_loop);
            else jump_to(comp, OP_JUMP_BACK, cond_loop);
            
            if (ex != 0) {
                confirm_jump(comp, ex);
            };
            __end_scope(comp);
            // chput(comp->compiled_chunk, OP_FOR_LOOP_END);
            return;
        case GURU_WHILE:
            advance(comp);
            uint32_t loop = comp->compiled_chunk->opcount;
            panic_if_not_this(comp, GURU_LEFT_PAREN, "condition must be in parentheses");
            __comp_expression(comp);
            panic_if_not_this(comp, GURU_RIGHT_PAREN, "condition must be in parentheses");
            uint32_t jump = emit_jump(comp, OP_JUMP_IF_FALSE);
            __comp_statement(comp);
            jump_to(comp, OP_JUMP_BACK, loop);
            confirm_jump(comp, jump);
            return;
        default:
            __comp_expression(comp);
            chput(comp->compiled_chunk, OP_OP);
    }
    panic_if_not_this(comp, GURU_SEMICOLON, "unterminated statement!");
}

static void __comp_number(struct compiler *comp) {
    double num = strtod(comp->then.lexeme, NULL);
    __insert_op(comp, &num, sizeof(num), VAL_NUMBER, OP_CONST, OP_CONST_16);
}

static void __comp_string(struct compiler *comp) {
    struct __blob_header *blob = get_int_str(comp->then.lexeme + 1, comp->then.len - 2);
    __insert_op(comp, &blob, sizeof(blob), BLOB_STRING, OP_CONST, OP_CONST_16);
}

static void __comp_grouping(struct compiler *comp) {

    __comp_expression(comp);
    panic_if_not_this(comp, GURU_RIGHT_PAREN, "unmatched left parentheses");
};

static void __comp_unary(struct compiler *comp) {
    enum tokent t = comp->then.type;
    __comp_with_precedence(comp, __PREC_UNARY);

    switch (t) {
        case GURU_MINUS: return chput(comp->compiled_chunk, OP_FLOAT_NEGATE);
        case GURU_BANG: return chput(comp->compiled_chunk, OP_LOGNOT);
        default: return;
    }
}

static void __comp_expression(struct compiler *comp) {
    __comp_with_precedence(comp, __PREC_ASSIGNMENT);
};

static void __comp_identifier(struct compiler *comp) {
    struct local *var;
    enum local_resolved loc;
    if (memcmp(comp->then.lexeme, "this", 4) == 0) {
        if (comp->now.type == GURU_EQUAL) {
            comp_err_report(comp, &comp->now, "cant assign to this");
            return;
        };
        chput(comp->compiled_chunk, OP_LOAD_THIS);
        return;
    };
    if ((loc = resolve_local(comp, &var)) == RESOLVED_NON_LOCAL) {
        struct __blob_header *blob = get_int_str(comp->then.lexeme, comp->then.len);
        if (comp->now.type == GURU_EQUAL) {
            if (!(comp->state & ASSIGN_PERMIT)) {
                comp_err_report(comp, &comp->then, "assignment not allowed here");
                return;
            }
            advance(comp);
            __comp_expression(comp);
            __insert_op(comp, &blob, sizeof(blob), BLOB_STRING, OP_ASSIGN_GLOBAL, OP_ASSIGN_GLOBAL_16);
        } else {
            __insert_op(comp, &blob, sizeof(blob), BLOB_STRING, OP_LOAD_GLOBAL, OP_LOAD_GLOBAL_16);
        }
        return;
    }
    if (comp->now.type == GURU_EQUAL) {
        if (var->is_const == 1) {
            comp_err_report(comp, &comp->now, "cant assign a const");
            return;
        }
        if (!(comp->state & ASSIGN_PERMIT)) {
            comp_err_report(comp, &comp->then, "assignment not allowed here");
            return;
        }
        advance(comp);
        __comp_expression(comp);
        if (loc == RESOLVED_IS_CLOSURE || loc == RESOLVED_NEED_CLOSURE) {
            chput(comp->compiled_chunk, OP_PUT_CLOSURE);
            chput(comp->compiled_chunk, var->is_link-1);
        } else {
            if (var->is_link)
                chput(comp->compiled_chunk, OP_PUT_LINK);
            else
                chput(comp->compiled_chunk, OP_PUT_8);
            chput(comp->compiled_chunk, var->ofsetted + PRIVILEGED_STACK_SLOTS);
        }
    } else if (loc == RESOLVED_IS_CLOSURE || loc == RESOLVED_NEED_CLOSURE) {
            chput(comp->compiled_chunk, OP_LOAD_CLOSURE);
            chput(comp->compiled_chunk, var->is_link-1);
    } else {
        if (var->is_link)
            chput(comp->compiled_chunk, OP_LOAD_LINK);
        else
            chput(comp->compiled_chunk, OP_LOAD_8);
        chput(comp->compiled_chunk, var->ofsetted + PRIVILEGED_STACK_SLOTS);
    }

};

static void __comp_if_expression(struct compiler *comp) {
    panic_if_not_this(comp, GURU_LEFT_PAREN, "expected left parentheses after if");
    __comp_expression(comp);
    panic_if_not_this(comp, GURU_RIGHT_PAREN, "expected right parentheses after condition");
    uint32_t offset = emit_jump(comp, OP_JUMP_IF_FALSE);
    __comp_expression(comp);
    uint32_t offset2 = emit_jump(comp, OP_JUMP);
    confirm_jump(comp, offset);
    if (comp->now.type == GURU_ELSE) {
        advance(comp);
        __comp_expression(comp);
    } else {
        chput(comp->compiled_chunk, OP_LOAD_NOTHING);
    };
    confirm_jump(comp, offset2);
};

static void __comp_liter(struct compiler *comp) {
    switch (comp->then.type) {
    case GURU_TRUE: return chput(comp->compiled_chunk, OP_LOAD_TRUTH);
    case GURU_FALSE: return chput(comp->compiled_chunk, OP_LOAD_LIES);
    case GURU_NOTHING: return chput(comp->compiled_chunk, OP_LOAD_NOTHING);
    case GURU_VOID: return chput(comp->compiled_chunk, OP_LOAD_VOID);
    default: comp_err_report(comp, &comp->then, "invalid literal"); // unreachable
    }
};

static void __comp_binary(struct compiler *comp) {
    enum tokent tt = comp->then.type;

    struct pratt_rule *pr = __get_pratt_rule(tt);
    __comp_with_precedence(comp, pr->prec + 1);

    switch (tt) {
    case GURU_PLUS: chput(comp->compiled_chunk, OP_FLOAT_SUM_2); break;
    case GURU_MINUS: chput(comp->compiled_chunk, OP_FLOAT_SUB); break;
    case GURU_STAR: chput(comp->compiled_chunk, OP_FLOAT_MULT_2); break;
    case GURU_SLASH: chput(comp->compiled_chunk, OP_FLOAT_DIVIDE); break;

    case GURU_STRING_CONCAT: chput(comp->compiled_chunk, OP_STR_CONC); break;
    case GURU_STRING_EQ: chput(comp->compiled_chunk, OP_STRING_EQ); break;
                

    case GURU_EQUAL_EQUAL:{
        chput(comp->compiled_chunk, OP_EQ);
        break;
    }
    case GURU_BANG_EQUAL: chput(comp->compiled_chunk, OP_NEQ); break;
    case GURU_GREATER: chput(comp->compiled_chunk, OP_GT); break;
    case GURU_GREATER_EQUAL: chput(comp->compiled_chunk, OP_GTE); break;
    case GURU_LESS: chput(comp->compiled_chunk, OP_LT); break;
    case GURU_LESS_EQUAL: chput(comp->compiled_chunk, OP_LTE); break;
    default: return;
    }
};

static void __comp_or(struct compiler *comp) {
    uint32_t j = emit_jump(comp, OP_JUMP_IF_TRUE_ONSTACK);
    chput(comp->compiled_chunk, OP_OP);
    __comp_with_precedence(comp, __PREC_OR);

    confirm_jump(comp, j);
};

static void __comp_and(struct compiler *comp) {
    uint32_t j = emit_jump(comp, OP_JUMP_IF_FALSE_ONSTACK);
    chput(comp->compiled_chunk, OP_OP);
    __comp_with_precedence(comp, __PREC_AND);

    confirm_jump(comp, j);
};

static void __comp_call(struct compiler *comp) {
    uint8_t i = 0;
    while (1) {
        if (comp->now.type == GURU_RIGHT_PAREN) {
            advance(comp);
            break;
        };
        i++;
        __comp_expression(comp);
        if (comp->now.type != GURU_COMMA) {
            panic_if_not_this(comp, GURU_RIGHT_PAREN, "need a closing parentheses");
            break;
        } else advance(comp);
    };
    chput(comp->compiled_chunk, OP_CALL);
    chput(comp->compiled_chunk, comp->local_count + PRIVILEGED_STACK_SLOTS);
    chput(comp->compiled_chunk, i);
};

static void __comp_fun_expression(struct compiler *comp) {
    __begin_scope(comp);

    uint8_t (*cl_old)[255] = comp->closures.closures;
    uint8_t old_count_cl = comp->closures.closure_count;

    comp->closures.closure_count = 0;
    comp->closures.closures = malloc(255);

    uint32_t jump = emit_jump(comp, OP_JUMP);

    panic_if_not_this(comp, GURU_LEFT_PAREN, "function literal has the form of 'fun (x, y) expression'");
    uint8_t arity = 0;
    __start_func_declaration(comp);

    while (1) {
        if (comp->now.type == GURU_RIGHT_PAREN) {
            advance(comp);
            break;
        };
        panic_if_not_this(comp, GURU_IDENTIFIER, "function params are identifiers");
        uint8_t i = __loc_var_decl(comp);
        __loc_var_init(comp);
        arity++;
        if (comp->now.type != GURU_COMMA) {
            panic_if_not_this(comp, GURU_RIGHT_PAREN, "need a closing parentheses");
            break;
        } else advance(comp);
    };

    uint32_t func_begin_v = comp->compiled_chunk->opcount;
    __comp_expression(comp);
    chput(comp->compiled_chunk, OP_RETURN);
    __end_func_declaration(comp);
    confirm_jump(comp, jump);
    struct __blob_header *h = alloc_guru_callable(func_begin_v, arity, comp->closures.closure_count);
    __insert_op(comp, &h, sizeof(h), BLOB_CALLABLE, OP_DECLARE_FUNCTION, OP_DECLARE_FUNCTION_16);
    chput(comp->compiled_chunk, comp->closures.closure_count);
    for (uint8_t i = 0; i < comp->closures.closure_count; i++) {
        chput(comp->compiled_chunk, *comp->closures.closures[i]);
    }

    free(comp->closures.closures);
    comp->closures.closures = cl_old;
    comp->closures.closure_count = old_count_cl;

    __end_scope(comp);
};

static void __comp_block_expression(struct compiler *comp) {
    comp->block_stack++;
    __begin_scope(comp);
    if (comp->now.type == GURU_RIGHT_BRACE) {
        comp->block_stack--;
        __end_scope(comp);
        chput(comp->compiled_chunk, OP_LOAD_NOTHING);
        return;
    }
    while (comp->now.type != GURU_RIGHT_BRACE) {
        __comp_statement(comp);
        if (comp->state & PANIC_MODE) {
            __synchronize(comp);
        }
    }
    panic_if_not_this(comp, GURU_RIGHT_BRACE, "unmatched left brace");
    chunputif(comp->compiled_chunk, OP_OP, OP_LOAD_NOTHING);
    uint16_t i = __end_scope(comp);
    chput(comp->compiled_chunk, OP_COLLECT_LOCALS);
    chputn(comp->compiled_chunk, &i, sizeof(i));
    comp->block_stack--;
};

static void __comp_with_precedence(struct compiler *comp, enum comp_precedence prec) {
    void (*pref_fn)(struct compiler*) = __get_pratt_rule(comp->now.type)->prefix_fn;
    if (pref_fn == NULL) {
        comp_err_report(comp, &comp->now, "an expression is expected here");
        return;
    } else advance(comp);
    
    
    uint16_t old_state = comp->state;
    if (prec <= __PREC_ASSIGNMENT)
        comp->state |= ASSIGN_PERMIT;
    else comp->state &= (~ASSIGN_PERMIT);
    pref_fn(comp);

    while (prec <= __get_pratt_rule(comp->now.type)->prec) {
        advance(comp);
        __get_pratt_rule(comp->then.type)->infix_fn(comp);
    }
    if (comp->state & PANIC_MODE) old_state |= PANIC_MODE;
    if (comp->state & ROTTEN) old_state |= ROTTEN;
    comp->state = old_state;
};

static void __comp_class(struct compiler *comp)
{
    panic_if_not_this(comp, GURU_LEFT_BRACE, "class literal has the form of 'class {things}'");

    uint8_t i = 0;

    struct u16_array_list *u16al = init_u16_array_list(10);

    struct __blob_header *blob_cl = alloc_class();
    struct guru_class *cl = (struct guru_class*)blob_cl->__cont;

    for (;;) {
        i++;
        switch (comp->now.type) {
            case GURU_FUN:
                advance(comp);
                panic_if_not_this(comp, GURU_IDENTIFIER, "a method should have a name");

                struct __blob_header *name = __alloc_blob(comp->then.len + 1);
                memcpy(name->__cont, comp->then.lexeme, comp->then.len);
                name->__cont[comp->then.len] = '\0';

                uint16_t const_i = add_const(comp->compiled_chunk, &name, sizeof(name), BLOB_STRING);
                append(u16al, const_i);

                __begin_scope(comp);
                uint8_t (*cl_old)[255] = comp->closures.closures;
                uint8_t old_count_cl = comp->closures.closure_count;

                comp->closures.closure_count = 0;
                comp->closures.closures = malloc(255);


                uint32_t jump = emit_jump(comp, OP_JUMP);

                panic_if_not_this(comp, GURU_LEFT_PAREN, "function literal has the form of 'fun (x, y) expression'");
                uint8_t arity = 0;
                __start_func_declaration(comp);

                while (1) {
                    if (comp->now.type == GURU_RIGHT_PAREN) {
                        advance(comp);
                        break;
                    };
                    panic_if_not_this(comp, GURU_IDENTIFIER, "function params are identifiers");
                    uint8_t i = __loc_var_decl(comp);
                    __loc_var_init(comp);
                    arity++;
                    if (comp->now.type != GURU_COMMA) {
                        panic_if_not_this(comp, GURU_RIGHT_PAREN, "need a closing parentheses");
                        break;
                    } else advance(comp);
                };

                uint32_t func_begin_v = comp->compiled_chunk->opcount;
                __comp_expression(comp);
                if (strcmp("__constructor", name->__cont) == 0)
                {
                    chput(comp->compiled_chunk, OP_OP);
                    chput(comp->compiled_chunk, OP_LOAD_THIS);
                }
                chput(comp->compiled_chunk, OP_RETURN);
                __end_func_declaration(comp);
                confirm_jump(comp, jump);
                
                struct __blob_header *h = alloc_guru_callable(func_begin_v, arity, comp->closures.closure_count);
                __insert_op(comp, &h, sizeof(h), BLOB_CALLABLE, OP_DECLARE_FUNCTION, OP_DECLARE_FUNCTION_16);
                ((struct guru_callable*)h->__cont)->__is_method = 1;
                chput(comp->compiled_chunk, comp->closures.closure_count);
                for (uint8_t i = 0; i < comp->closures.closure_count; i++) {
                    chput(comp->compiled_chunk, *comp->closures.closures[i]);
                }

                free(comp->closures.closures);
                comp->closures.closures = cl_old;
                comp->closures.closure_count = old_count_cl;

                __end_scope(comp);

                panic_if_not_this(comp, GURU_SEMICOLON, "a forgotten semicolon");
                break;
            case GURU_VAR:
                advance(comp);
                panic_if_not_this(comp, GURU_IDENTIFIER, "a realm must have a king; a field must have a name that is an identifier");
                struct __blob_header *blob = get_int_str(comp->then.lexeme, comp->then.len);
                if (comp->now.type == GURU_EQUAL) {
                    advance(comp);
                    __comp_expression(comp);
                } else {
                    chput(comp->compiled_chunk, OP_LOAD_VOID);
                }
                // __insert_op(comp, &blob, sizeof(blob), BLOB_STRING, OP_DEFINE_CLASSVAR, OP_DEFINE_CLASSVAR_16);
                break;
            default:
                comp_err_report(comp, &comp->now, "this is not allowed in class literal");
                break;
        }
        if (comp->now.type == GURU_RIGHT_BRACE) {
            advance(comp);
            break;
        };
    };

    __insert_op(comp, &blob_cl, sizeof(blob_cl), BLOB_CLASS, OP_DECLARE_CLASS, OP_DECLARE_CLASS_16);
    chput(comp->compiled_chunk, i);
    for (uint16_t it = i; it > 0; it--) {
        uint16_t var = get(u16al, it-1);
        chputn(comp->compiled_chunk, &var, 2);
    }
};

static void __comp_index_sq(struct compiler *comp)
{
    __comp_expression(comp); // index expression
    panic_if_not_this(comp, GURU_RIGHT_SQ, "close your index expressions with square brackets");

    if (comp->now.type == GURU_EQUAL) {
        advance(comp);
        __comp_expression(comp);
        chput(comp->compiled_chunk, OP_INDEX_SET);
    } else {
        chput(comp->compiled_chunk, OP_INDEX_GET);
    }
};

static void __comp_getset(struct compiler *comp)
{
    panic_if_not_this(comp, GURU_IDENTIFIER, "expected an identifier after the dot");
    struct __blob_header *blob = get_int_str(comp->then.lexeme, comp->then.len);
    if (comp->now.type == GURU_EQUAL)
    {
        advance(comp);
        __comp_expression(comp);
        // __comp_with_precedence(comp, __PREC_CALL);
        if (comp->state & ASSIGN_PERMIT)
            __insert_op(comp, &blob, sizeof(blob), BLOB_STRING, OP_SET, OP_SET_16);
        else {
            comp_err_report(comp, &comp->now, "cannot assign here");
            return;
        };
    } else {
        __insert_op(comp, &blob, sizeof(blob), BLOB_STRING, OP_GET, OP_GET_16);
    }
};

static struct pratt_rule pratt_table[] = {
    [GURU_LEFT_PAREN] = {&__comp_grouping, &__comp_call, __PREC_CALL},
    [GURU_RIGHT_PAREN] = {NULL, NULL, __PREC_NONE},
    [GURU_LEFT_BRACE] = {&__comp_block_expression, NULL, __PREC_NONE},
    [GURU_RIGHT_BRACE] = {NULL, NULL, __PREC_NONE},
    [GURU_LEFT_SQ] = {NULL, &__comp_index_sq, __PREC_CALL},
    [GURU_RIGHT_SQ] = {NULL, NULL, __PREC_NONE},
    [GURU_COMMA] = {NULL, NULL, __PREC_NONE},
    [GURU_DOT] = {NULL, &__comp_getset, __PREC_CALL},
    [GURU_MINUS] = {&__comp_unary, &__comp_binary, __PREC_TERM},
    [GURU_PLUS] = {NULL, &__comp_binary, __PREC_TERM},
    [GURU_SEMICOLON] = {NULL, NULL, __PREC_NONE},
    [GURU_SLASH] = {NULL, &__comp_binary, __PREC_FACTOR},
    [GURU_STAR] = {NULL, &__comp_binary, __PREC_FACTOR},
    [GURU_BANG] = {&__comp_unary, NULL, __PREC_NONE},

    [GURU_BANG_EQUAL] = {NULL, &__comp_binary, __PREC_EQUALITY},
    [GURU_EQUAL_EQUAL] = {NULL, &__comp_binary, __PREC_EQUALITY},
    [GURU_STRING_CONCAT] = {NULL, &__comp_binary, __PREC_TERM},
    [GURU_STRING_EQ] = {NULL, &__comp_binary, __PREC_EQUALITY},

    [GURU_GREATER] = {NULL, &__comp_binary,__PREC_COMPARISON},
    [GURU_GREATER_EQUAL] = {NULL, &__comp_binary, __PREC_COMPARISON},
    [GURU_LESS] = {NULL, &__comp_binary, __PREC_COMPARISON},
    [GURU_LESS_EQUAL] = {NULL, &__comp_binary, __PREC_COMPARISON},

    [GURU_EQUAL] = {NULL, NULL, __PREC_NONE},

    [GURU_IDENTIFIER] = {&__comp_identifier, NULL, __PREC_NONE},
    [GURU_NUMBER] = {&__comp_number, NULL, __PREC_NONE},
    [GURU_STRING] = {&__comp_string, NULL, __PREC_NONE},

    [GURU_AND] = {NULL, &__comp_and, __PREC_AND},
    [GURU_OR] = {NULL, &__comp_or, __PREC_OR},
    // [GURU_INTEGER] = {&__comp_integer, NULL, __PREC_NONE},
    //
    [GURU_TRUE] = {&__comp_liter, NULL, __PREC_NONE},
    [GURU_FALSE] = {&__comp_liter, NULL, __PREC_NONE},
    [GURU_VOID] = {&__comp_liter, NULL, __PREC_NONE},
    [GURU_NOTHING] = {&__comp_liter, NULL, __PREC_NONE},

    [GURU_FOR] = {NULL, NULL, __PREC_NONE},
    [GURU_FUN] = {&__comp_fun_expression, NULL, __PREC_NONE},

    [GURU_IF] = {&__comp_if_expression, NULL, __PREC_NONE},
    [GURU_ELSE] = {NULL, NULL, __PREC_NONE},

    [GURU_P_ERR] = {NULL, NULL, __PREC_NONE},
    [GURU_P_OUT] = {NULL, NULL, __PREC_NONE},

    [GURU_RETURN] = {NULL, NULL, __PREC_NONE},

    [GURU_CLASS] = {&__comp_class, NULL, __PREC_NONE},
    [GURU_SUPER] = {NULL, NULL, __PREC_NONE},
    [GURU_THIS] = {NULL, NULL, __PREC_NONE},
};

static struct pratt_rule *__get_pratt_rule(enum tokent tt) {
    return &pratt_table[tt];
};
