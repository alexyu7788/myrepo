export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig

CC=g++
CFLAGS= -Wall -O2 -Ipthread $(shell pkg-config --cflags sdl2 SDL2_image SDL2_ttf gsl protobuf libprotobuf-c)
LIBS=-Lpthread $(shell pkg-config --libs sdl2 SDL2_image SDL2_ttf gsl protobuf libprotobuf-c)
RM=rm -rf
TARGET=fcws
#FILES=$(wildcard *.c)
OBJS=$(patsubst %.cpp,%.o,$(wildcard *cpp))

all:$(OBJS)
	$(CC) $(CFLAGS) $(LIBS) -o $(TARGET) $(OBJS)

%.o:%.cpp
	$(CC) $(CFLAGS) $(LIBS) -c $< -o $@ 

clean:
	$(RM) $(OBJS) $(TARGET)

.PHONY:clean

