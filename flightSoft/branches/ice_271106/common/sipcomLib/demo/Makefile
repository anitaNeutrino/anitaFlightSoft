include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk

#CFLAGS = -g -I..
LIBS = -L.. $(ANITA_LIBS) $(ANITA_SIP_LIBS) -lpthread

OBJS = rand_no.o

Targets = driver


$(Targets): % : %.o

Target = driver

all: $(Target)

#driver:	driver.c  $(OBJS)
$(Targets): % : %.o $(OBJS)
	@echo "<**Linking**> $@ ..."
#	gcc $(CCFLAGS) -o driver driver.c $(OBJS) $(LIBS)
	@$(LD) $@.o $(OBJS) $(LDFLAGS) $(LIBS) -o $@


rand_no.o:	rand_no.h

clean: objclean
	@rm -f driver *.o
