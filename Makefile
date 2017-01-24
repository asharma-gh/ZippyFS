CC = g++
CFLAGS := -Wall `pkg-config fuse --cflags` -lzip -g -D_FILE_OFFSET_BITS=64
LDFLAGS := -g `pkg-config fuse --libs` -lzip -D_FILE_OFFSET_BITS=64
SRCS := $(wildcard %.c) $(wildcard %.cpp)
OBJS := $(patsubst %.c,%.o,$(wildcard *.c)) $(patsubst %.cpp, %.o, $(wildcard *.cpp))
DEP = $(wildcard %.h)

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
