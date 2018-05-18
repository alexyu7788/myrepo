CC= g++
RM=rm

CFLAGS += -Wall -g -O2 
CFLAGS := $(shell pkg-config --cflags gsl protobuf libprotobuf-c) $(CFLAGS)
LDLIBS := $(shell pkg-config --libs gsl protobuf libprotobuf-c) $(LDLIBS)
#CFLAGS := $(shell pkg-config --cflags gsl protobuf) $(CFLAGS)
#LDLIBS := $(shell pkg-config --libs gsl protobuf) $(LDLIBS)
EXAMPLES= gen_param

SRC := $(wildcard *.c)
OBJS := $(SRC:.c=.o)

$(EXAMPLES): $(OBJS)
	$(CC) $(OBJS) $(LDLIBS) $(CFLAGS) -o $@

%.o: %.c
	$(CC) $< $(CFLAGS) $(LDLIBS) -c

.PHONY= clean

clean:
	$(RM) $(OBJS) $(EXAMPLES)
