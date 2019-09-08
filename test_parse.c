/* Test for parser of unlambda.
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
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ul_parse.h"

static void run_test_case(const char *filename);

int main()
{
    run_test_case("t/fib.ul");
    run_test_case("t/hello.ul");
    puts("ok.");
}

void run_test_case(const char *filename)
{
    char buf[1024];
    int fd = open(filename, O_RDONLY);
    assert(fd != -1);
    read(fd, buf, 1024);
    ul_parse_state_t state = {buf, UL_PARSE_OK};
    ul_ast_t *ast = ul_parse_prog(&state);
    assert(ast);
    ul_ast_dump(ast, stdout);
    puts("");
    ul_ast_free(ast);
    return;
}
