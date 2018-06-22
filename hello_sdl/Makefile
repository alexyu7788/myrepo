export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig

CC=gcc
CFLAGS= -Wall -O2 -Ipthread $(shell pkg-config --cflags sdl2)
LIBS=-Lpthread $(shell pkg-config --libs sdl2)
RM=rm -rf
TARGET=hellow
#FILES=$(wildcard *.c)
OBJS=$(patsubst %.c,%.o,$(wildcard *c))

all:$(OBJS)
	$(CC) $(CFLAGS) $(LIBS) -o $(TARGET) $(OBJS)

%.o:%.c
	$(CC) $(CFLAGS) $(LIBS) -c $< -o $@ 

clean:
	$(RM) $(OBJS) $(TARGET)

#.PHONY:clean
