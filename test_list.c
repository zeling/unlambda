/* Tests for list implementation.
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
#include "list.h"
#include <stdio.h>
#include <stdlib.h>

struct dummy {
    int num;
    list_link_t link;
};

int main()
{
    list_t l;
    list_init(&l);
    struct dummy *unlink1, *unlink2;
    for (int i = 0; i < 10; i++) {
        struct dummy *obj1 = malloc(sizeof(struct dummy));
        struct dummy *obj2 = malloc(sizeof(struct dummy));
        obj1->num = obj2->num = i;
        if (i == 5) {
            unlink1 = obj1;
            unlink2 = obj2;
        }
        list_insert_back(&l, &obj1->link);
        list_insert_front(&l, &obj2->link);
    }
    list_unlink(&unlink1->link);
    list_unlink(&unlink2->link);
    LIST_FOR_EACH(&l, e, struct dummy, link)
    {
        printf("%d\n", ((struct dummy *)e)->num);
    }
    puts("ok.");
}
