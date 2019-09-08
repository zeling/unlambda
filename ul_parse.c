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
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "ul_parse.h"
int ul_ast_is_atom(ul_ast_t *ast)
{
    return ast->nrands == 0;
}

int ul_ast_is_app(ul_ast_t *ast)
{
    return !ul_ast_is_atom(ast);
}

ul_ast_t *ul_ast_mk_atom(ul_atom_t atom)
{
    ul_ast_t *ast = (ul_ast_t *)malloc(sizeof(ul_ast_t));
    if (!ast)
        return NULL;
    ast->nrands = 0;
    ast->u.atom = atom;
    return ast;
}

void skip_whitespace(ul_parse_state_t *state)
{
    char ch;
    while ((ch = *state->text) && isspace(ch)) {
        state->text++;
    }
}

ul_ast_t *ul_parse_atom(ul_parse_state_t *state)
{
    skip_whitespace(state);
    char *p = state->text;
    ul_ast_t *ast = NULL;
    if (!*p) {
        state->error = UL_PARSE_EOF;
        return NULL;
    }
    switch (*p++) {
    case 's':
        ast = ul_ast_mk_atom(UL_S);
        break;
    case 'k':
        ast = ul_ast_mk_atom(UL_K);
        break;
    case 'i':
        ast = ul_ast_mk_atom(UL_I);
        break;
    case 'd':
        ast = ul_ast_mk_atom(UL_D);
        break;
    case 'c':
        ast = ul_ast_mk_atom(UL_C);
        break;
    case 'v':
        ast = ul_ast_mk_atom(UL_V);
        break;
    case '.':
        if (!*p) {
            state->error = UL_PARSE_EOF;
            return NULL;
        }
        ast = ul_ast_mk_atom(*p++);
        break;
    case 'r':
        ast = ul_ast_mk_atom('\n');
        break;
    default:
        state->error = UL_PARSE_UNRECOGNIZED;
        return NULL;
    }
    state->text = p;
    return ast;
}

ul_ast_t *ul_parse_app(ul_parse_state_t *state)
{
    skip_whitespace(state);
    size_t nrands = 1;
    while (*++state->text == '`') {
        skip_whitespace(state);
        nrands++;
    }

    ul_ast_t *rator = ul_parse_prog(state);
    if (!rator)
        return NULL;

    ul_ast_t *ast =
        (ul_ast_t *)malloc(sizeof(ul_ast_t) + nrands * sizeof(ul_ast_t *));
    if (!ast) {
        state->error = UL_PARSE_OOM;
        return NULL;
    }
    ast->nrands = nrands;
    ast->u.rator = rator;
    for (int i = 0; i < nrands; i++) {
        ast->rands[i] = ul_parse_prog(state);
        if (!ast->rands[i]) {
            for (int j = 0; j < i; j++) {
                ul_ast_free(ast->rands[j]);
            }
            free(ast);
            return NULL;
        }
    }
    return ast;
}

void ul_ast_free(ul_ast_t *ast)
{
    if (ul_ast_is_app(ast)) {
        ul_ast_free(ast->u.rator);
        for (int i = 0; i < ast->nrands; i++) {
            ul_ast_free(ast->rands[i]);
        }
    }
    free(ast);
}

ul_ast_t *ul_parse_prog(ul_parse_state_t *state)
{
    skip_whitespace(state);
    if (!*state->text) {
        state->error = UL_PARSE_EOF;
        return NULL;
    }
    if (*state->text == '`') {
        return ul_parse_app(state);
    } else {
        return ul_parse_atom(state);
    }
}

void ul_ast_dump_atom(ul_atom_t atom, FILE *out)
{
    switch (atom) {
    case UL_S:
        fputc('s', out);
        break;
    case UL_K:
        fputc('k', out);
        break;
    case UL_I:
        fputc('i', out);
        break;
    case UL_C:
        fputc('c', out);
        break;
    case UL_D:
        fputc('d', out);
        break;
    case UL_V:
        fputc('v', out);
        break;
    case '\n':
        fputc('r', out);
        break;
    default:
        fprintf(out, ".%c", atom);
        break;
    }
}

void ul_ast_dump(ul_ast_t *ast, FILE *out)
{
    if (ul_ast_is_atom(ast)) {
        ul_ast_dump_atom(ast->u.atom, out);
    } else {
        for (int i = 0; i < ast->nrands; i++) {
            fputc('`', out);
        }
        ul_ast_dump(ast->u.rator, out);
        for (int i = 0; i < ast->nrands; i++) {
            ul_ast_dump(ast->rands[i], out);
        }
    }
}
