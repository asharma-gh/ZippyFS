#makefile is fuked

CC = g++
CFLAGS := -Wall `pkg-config fuse --cflags` -lzip -g -D_FILE_OFFSET_BITS=64
LDFLAGS := -g `pkg-config fuse --libs` -lzip -D_FILE_OFFSET_BITS=64
SRCS := fuse_ops.cpp block_cache.cpp Main.c util.cpp block.cpp
OBJS := fuse_ops.o block_cache.o Main.o util.o block.o
DEP = fuse_ops.h block_cache.h block.h util.h

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
