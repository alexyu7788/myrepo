CC = gcc
CFLAGS = -O3 -fPIC $(shell pkg-config --cflags gsl)
LDLIBS = $(shell pkg-config --libs gsl)
INSTALL_PREFIX = /usr/local

ICA: fastICA.c libICA.so
	$(CC) $(CFLAGS) $(LDLIBS) fastICA.c -o fastICA -L. -lICA 

libICA.so: libICA.o svdcmp.o matrix.o
	$(CC) $(CFLAGS) $(LDLIBS) -shared -Wl,-soname,libICA.so -o libICA.so svdcmp.o \
	matrix.o libICA.o -lm
libICA.o: libICA.c libICA.h
	$(CC) $(CFLAGS)  $(LDLIBS) -c libICA.c -o libICA.o
matrix.o: matrix.c matrix.h
	$(CC) $(CFLAGS) $(LDLIBS) -c matrix.c -o matrix.o
svdcmp.o: svdcmp.c svdcmp.h
	$(CC) $(CFLAGS) $(LDLIBS) -c svdcmp.c -o svdcmp.o

install:
	cp libICA.so $(INSTALL_PREFIX)/lib/libICA.so
	cp libICA.h  $(INSTALL_PREFIX)/include/libICA.h

clean:
	rm *.o libICA.so fastICA
