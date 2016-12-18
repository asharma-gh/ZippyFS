CC = gcc
CFLAGS = -Wall `pkg-config fuse --cflags --libs glib-2.0` -lzip -g -D_FILE_OFFSET_BITS=64
SRCFILES = $(wildcard *.c)
PROGFILES = $(patsubst %.c, %, $(SRCFILES))

.PHONY: all clean

all: $(PROGFILES)

clean:
	rm $(PROGFILES)
