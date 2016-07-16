# 
#
#

include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk


Targets =  RTLd RTL_singleshot_power

all: $(Targets)





$(Targets): % : %.o
	@echo "<**Linking**> $@ ..."
	@$(LD) $@.o $(LDFLAGS) $(ANITA_LIBS) $(ANITA_GPS_LIBS) -lrt -lrtlsdr -o $@
	@chmod 555 $@
	ln -sf $(shell pwd)/$@ ${ANITA_FLIGHT_SOFT_DIR}/bin

clean: objclean
	@-rm -f $(Targets) 




















