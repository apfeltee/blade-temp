
##
# this file is fragile as all hell.
# need to switch to cmake/ninja, eventually.
##

INCFLAGS = -I.

CC = gcc -Wall -Wextra -Wshadow -Wunused-macros -Wunused-local-typedefs
#CFLAGS = $(INCFLAGS) -Ofast -march=native -flto -ffast-math -funroll-loops
CFLAGS = $(INCFLAGS) -O0 -g3 -ggdb3 
LDFLAGS = -flto -ldl -lm  -lreadline -lpthread -lpcre2-8
target = run

src = $(wildcard *.c)
rawobj = $(src:.c=.o)
obj = $(addprefix _obj/, $(rawobj))
# don't use addprefix here; it's already there
dep = $(obj:.o=.d)
dummy := $(shell mkdir -p _obj)


$(target): $(obj)
	$(CC) -o $@ $^ $(LDFLAGS)


-include $(dep)

.PHONY: all clean

pre-build:
	mkdir -p _obj


# rule to generate a dep file by using the C preprocessor
# (see man cpp for details on the -MM and -MT options)
_obj/%.d: %.c
	$(CC) $(CFLAGS) $< -MM -MT $(@:.d=.o) -MF $@

_obj/%.o: %.c
	$(CC) $(CFLAGS) -c $(DBGFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm -f $(obj) $(target)

.PHONY: cleandep
cleandep:
	rm -f $(dep)

.PHONY: rebuild
rebuild: clean cleandep $(target)

.PHONY: sanity
sanity:
	./run sanity.msl
