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
#include <stdarg.h>
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

ul_ast_t *ul_ast_mk_atom(ul_sym_t *sym)
{
    ul_ast_t *ast = (ul_ast_t *)malloc(sizeof(ul_ast_t));
    if (!ast)
        return NULL;
    ast->nrands = 0;
    ast->u.atom = sym;
    return ast;
}

ul_ast_t *ul_ast_mk_app(ul_ast_t *rator, size_t nrands, ...)
{
    ul_ast_t *ast =
        (ul_ast_t *)malloc(sizeof(ul_ast_t) + nrands * sizeof(ul_ast_t *));
    if (!ast)
        return NULL;
    ast->u.rator = rator;
    ast->nrands = nrands;
    va_list ap;
    va_start(ap, nrands);
    for (int i = 0; i < nrands; i++) {
        ast->rands[i] = va_arg(ap, ul_ast_t *);
    }
    va_end(ap);
    return ast;
}
