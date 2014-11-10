include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk

LIBS = -L.. $(ANITA_LIBS)

Targets =  Openportd 

$(Targets): % : %.o

all: $(Targets)

#driver:	driver.c  $(OBJS)
$(Targets): % : %.o $(OBJS)
	@echo "<**Linking**> $@ ..."
#	gcc $(CCFLAGS) -o driver driver.c $(OBJS) $(LIBS)
	@$(LD) $@.o $(OBJS) $(LDFLAGS) $(LIBS) -o $@
	@chmod 555 $@
	ln -sf $(shell pwd)/$@ ${ANITA_FLIGHT_SOFT_DIR}/bin


clean: objclean
	@rm -f $(Targets) 
