#makefile is fuked

CC = g++
CFLAGS = -Wall `pkg-config fuse --cflags` -lzip -g -D_FILE_OFFSET_BITS=64
SRCS := $(wildcard *c) $(wildcard *.cpp)
OBJS := DEP = fuse_ops.h block_cache.h block.h util.h

*.o: $(DEPS)
	$(CC) $(CXXFLAGS) $(SRCFILES)
