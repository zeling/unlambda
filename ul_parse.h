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

typedef enum {
    UL_PARSE_OK = 0,
    UL_PARSE_EOF,
    UL_PARSE_UNRECOGNIZED,
} ul_parse_err_t;

typedef struct ul_ast {
    union {
        struct ul_ast *rator;
        ul_sym_t *atom;
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

ul_ast_t *ul_ast_mk_atom(ul_sym_t *sym);

ul_ast_t *ul_parse_atom(ul_parse_state_t *s);
ul_ast_t *ul_parse_app(ul_parse_state_t *s);
