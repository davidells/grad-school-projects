CFLAGS = -Wall 
LDFLAGS = -g
CC = gcc

all: p3

libp3d.o: libp3d.c
	@echo "*  Use -fPIC to build an object file for a shared library."
	gcc -fPIC $(CFLAGS) -c $<

libp3d.so.0.0:	libp3d.o
	@echo "*  Link libp3d against libc for best results on all systems."
	gcc $(LDFLAGS) -shared -Wl,-soname,libp3d.so.0 -o $@ $< -lc
	@echo "*  Create a pointer from the soname to the library."
	ln -sf libp3d.so.0.0 libp3d.so.0
	@echo "*  Create a pointer for the linker."
	ln -sf libp3d.so.0 libp3d.so
	@echo "*  You would run ldconfig to make this a 'permanent' library."

p3: p3.o libp3d.so.0.0
	gcc $(LDFLAGS) -o $@ $< -ldl
	@echo "*  To run p3: LD_LIBRARY_PATH=`pwd` ./$@"
	@echo " "

clean:
	rm -f p3 *.o libp3d.so*

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
