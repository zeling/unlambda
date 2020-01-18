/* Cheney on the MTA!
 *
 * MIT License
 *
 * Copyright (c) 2020 Zeling Feng
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
#include <alloca.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <ucontext.h>

#define ul_noreturn __attribute__((noreturn))
#define ul_force_inline __attribute__((always_inline))

#define DEBUG 0

#define N_CLOSURE(N) (sizeof(ul_closure_t *) * (N))

typedef uintptr_t ul_value_t;

struct ul_closure;
typedef struct {
    size_t n_captured;
    struct ul_closure *captured[];
} ul_env_t;

typedef void (*ul_closure_fn)(struct ul_closure *cont, ul_env_t *env, size_t n_args, struct ul_closure *args[]);
typedef void (*ul_cont_fn)(ul_env_t *env, struct ul_closure *arg);

typedef struct ul_closure {
    union  {
        ul_closure_fn clos_fn;
        ul_cont_fn cont_fn;
        struct ul_closure *fwd_ptr;
    };
    ul_env_t env;
} ul_closure_t;

#define ALLOC_CLOS(clos, fn, n_cap) \
    ul_closure_t *clos = alloca(sizeof(ul_closure_t) + N_CLOSURE(n_cap)); \
    clos->clos_fn = fn; \
    clos->env.n_captured = (n_cap)

#define ALLOC_CONT(cont, fn, n_cap) \
    ul_closure_t *cont = alloca(sizeof(ul_closure_t) + N_CLOSURE(n_cap)); \
    cont->cont_fn = fn; \
    cont->env.n_captured = n_cap

static void ul_K(ul_closure_t *cont, ul_env_t *env, size_t n_args, ul_closure_t *args[]);
static void ul_I(ul_closure_t *cont, ul_env_t *env, size_t n_args, ul_closure_t *args[]);
static void ul_S(ul_closure_t *cont, ul_env_t *env, size_t n_args, ul_closure_t *args[]);

/* GC */
static ul_closure_t *gc_cont, *gc_clos;
static ucontext_t ul_ctx, gc_ctx;
static char gc_stk[1024];
static void gc(ul_closure_t *cont, ul_closure_t *clos);
static void gc_main();
static char *stk_to, *stk_from;
#define STK_SIZE (64 * 1024)
#define STK_GC_THRES 0x1000


static void dump_clos(ul_closure_t *clos) {
    if (clos->clos_fn == &ul_K) {
        puts("K");
    } else if (clos->clos_fn == &ul_S) {
        puts("S");
    } else if (clos->clos_fn == &ul_I) {
        puts("I");
    } else {
        puts("Not possible");
        exit(1);
    }
}

static inline ul_force_inline void apply_cont(ul_closure_t *cont, ul_closure_t *clos) {
    int dumb;
    if ((uintptr_t) &dumb < (uintptr_t) stk_from + STK_GC_THRES) {
        gc(cont, clos);
    } else {
        cont->cont_fn(&cont->env, clos);
    }
}

static void resume_application_cont_fn(ul_env_t *env, ul_closure_t *clos);

static inline ul_force_inline void apply_clos(ul_closure_t *clos, ul_closure_t *cont, size_t n_args, ul_closure_t *args[]) {
    int dumb;
    #if DEBUG
        printf("diff: %lu\n", (uintptr_t) &dumb - (uintptr_t) stk_from);
    #endif
    if ((uintptr_t) &dumb < (uintptr_t) stk_from + STK_GC_THRES + sizeof(ul_closure_t) + N_CLOSURE(n_args + 2)) {
        ALLOC_CONT(kont, &resume_application_cont_fn, n_args + 2);
        kont->env.captured[0] = clos;
        kont->env.captured[1] = cont;
        memcpy(kont->env.captured + 2, args, N_CLOSURE(n_args));
        gc(kont, NULL);
    } else {
        if (clos->env.n_captured + n_args == 0) {
            apply_cont(cont, clos);
        } else {
    #if DEBUG
            dump_clos(clos);
    #endif
            clos->clos_fn(cont, &clos->env, n_args, args);
        }
    }
}

static void resume_application_cont_fn(ul_env_t *env, ul_closure_t *clos) {
    apply_clos(env->captured[0], env->captured[1], env->n_captured - 2, env->captured + 2);
}

static void ul_K(ul_closure_t *cont, ul_env_t *env, size_t n_args, ul_closure_t *args[]) {
    assert(env->n_captured < 2);
    if (env->n_captured + n_args >= 2) {
        ul_closure_t *x = env->n_captured == 0? args[0] : env->captured[0];
        return apply_clos(x, cont, env->n_captured + n_args - 2, args + 2 - env->n_captured);
    } else {
        ALLOC_CLOS(clos, &ul_K, env->n_captured + n_args);
        memcpy(&clos->env.captured, env->captured, N_CLOSURE(env->n_captured));
        memcpy(&clos->env.captured[env->n_captured], args, N_CLOSURE(n_args));
        return apply_cont(cont, clos);
    }
}

static void ul_I(ul_closure_t *cont, ul_env_t *env, size_t n_args, ul_closure_t *args[]) {
    if (n_args == 1) {
        return apply_cont(cont, args[0]);
    } else {
        return apply_clos(args[0], cont, n_args - 1, args + 1);
    }
}

static void ul_S_cont(ul_env_t *env, ul_closure_t *clos) {
    ul_closure_t *cont = env->captured[0];
    ul_closure_t *x = env->captured[1];
    ul_closure_t *z = env->captured[2];
    env->captured[1] = z;
    env->captured[2] = clos;
    return apply_clos(x, cont, env->n_captured - 1, env->captured + 1);
}

static void ul_S(ul_closure_t *cont, ul_env_t *env, size_t n_args, ul_closure_t *args[]) {
    assert(env->n_captured < 3);
    if (env->n_captured + n_args >= 3) {
        ul_closure_t *y, *z;
        ALLOC_CONT(kont, &ul_S_cont, env->n_captured + n_args);
        kont->env.captured[0] = cont;
        kont->env.captured[1] = env->n_captured == 0? args[0] : env->captured[0];
        y = env->n_captured == 2? env->captured[1] : args[1 - env->n_captured];
        z = kont->env.captured[2] = args[2 - env->n_captured];
        return apply_clos(y, kont, 1, &z);
    } else {
        ALLOC_CLOS(clos, &ul_S, env->n_captured + n_args);
        memcpy(&clos->env.captured, env->captured, N_CLOSURE(env->n_captured));
        memcpy(&clos->env.captured[env->n_captured], args, N_CLOSURE(n_args));
        return apply_cont(cont, clos);
    }
}

static ul_closure_t I = {
    .clos_fn = &ul_I,
    .env = {
        .n_captured = 0,
    },
};

static ul_closure_t K = {
    .clos_fn = &ul_K,
    .env = {
        .n_captured = 0,
    },
};

static ul_closure_t S = {
    .clos_fn = &ul_S,
    .env = {
        .n_captured = 0,
    }
};

static void end_cont_fn(ul_env_t *env_t, ul_closure_t *clos) {
    dump_clos(clos);
}

static ul_closure_t end_cont = {
    .cont_fn = end_cont_fn,
    .env = {
        .n_captured = 0,
    },
};


static void SII_cont_fn(ul_env_t *env_t, ul_closure_t *clos) {
    ul_closure_t *args[3] = { &I, &I, clos };
    apply_clos(&S, &end_cont, 3, args);
}

static ul_closure_t SII_cont = {
    .cont_fn = SII_cont_fn,
    .env = {
        .n_captured = 0,
    }
};

static void ul_main(ul_closure_t *cont, ul_closure_t *clos) {
    apply_cont(cont, clos);
}

int main() {
    stk_to = mmap(0, STK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    stk_from = mmap(0, STK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    assert(stk_to != MAP_FAILED && stk_from != MAP_FAILED);
    ALLOC_CONT(cont, &resume_application_cont_fn, 4);
    cont->env.captured[0] = &S,
    cont->env.captured[1] = &SII_cont,
    cont->env.captured[2] = &I,
    cont->env.captured[3] = &I,
    getcontext(&gc_ctx);
    gc_ctx.uc_stack.ss_sp = &gc_stk;
    gc_ctx.uc_stack.ss_size = 1024;
    gc_ctx.uc_link = NULL;
    makecontext(&gc_ctx, (void (*)())&gc_main, 0);
    getcontext(&ul_ctx);
    ul_ctx.uc_stack.ss_sp = stk_from;
    ul_ctx.uc_stack.ss_size = STK_SIZE;
    ul_ctx.uc_link = NULL;
    makecontext(&ul_ctx, (void (*)())&ul_main, 2, cont, NULL);
    setcontext(&ul_ctx);
    return 0;
}

static char *allocp;
static inline ul_force_inline ul_closure_t *copy(ul_closure_t *c) {
#define GC_FWDPTR_TAG (-1UL)
    allocp -= sizeof(ul_closure_t) + N_CLOSURE(c->env.n_captured);
    // memcpy(allocp, c, sizeof(ul_closure_t) + N_CLOSURE(c->env.n_captured));
    ((ul_closure_t *) allocp)->clos_fn = c->clos_fn;
    ((ul_closure_t *) allocp)->env.n_captured = c->env.n_captured;
    for (size_t i = 0; i < c->env.n_captured; i++) {
        ((ul_closure_t *) allocp)->env.captured[i] = c->env.captured[i];
    }
    c->fwd_ptr = (ul_closure_t *) allocp;
    c->env.n_captured = GC_FWDPTR_TAG;
    return c->fwd_ptr;
}

void gc(ul_closure_t *cont, ul_closure_t *clos) {
    gc_cont = cont;
    gc_clos = clos;
    swapcontext(&ul_ctx, &gc_ctx);
}

void gc_main() {
    static char *scan_limit, *scanp, *old_allocp;
    static int need_scan;
    need_scan = 1;
    scan_limit = allocp = stk_to + STK_SIZE;
    gc_cont = copy(gc_cont);
    if(gc_clos) gc_clos = copy(gc_clos);
    while (need_scan) {
        need_scan = 0;
        old_allocp = scanp = allocp;
        while (scanp < scan_limit) {
            ul_closure_t *const scanned = (ul_closure_t *)scanp;
            for (size_t i = 0; i < scanned->env.n_captured; i++) {
                if (scanned->env.captured[i]->env.n_captured == GC_FWDPTR_TAG) {
                    scanned->env.captured[i] = scanned->env.captured[i]->fwd_ptr;
                } else {
                    scanned->env.captured[i] = copy(scanned->env.captured[i]);
                    need_scan = 1;
                }
            }
            scanp += sizeof(ul_closure_t) + N_CLOSURE(scanned->env.n_captured);
        }
        scan_limit = old_allocp;
    }
    ul_ctx.uc_stack.ss_sp = stk_to;
    ul_ctx.uc_stack.ss_size = allocp - stk_to;
    char *tmp = stk_to;
    stk_to = stk_from;
    stk_from = tmp;
    makecontext(&ul_ctx, (void (*)())&ul_main, 2, gc_cont, gc_clos);
    setcontext(&ul_ctx);
#undef GC_FWDPTR_TAG
}
