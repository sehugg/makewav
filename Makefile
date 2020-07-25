.SUFFIXES: .exe .o .c .pc

all:makewav

#queue.o:
#	$(CC) -c queue.c

makewav: queue.o streambuffer.o
	$(CC) $(LDFLAGS) -lm -O queue.o streambuffer.o -o makewav makewav.c
