/* Simple list implementation.
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

#define CONTAINER_OF(p, type, field)                                           \
    (type *)(((char *)(p)) - offsetof(type, field))

typedef struct list_link {
    struct list_link *prev;
    struct list_link *next;
} list_t, list_link_t;

static inline void list_init(list_t *list)
{
    list->prev = list->next = list;
}

static inline int list_empty(list_t *list)
{
    return list->prev == list->next;
}

static inline int link_unlinked(list_link_t *link)
{
    return link->prev == link->next;
}

static inline void list_insert_after(list_link_t *link, list_link_t *obj)
{
    obj->next = link->next;
    obj->prev = link;
    link->next = obj;
    obj->next->prev = obj;
}

static inline void list_insert_before(list_link_t *link, list_link_t *obj)
{
    list_insert_after(link->prev, obj);
}

static inline void list_insert_back(list_t *list, list_link_t *obj)
{
    list_insert_after(list->prev, obj);
}

static inline void list_insert_front(list_t *list, list_link_t *obj)
{
    list_insert_before(list->next, obj);
}

static inline void list_unlink(list_link_t *link)
{
    link->next->prev = link->prev;
    link->prev->next = link->next;
}

#define LIST_FOR_EACH(l, e, type, field)                                       \
    for (type *e = CONTAINER_OF((l)->next, type, field); &(e->field) != (l);   \
         e = CONTAINER_OF(e->field.next, type, field))
