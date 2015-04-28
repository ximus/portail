LIB_INSTALL=$(HOME)/lib
INCLUDE_INSTALL=$(HOME)/include

LIBS=-L/usr/local/lib -L/usr/lib/ -L. -lcunit
INCLUDE=-I/usr/local/include -I/usr/include

#CXX=g++
CXX=msp430-gcc
CXXFLAGS=-Wall -DDEBUG -std=c++11

#CC=gcc
CC=msp430-gcc
CFLAGS=-Wall -std=c99 -DDEBUG

default: nethelper.o staticlib test

test: libcantcoap.a test.cpp
	$(CXX) $(CXXFLAGS) test.cpp -o test -lcantcoap $(LIBS) $(INCLUDE)

cantcoap.o: cantcoap.cpp
	$(CXX) $(CXXFLAGS) $^ -c -o $@ $(LIBS) $(INCLUDE)

nethelper.o: nethelper.c
	$(CC) $(CFLAGS) -c $^ -c -o $@ $(INCLUDE) $(LIBS)

staticlib: libcantcoap.a

libcantcoap.a: cantcoap.o
	ar -rc libcantcoap.a $^

clean:
	rm *.o; rm test; rm libcantcoap.a

install:
	cp libcantcoap.a $(LIB_INSTALL)/
	cp cantcoap.h $(INCLUDE_INSTALL)/
