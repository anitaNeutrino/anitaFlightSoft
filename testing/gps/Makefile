# 
#
#

include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk


Targets =  gpsSimulator

all: $(Targets)


$(Targets): % : %.o
	@echo "<**Linking**> $@ ..."
#	$(LD)  $@.o $(LDFLAGS) $(ANITA_COMP_LIBS) $(ANITA_LIBS) $(BZ_LIB) -lreadline -lcurses -o $@
	$(LD)  $@.o $(LDFLAGS) $(ANITA_GPS_LIBS) $(ANITA_LIBS) -lreadline  -lcurses -o $@
	@chmod 555 $@

clean: objclean
	@-rm -f $(Targets) 




















