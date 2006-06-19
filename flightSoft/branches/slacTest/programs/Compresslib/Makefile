
CXX	= g++
CXXFLAGS = -Wall -g -O2 -Wno-unused
CFLAGS= -g

all:	ftest btest packtest unpacktest testmedian packwavetest unpackwavetest

compresslib.o: compresslib.h compresslib.c
	gcc -c compresslib.c

packtest: packtest.o compresslib.o
	gcc -o packtest packtest.o compresslib.o

unpacktest: unpacktest.o compresslib.o
	gcc -o unpacktest unpacktest.o compresslib.o

ftest:	ftest.o compresslib.o
	gcc -o ftest ftest.o compresslib.o

btest:  btest.o compresslib.o
	gcc -o btest btest.o compresslib.o

testmedian: testmedian.o compresslib.o
	gcc -o testmedian testmedian.o compresslib.o

packwavetest: packwavetest.o compresslib.o
	gcc -o packwavetest packwavetest.o compresslib.o

unpackwavetest: unpackwavetest.o compresslib.o
	gcc -o unpackwavetest unpackwavetest.o compresslib.o

clean:
	/bin/rm -f  *.o	ftest btest packtest unpacktest testmedian packwavetest unpackwavetest

%.o:	%.cxx
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o:	%.cxx %.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

