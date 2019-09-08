/* Symbol table interface.
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
#include <stddef.h>

#include "list.h"

#define UL_SYMTAB_BUCKET_SIZE 128
#define UL_SYMTAB_BUCKET_MASK (UL_SYMTAB_BUCKET_SIZE - 1u)

typedef enum {
    UL_SYM_S,
    UL_SYM_K,
    UL_SYM_I,
    UL_SYM_C,   /* call/cc */
    UL_SYM_D,   /* delay (promise) */
    UL_SYM_Dot, /* output */
} ul_sym_kind;

typedef struct ul_sym {
    list_link_t symtab_link;
    ul_sym_kind kind;
    char data[1];
} ul_sym_t;

typedef struct ul_symtab {
    size_t nelems;
    list_t buckets[UL_SYMTAB_BUCKET_SIZE];
} ul_symtab_t;

ul_sym_t *ul_sym_new(ul_sym_kind kind, const char *str, size_t nchar);
void ul_sym_free(ul_sym_t *sym);

void ul_symtab_init(ul_symtab_t *tbl);
void ul_symtab_destroy(ul_symtab_t *tbl);
ul_sym_t *ul_symtab_get(ul_symtab_t *tbl, ul_sym_kind kind, const char *str,
                        size_t nchar);
