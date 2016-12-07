CC = gcc
CFLAGS = -Wall `pkg-config fuse --cflags --libs` -lzip -g -D_FILE_OFFSET_BITS=64
SRCFILES = $(wildcard *.c)
PROGFILES = $(patsubst %.c, %, $(SRCFILES))

.PHONY: all clean

all: $(PROGFILES)

clean:
	rm $(PROGFILES)
