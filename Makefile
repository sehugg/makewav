.SUFFIXES: .exe .o .c .pc

all:makewav

queue.o:
	gcc -c queue.c

makewav: queue.o
	gcc -lm -DLINUX -O queue.o -o makewav makewav.c
