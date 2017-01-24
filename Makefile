#makefile is fuked

CC = g++
CFLAGS := -Wall `pkg-config fuse --cflags` -lzip -g -D_FILE_OFFSET_BITS=64
LDFLAGS := -g `pkg-config fuse --libs` -lzip -D_FILE_OFFSET_BITS=64
SRCS := $(wildcard %.c) $(wildcard %.cpp) # fuse_ops.cpp block_cache.cpp Main.c util.cpp block.cpp inode.cpp
OBJS := $(patsubst %.c,%.o,$(wildcard *.c)) $(patsubst %.cpp, %.o, $(wildcard *.cpp))
 #fuse_ops.o block_cache.o Main.o util.o block.o inode.o
DEP = $(wildcard %.h) #fuse_ops.h block_cache.h block.h util.h inode.h

all:
	(make -j file-system)
file-system: $(OBJS)
	g++ -o file-system $(OBJS) $(LDFLAGS)

%.o: %.c fuse_ops.h
	gcc -c -o $@ -Wall $< $(CFLAGS)

%.o: %.cpp $(DEP)
	g++ --std=gnu++11 -c -o $@ $< $(CFLAGS)

clean:
	rm file-system *.o
