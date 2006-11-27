# 
#
#

include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk


Targets = magnetometer

all: $(Targets)


$(Targets): % : %.o
	@echo "<**Linking**> $@ ..."
	@$(LD) $@.o $(LDFLAGS) $(ANITA_GPS_LIBS)  -o $@
	@chmod 555 $@

clean: objclean
	@-rm -f $(Targets) 




















