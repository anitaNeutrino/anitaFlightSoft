# 
#
#

include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk

CCFLAGS += -g
LDFLAGS += -g

Targets =  RTLd RTL_singleshot_power

all: $(Targets)





$(Targets): % : %.o RTL_common.o
	@echo "<**Linking**> $@ ..."
	@$(LD) $@.o $(LDFLAGS) $(ANITA_LIBS) -lrt -Wl,-Bstatic -lrtlsdr -Wl,-Bdynamic  -lusb-1.0 RTL_common.o -o $@ 
	@chmod 555 $@
	ln -sf $(shell pwd)/$@ ${ANITA_FLIGHT_SOFT_DIR}/bin

clean: objclean
	@-rm -f $(Targets) 




















