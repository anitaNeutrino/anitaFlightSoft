# 
#
#

include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk

Targets =  RTLd RTL_singleshot_power RTL_dump 

all: $(Targets)





$(Targets): % : %.o RTL_common.o
	@echo "<**Linking**> $@ ..."
	$(LD) $@.o RTL_common.o $(LDFLAGS) $(ANITA_LIBS) $(ANITA_RTL_LIBS)  -o $@ 
	@chmod 555 $@
	ln -sf $(shell pwd)/$@ ${ANITA_FLIGHT_SOFT_DIR}/bin

clean: objclean
	@-rm -f $(Targets) 




















