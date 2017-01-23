CC = g++
CFLAGS = -Wall `pkg-config fuse --cflags --libs glib-2.0` -lzip -g -D_FILE_OFFSET_BITS=64
SRCFILES = fuse_ops.cpp block_cache.cpp block.cpp Main.c util.cpp
OBJS = fuse_ops.o block_cache.o block.o Main.o util.o
DEP = fuse_ops.h block_cache.h block.h util.h

*.o: $(DEPS)
	$(CC) $(CFLAGS) $(SRCFILES)
