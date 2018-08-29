export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig

CC=g++
CFLAGS= -Wall -O2 -Ipthread -fPIC $(shell pkg-config --cflags sdl2 SDL2_image SDL2_ttf gsl protobuf libprotobuf-c)
LIBS=-Lpthread $(shell pkg-config --libs sdl2 SDL2_image SDL2_ttf gsl protobuf libprotobuf-c)
LDFLAGS=-shared
RM=rm -rf
TARGET_LIB=fcws.so
#FILES=$(wildcard *.c)
OBJS=$(patsubst %.cpp,%.o,$(wildcard *cpp))

all:$(OBJS)
	$(CC) $(CFLAGS) $(LIBS) ${LDFLAGS} -o $(TARGET_LIB) $(OBJS)

%.o:%.cpp
	$(CC) $(CFLAGS) $(LIBS) -c $< -o $@ 

clean:
	$(RM) $(OBJS) $(TARGET_LIB)

.PHONY:clean

