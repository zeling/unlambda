/* Symbol table implementation.
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
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "ul_symtab.h"

static size_t djb2(const char *str, size_t nsize);

ul_sym_t *ul_sym_new(ul_sym_kind kind, const char *str, size_t nchar)
{
    ul_sym_t *sym = malloc(sizeof(ul_sym_t) + nchar);
    if (!sym)
        return NULL;
    sym->kind = kind;
    list_init(&sym->symtab_link);
    strncpy(sym->data, str, nchar + 1);
    return sym;
}

void ul_sym_free(ul_sym_t *sym)
{
    list_unlink(&sym->symtab_link);
    free(sym);
}

void ul_symtab_init(ul_symtab_t *tbl)
{
    int i;
    tbl->nelems = 0;

    for (i = 0; i < UL_SYMTAB_BUCKET_SIZE; i++) {
        list_init(&tbl->buckets[i]);
    }
}

void ul_symtab_destroy(ul_symtab_t *tbl)
{
    int i;
    for (i = 0; i < UL_SYMTAB_BUCKET_SIZE; i++) {
        LIST_FOR_EACH(&tbl->buckets[i], sym, ul_sym_t, symtab_link)
        {
            /* lets make ASAN happy */
#ifdef ASAN
            ul_sym_t *prev =
                CONTAINER_OF(sym->symtab_link.prev, ul_sym_t, symtab_link);
#endif
            ul_sym_free(sym);
#ifdef ASAN
            sym = prev;
#endif
            --tbl->nelems;
        }
    }
    assert(tbl->nelems == 0);
}

static ul_sym_t *ul_symtab_insert(ul_symtab_t *tbl, ul_sym_kind kind,
                                  const char *str, size_t nchar)
{
    ul_sym_t *sym = ul_sym_new(kind, str, nchar);
    if (!sym)
        return NULL;
    /* str is not necessarily a C string, but sym->data is guaranteed to be */
    list_t *bucket = &tbl->buckets[djb2(str, nchar) & UL_SYMTAB_BUCKET_MASK];
    list_insert_back(bucket, &sym->symtab_link);
    ++tbl->nelems;
    return sym;
}

ul_sym_t *ul_symtab_get(ul_symtab_t *tbl, ul_sym_kind kind, const char *str,
                        size_t nchar)
{
    list_t *bucket = &tbl->buckets[djb2(str, nchar) & UL_SYMTAB_BUCKET_MASK];
    LIST_FOR_EACH(bucket, sym, ul_sym_t, symtab_link)
    {
        if (strncmp(sym->data, str, nchar) == 0) {
            return sym;
        }
    }
    return ul_symtab_insert(tbl, kind, str, nchar);
}

size_t djb2(const char *str, size_t nsize)
{
    size_t hash = 5381;
    int i;

    for (i = 0; i < nsize; i++)
        hash = ((hash << 5u) + hash) + str[i]; /* hash * 33 + c */

    return hash;
}
