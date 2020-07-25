.SUFFIXES: .exe .o .c .pc

all:makewav

#queue.o:
#	$(CC) -c queue.c

makewav: queue.o
	$(CC) $(LDFLAGS) -lm -DLINUX -O queue.o -o makewav makewav.c
