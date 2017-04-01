CC = g++
CFLAGS := -Wall `pkg-config fuse --cflags` -lsodium -g -D_FILE_OFFSET_BITS=64
LDFLAGS := -g `pkg-config fuse --libs` -lsodium -D_FILE_OFFSET_BITS=64
SRCS := $(wildcard %.c) $(wildcard %.cpp) $(wildcard src/*.cpp) $(wildcard src/*.c)
OBJS := $(patsubst %.c,%.o,$(wildcard *.c)) $(patsubst %.cpp, %.o, $(wildcard src/*.cpp))
DEP = $(wildcard %.h)

all:
	(make -j bt)
bt: $(OBJS)
	g++ -o bt $(OBJS) $(LDFLAGS)

%.o: %.c fuse_ops.h
	gcc -c -o $@ -Wall $< $(CFLAGS)

%.o: %.cpp $(DEP)
	g++ --std=gnu++11 -c -o $@ $< $(CFLAGS)

clean:
	rm bt *.o
