
CXX	= g++
CXXFLAGS = -Wall -g -O2 -Wno-unused
CFLAGS= -g

all:	ftest btest packtest unpacktest testmedian packwavetest

packtest: packtest.o fibonacci.o bytepack.o bifurcate.o
	gcc -o packtest packtest.o bifurcate.o fibonacci.o bytepack.o

unpacktest: unpacktest.o fibonacci.o byteunpack.o bifurcate.o
	gcc -o unpacktest unpacktest.o bifurcate.o fibonacci.o byteunpack.o

ftest:	ftest.o fibonacci.o
	gcc -o ftest ftest.o fibonacci.o

btest:  btest.o bifurcate.o fibonacci.o
	gcc -o btest btest.o bifurcate.o fibonacci.o

testmedian: testmedian.o findmedian.o
	gcc -o testmedian testmedian.o findmedian.o

packwavetest: packwavetest.o packwave.o bitpack.o bitstrip.o findmedian.o fibonacci.o bytepack.o bifurcate.o
	gcc -o packwavetest packwavetest.o packwave.o bitpack.o bitstrip.o findmedian.o fibonacci.o bytepack.o bifurcate.o

clean:
	/bin/rm -f ftest btest packtest unpacktest *.o	

%.o:	%.cxx
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o:	%.cxx %.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

