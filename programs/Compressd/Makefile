
CXX	= g++
CXXFLAGS = -Wall -g -O2 -Wno-unused
CFLAGS= -g

all:	ftest packtest unpacktest 

packtest: packtest.o fibonacci.o bytepack.o
	gcc -o packtest packtest.o fibonacci.o bytepack.o

unpacktest: unpacktest.o fibonacci.o byteunpack.o
	gcc -o unpacktest unpacktest.o fibonacci.o byteunpack.o

ftest:	ftest.o fibonacci.o
	gcc -o ftest ftest.o fibonacci.o

btest:  btest.o bifurcate.o fibonacci.o
	gcc -o btest btest.o bifurcate.o fibonacci.o

clean:
	/bin/rm -f ftest packtest unpacktest hpacktest *.o	

%.o:	%.cxx
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o:	%.cxx %.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

