CFLAGS = -Wall 
LDFLAGS = -g
CC = mpicc

all: libdsm.a

libdsm.o: libdsm.c
	$(CC) $(CFLAGS) -c $<

libdsm.a:	libdsm.o
	ar rcs libdsm.a  libdsm.o

libdsm: libdsm.a

test: test.c libdsm.a
	$(CC) --static libdsm.a test.c -o test -ldsm -L. $(CFLAGS)

clean:
	rm -f *.o
	rm -f *.a
	rm -f test
