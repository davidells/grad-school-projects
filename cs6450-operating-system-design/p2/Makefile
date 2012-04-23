CFLAGS = -Wall 
LDFLAGS = -g
CC = mpicc

all: osdrpc.o libosd1.so.0.0 libosd2.so.0.0

libosd1.o: libosd1.c
	$(CC) -fPIC $(CFLAGS) -c $<

libosd1.so.0.0:	libosd1.o
	$(CC) $(LDFLAGS) -shared -Wl,-soname,libosd1.so.0 -o $@ $< -lc
	ln -sf libosd1.so.0.0 libosd1.so.0
	ln -sf libosd1.so.0 libosd1.so

libosd2.o: libosd2.c
	$(CC) -fPIC $(CFLAGS) -c $<

libosd2.so.0.0:	libosd2.o
	$(CC) $(LDFLAGS) -shared -Wl,-soname,libosd2.so.0 -o $@ $< -lc
	ln -sf libosd2.so.0.0 libosd2.so.0
	ln -sf libosd2.so.0 libosd2.so

test1: test1.o osdrpc.o libosd1.so.0.0 libosd2.so.0.0
	$(CC) $(LDFLAGS) -o $@ $< osdrpc.o -ldl

clean:
	rm -f test1 *.o libosd?.so*

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
