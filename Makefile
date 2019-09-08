SANITIZER=-fsanitize=address,undefined -DASAN

CFLAGS=-Werror -Wall -std=c99 -pedantic -g $(SANITIZER)
LDFLAGS=$(SANITIZER)

all: test_symtab test_list test_parse

test_symtab: test_symtab.o ul_symtab.o
test_parse: test_parse.o ul_parse.o ul_symtab.o

fmt:
	clang-format -i -style=file *.h *.c

clean:
	rm -f *.o test_symtab test_list
