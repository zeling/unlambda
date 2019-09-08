/* The parser for unlambda.
 *
 * MIT License
 *
 * Copyright (c) 2019 Zeling Feng
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#pragma once
#include "ul_symtab.h"
#include <stddef.h>

extern ul_symtab_t symtab;

enum {
    UL_S = -1,
    UL_K = -2,
    UL_I = -3,
    UL_C = -4, /* call/cc */
    UL_D = -5, /* delay (promise) */
    UL_V = -6, /* void */
};

typedef int ul_atom_t;

typedef enum {
    UL_PARSE_OK = 0,
    UL_PARSE_EOF,
    UL_PARSE_UNRECOGNIZED,
    UL_PARSE_OOM,
} ul_parse_err_t;

typedef struct ul_ast {
    union {
        struct ul_ast *rator;
        ul_atom_t atom;
    } u;
    size_t nrands;
    struct ul_ast *rands[];
} ul_ast_t;

typedef struct ul_parse_state {
    char *text;
    int error;
} ul_parse_state_t;

int ul_ast_is_atom(ul_ast_t *ast);
int ul_ast_is_app(ul_ast_t *ast);

void ul_ast_free(ul_ast_t *ast);

ul_ast_t *ul_parse_prog(ul_parse_state_t *s);
void ul_ast_dump(ul_ast_t *ast, FILE *out);
