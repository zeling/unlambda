/* The runtime for unlambda.
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
#include <sys/mman.h>
#include <unistd.h>
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>

/* #pages */
#define HEAP_SIZE 16
#define STACK_SIZE 16

/* Tagged pointer */
typedef uintptr_t ul_value_t;
static uint8_t *ul_stack, *ul_from, *ul_to, *ul_free;
static ul_value_t *sp;
static uintptr_t pc;

struct ul_closure {
    void (*entry)(struct ul_closure *);
    size_t n_arity;
    size_t n_args;
    ul_value_t args[];
};

static void ul_initialize() {
    size_t pgsize = sysconf(_SC_PAGESIZE);
    ul_from = mmap(0, HEAP_SIZE * pgsize, PROT_READ | PROT_WRITE, MAP_PRIVATE, -1, 0);
    assert(ul_from != MAP_FAILED);
    ul_to = mmap(0, HEAP_SIZE * pgsize, PROT_READ | PROT_WRITE, MAP_PRIVATE, -1, 0);
    assert(ul_to != MAP_FAILED);
    ul_stack = mmap(0, STACK_SIZE * pgsize, PROT_READ | PROT_WRITE, MAP_PRIVATE, -1, 0);
    assert(ul_stack != MAP_FAILED);
    ul_free = ul_from;
}

static struct ul_closure *allocate(size_t nbytes) {
    size_t pgsize = sysconf(_SC_PAGESIZE);
    if (ul_free + nbytes >= ul_from + HEAP_SIZE * pgsize) {
        /* TODO: Cheney's algorithm */
    }
    struct ul_closure *result = (struct ul_closure *) ul_free;
    ul_free += nbytes;
    return result;
}

static struct ul_closure *ul_new_closure(void (*entry) (struct ul_closure *), size_t n_arity, size_t n_args, ...) {
    struct ul_closure *closure = allocate(sizeof(struct ul_closure) + n_args * sizeof(ul_value_t));
    closure->entry = entry;
    closure->n_arity = n_arity;
    closure->n_args = n_args;
    va_list args;
    va_start(args, n_args);
    for (int i = 0; i < n_args; i++) {
        closure->args[i] = va_arg(args, ul_value_t);
    }
    return closure;
}

#define UL_VAL_MASK 0x1
#define UL_VAL_CLOS 0x1
#define UL_VAL_COMB 0x0

#define POP_STACK() (*sp++)
#define PUSH_STACK(ptr) (*--sp = (ptr))

static void ul_closure_enter(ul_value_t val) {
    if ((val & UL_VAL_MASK) == UL_VAL_CLOS) {
        struct ul_closure *closure = (struct ul_closure *)(val & ~UL_VAL_CLOS);
        closure->entry(closure);
    } else {
        ((void (*) ()) val)();
    }
}

/* Kxy = x */
static void ul_eval_k() {
    ul_value_t x = POP_STACK();
    ul_value_t y = POP_STACK();
    PUSH_STACK(x);
}

/* Ix = x */
static void ul_eval_i() {
    /* no-op */
}

/* Sxyz = xz(yz) */
static void ul_eval_s() {
    ul_value_t x = POP_STACK();
    ul_value_t y = POP_STACK();
    ul_value_t z = POP_STACK();
    PUSH_STACK(z);
    ul_closure_enter(y);
    PUSH_STACK(z);
    ul_closure_enter(x);
}
