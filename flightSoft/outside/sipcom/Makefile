# Uncomment USE_STOR to use the stor library to save the data locally. Use
# only for debugging? You will have to link with -lstor if you use it.
USE_STOR =
#USE_STOR = -DUSE_STOR

CFLAGS = -g $(USE_STOR)
ND = /usr/local/src/new/naturaldocs-1.35/NaturalDocs

OBJS = crc_simple.o serial.o sipcom.o sipthrottle.o

libsipcom.a:	$(OBJS)
	ar r libsipcom.a $(OBJS)

crc_simple.o:	crc_simple.h
serial.o:	serial.h
sipcom.o:	sipcom.h
sipthrottle.o:	sipthrottle.h

driver: libsipcom.a
	cd demo; make

install:	libsipcom.a
	cp sipcom.h /usr/local/include
	cp libsipcom.a /usr/local/lib

clean:
	rm -f libsipcom.a *.o
	cd demo; make clean

docs:
	$(ND) -i . -o html ./doc -p ./ndinfo -s mydefault
