/* Tests for symbol table.
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
#include <stdio.h>

#include "string.h"
#include "ul_symtab.h"

int main()
{
    static char *test_data[] = {
        "Test", "Hello", "Yay", "Test", "Yay",
    };
    ul_symtab_t symtab;
    ul_symtab_init(&symtab);
    for (int i = 0; i < sizeof(test_data) / sizeof(test_data[0]); i++) {
        ul_sym_t *sym =
            ul_symtab_get(&symtab, UL_S, test_data[i], strlen(test_data[i]));
        assert(strcmp(test_data[i], sym->data) == 0);
    }
    assert(symtab.nelems == 3);
    ul_symtab_destroy(&symtab);
    puts("ok.");
}
