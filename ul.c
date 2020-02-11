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
#include <string.h>
#include "ul_parse.h"
#include "dynbuf.h"

#define UL_COMB_LIST(T) \
    T(S, 3, ul_eval_S) \
    T(K, 2, ul_eval_K) \
    T(I, 1, ul_eval_I) 

#define UL_OPCODE_LIST(T) \
    T(hlt) \
    T(push) \
    T(push1) \
    T(pop) \
    T(apply_S) \
    T(apply_K) \
    T(apply_I) \
    T(shuffle) \
    T(apply_unk) \

enum {
#define T(x) x,
    UL_OPCODE_LIST(T)
#undef T
};

/* Tagged pointer */
typedef uintptr_t ul_value_t;

#define UL_VAL_MASK 0x1
#define UL_VAL_CLOS 0x1
#define UL_VAL_COMB 0x0

struct ul_closure;

typedef struct ul_ctx {
    size_t heap_size;
    size_t stack_size;
    ul_value_t *sp;
    uint8_t *stack_base;
    uint8_t *gc_allocp;
    uint8_t *gc_from;
    uint8_t *gc_to;
    struct ul_closure *rt_val;
    uint8_t *rt_pc;
    dynbuf_t ul_bc;
} ul_ctx_t;

typedef struct {
    size_t n_captured;
    struct ul_closure *captured[];
} ul_env_t;

typedef void (*ul_closure_fn)(ul_env_t *env, ul_ctx_t *ctx);
typedef void (*ul_cont_fn)(ul_ctx_t *ctx);

typedef struct ul_closure {
    union  {
        ul_closure_fn clos_fn;
        struct ul_closure *fwd_ptr; /* for GC */
    };
    size_t arity;
    ul_env_t env;
} ul_closure_t;

size_t inline __attribute__((always_inline)) ul_closure_size(size_t n_args) {
    return sizeof(ul_closure_t) + n_args * sizeof(ul_closure_t *);
}

int ul_ctx_init(ul_ctx_t *ctx, size_t heap_size, size_t stack_size) {
    if ((ctx->gc_from = mmap(0, heap_size, PROT_READ | PROT_WRITE, MAP_ANON| MAP_PRIVATE, -1, 0)) == MAP_FAILED) {
        return -1;
    }
    if ((ctx->gc_to = mmap(0, heap_size, PROT_READ | PROT_WRITE, MAP_ANON| MAP_PRIVATE, -1, 0)) == MAP_FAILED) {
        goto error1;
    }
    if ((ctx->sp = mmap(0, stack_size, PROT_READ | PROT_WRITE, MAP_ANON| MAP_PRIVATE, -1, 0)) == MAP_FAILED) {
        goto error2;
    }
    /* cannot fail */
    ctx->heap_size = heap_size;
    ctx->stack_size = stack_size;
    ctx->stack_base = ctx->sp;
    ctx->gc_allocp = ctx->gc_from;
    ctx->rt_val = NULL;
    dynbuf_init(&ctx->ul_bc);
    return 0;
error2:
    munmap(ctx->gc_to, heap_size);
error1:
    munmap(ctx->gc_from, heap_size);
    return -1;
}

static ul_value_t gc_copy(ul_ctx_t *ctx, ul_value_t ul) {
#define GC_FWDPTR_TAG ((size_t) -1)
    ul_closure_t *old = (ul_closure_t *) ul;
    size_t req_size = ul_closure_size(old->env.n_captured);
    assert(ctx->gc_allocp + req_size < ctx->gc_to + ctx->heap_size);
    assert(old->env.n_captured != GC_FWDPTR_TAG);
    /* by the time we are here, the allocp is already pointing into to-space */
    ul_closure_t *new = (ul_closure_t *) ctx->gc_allocp;
    ctx->gc_allocp += req_size;
    memcpy(new, old, req_size);
    old->fwd_ptr = new;
    old->env.n_captured = GC_FWDPTR_TAG;
    return (ul_value_t) new;
}

static ul_value_t gc(ul_ctx_t *ctx) {
    uint8_t *scanp;
    scanp = ctx->gc_allocp = ctx->gc_to;
    ctx->rt_val = gc_copy(ctx, ctx->rt_val);
    while (scanp < ctx->gc_allocp) {
        ul_closure_t *scanned = (ul_closure_t *)scanp;
        for (size_t i = 0; i < scanned->env.n_captured; i++) {
            ul_closure_t *ptr = scanned->env.captured[i];
            if (ptr && ptr->env.n_captured == GC_FWDPTR_TAG) {
                scanned->env.captured[i] = ptr->fwd_ptr;
            } else {
                scanned->env.captured[i] = gc_copy(ctx, ptr);
            }
        }
        scanp += ul_closure_size(scanned->env.n_captured);
    }
    /* swap the two hemispace */
    uint8_t *t = ctx->gc_to;
    ctx->gc_to = ctx->gc_from;
    ctx->gc_from = t;
#undef GC_FWDPTR_TAG
}

ul_closure_t *ul_alloc(ul_ctx_t *ctx, size_t n_args) {
    size_t size = ul_closure_size(n_args);
    if (ctx->gc_allocp + size > ctx->gc_from + ctx->heap_size) {
        gc(ctx);
        if (ctx->gc_allocp + size > ctx->gc_from + ctx->heap_size) {
            return NULL;
        }
    }
    ul_closure_t *new = (ul_closure_t *) ctx->gc_allocp;
    ctx->gc_allocp += size;
    return new;
}

static inline __attribute__((always_inline)) void ul_push(ul_ctx_t *ctx, ul_value_t val) {
    assert(ctx->sp < ctx->stack_base);
    *ctx->sp++ = val;
}

static inline __attribute__((always_inline)) ul_value_t ul_pop(ul_ctx_t *ctx) {
    assert(ctx->sp > ctx->stack_base + ctx->stack_size);
    return *(--ctx->sp);
}

static void ul_apply_cont(ul_ctx_t *ctx) {
    size_t rest = ul_pop(ctx);
}


/* Sxyz = xz(yz) */
static void ul_eval_S(ul_ctx_t *ctx) {
    ul_value_t x, y, z;
    x = ctx->sp[0];
    y = ctx->sp[1];
    z = ctx->sp[2];
    // ctx->sp -= 3;
    // ctx->rt_val = z;
    // ul_push(ctx, (ul_value_t) z);
    // ul_push(ctx, (ul_value_t) x);
    // ul_push(ctx, (ul_value_t) &ul_S_cont2);
    // ul_push(ctx, (ul_value_t) y);
    // ul_push(ctx, (ul_value_t) &ul_S_cont1 | UL_VAL_COMB);
}

static void ul_eval_K(ul_ctx_t *ctx) {
    ctx->rt_val = ctx->sp[0];
    ctx->sp -= 2;
}
static void ul_eval_I(ul_ctx_t *ctx) {
    ctx->rt_val = ctx->sp[0];
    --ctx->sp;
}

static int ul_value_is_cont(ul_value_t val) {
    return val == &ul_apply_cont;
}

void ul_run(ul_ctx_t *ctx) {
    uint8_t *pc = ctx->ul_bc.data, op;
    ul_cont_fn kont;
    size_t nargs;
    ul_closure_t *clos;
    static struct {
        int arity;
        void (*eval_fn)(ul_ctx_t *);
    } *desc, comb_descs[] = { 
#define T(x, y, z) y,
        UL_COMB_LIST(T)
#undef T
    };
#define GET_NARGS() memcpy(&nargs, pc, sizeof(nargs))
#ifdef DIRECT_THREADING
    void *jmptbl[] = {
        #define T(op) &&jmptbl_##op,
        UL_OPCODE_LIST(T)
        #undef T
    };
    #define CASE(op) jmptbl_##op
    #define SWITCH(pc) goto *jmptbl[(op = *pc++)];
    #define BREAK goto *jmptbl[*pc++]
    #define CONTINUE goto *jmptbl[*pc++]
#else
    #define SWITCH(pc) switch((op = *pc++))
    #define CASE(op) op
    #define BREAK break
    #define CONTINUE goto decode
decode:
#endif
    SWITCH(pc) {
        CASE(push1):
            GET_NARGS();
            ctx->rt_val = nargs;
        CASE(push):
            ul_push(ctx, ctx->rt_val);
            BREAK;
        CASE(pop):
            ctx->rt_val = ul_pop(ctx);
            BREAK;
        CASE(apply_K):
        CASE(apply_S):
        CASE(apply_I):
            GET_NARGS();
            desc = &comb_descs[op - apply_K];
            if (nargs >= desc->arity) {
                if (nargs > desc->arity) {
                    /* we have to push a continuation on the stack */
                    ul_push(ctx, nargs - desc->arity);
                    ul_push(ctx, &ul_apply_cont);
                }
                desc->eval_fn(ctx);
            } else {
                /* we can set rt_val */
                clos = ul_alloc(ctx, nargs);
                assert(clos);
                clos->env.n_captured = nargs;
                for (size_t i = 0; i < nargs; i++) {
                }
                ctx->rt_val = clos;
            }
            /* always try to scrutinize the closure as unlambda is a strict language */
            goto scrutinize;
        CASE(apply_unk):
        CASE(shuffle):
        CASE(hlt):
            return;
    }
scrutinize:
    while (ul_value_is_cont(*ctx->sp)) {
        kont = (ul_cont_fn)ul_pop(ctx);
        kont(ctx);
    }
    CONTINUE;
}

int main() {
    ul_ctx_t ctx;
    assert(ul_ctx_init(&ctx, 4096, 4096) == 0);
    dynbuf_put_uint8_t(&ctx.ul_bc, hlt);
    ul_run(&ctx);
    return 0;
}