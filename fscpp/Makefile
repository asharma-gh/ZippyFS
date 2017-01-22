CC = g++
CFLAGS = -Wall `pkg-config fuse --cflags --libs glib-2.0` -lzip -g -D_FILE_OFFSET_BITS=64
OBJS = file-system.o

file-system: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o file-system

file-system.o: fuse_ops.h
	$(CC) $(CFLAGS) fuse_ops.cpp

.PHONY: all clean

all: $(PROGFILES)

clean:
	rm $(PROGFILES)
