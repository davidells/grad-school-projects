CC = gcc

all: parallel6

parallel6: parallel6.o
	$(CC) -o $@ $< -lpthread 

clean:
	rm -f *.o parallel6
