export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig

CC=g++
CFLAGS= -Wall -O2 -Ipthread $(shell pkg-config --cflags sdl2 SDL2_image)
LIBS=-Lpthread $(shell pkg-config --libs sdl2 SDL2_image)
RM=rm -rf
TARGET=test
#FILES=$(wildcard *.c)
OBJS=$(patsubst %.cpp,%.o,$(wildcard *cpp))

all:$(OBJS)
	$(CC) $(CFLAGS) $(LIBS) -o $(TARGET) $(OBJS)

%.o:%.cpp
	$(CC) $(CFLAGS) $(LIBS) -c $< -o $@ 

clean:
	$(RM) $(OBJS) $(TARGET)

.PHONY:clean

