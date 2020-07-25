.SUFFIXES: .exe .o .c .pc

all:makewav

#queue.o:
#	$(CC) -c queue.c

makewav: queue.o
	$(CC) $(LDFLAGS) -lm -O queue.o -o makewav makewav.c
