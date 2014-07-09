
include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk


# Uncomment USE_STOR to use the stor library to save the data locally. Use
# only for debugging? You will have to link with -lstor if you use it.
USE_STOR =
#USE_STOR = -DUSE_STOR

CFLAGS = -g $(USE_STOR) -I/usr/inclyde/sys
#ND = /usr/local/src/new/naturaldocs-1.35/NaturalDocs

OBJS =  conf_simple.c crc_simple.c serial.c sipcom.c sipthrottle.c \
	telemwrap.c


Target       = $(ANITA_LIB_DIR)/libSipcom.a

$(Target): $(OBJS)
	@/bin/rm -f $(Target)
	@ar -r $@ $^	

all: $(Target)

#crc_simple.o:	crc_simple.h
#serial.o:	serial.h
#sipcom.o:	sipcom.h
#sipthrottle.o:	sipthrottle.h

test: $(Target)
	@cd demo; make

#install:	libsipcom.a
#	cp sipcom.h /usr/local/include
#	cp libsipcom.a /usr/local/lib

clean: objclean
	@/bin/rm -f ${LIB_OBJS}
	@/bin/rm -f $(Target)
	@cd demo; make clean

docs:
	$(ND) -i . -o html ./doc -p ./ndinfo -s mydefault
