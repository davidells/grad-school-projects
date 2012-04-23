CFLAGS = -Wall 
LDFLAGS = -g
CC = gcc
PROG_NAME = dfs-search
SOURCES = dfsmain.c tree.c stack.c list.c
OBJECTS = $(SOURCES:.c=.o)

all: $(PROG_NAME)

randints: randints.o

index-search: index-search.o
	$(CC) $(LDFLAGS) -o $@ $< -lpthread

$(PROG_NAME): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) -lpthread

clean:
	rm -f $(PROG_NAME) randints index-search *.o

expand:
	@for n in *.c; do \
	    expand $$n > $$n.foo; \
	    if cmp -s $$n $$n.foo ; then \
		rm -f $$n.foo; \
	    else  \
		echo "Updating $$n"; \
		mv $$n.foo $$n; \
	    fi \
	done 
