SANITIZER=-fsanitize=address,undefined -DASAN

CFLAGS=-Wall -std=gnu99 -g $(SANITIZER)
LDFLAGS=$(SANITIZER)

all: test_symtab test_list test_parse ul_rt

test_symtab: test_symtab.o ul_symtab.o
test_parse: test_parse.o ul_parse.o ul_symtab.o
ul: ul.o
ul_rt: ul_rt.o

fmt:
	clang-format -i -style=file *.h *.c

clean:
	rm -f *.o test_symtab test_list
