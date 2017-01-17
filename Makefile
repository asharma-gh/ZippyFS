CC = g++
CFLAGS = -Wall `pkg-config fuse --cflags --libs glib-2.0` -lzip -g -D_FILE_OFFSET_BITS=64
SRCFILES = $(wildcard src/*.cpp) $(wildcard include/*.h)
PROGFILES = $(patsubst %.cpp, %, $(SRCFILES))

.PHONY: all clean

all: $(PROGFILES)

clean:
	rm $(PROGFILES)
