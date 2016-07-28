# 
#
#

include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk


Targets = readCalibAdu5DataFiles makeCalibAdu5DataFiles adu5_command adu5_file adu5_output g12_command adu5_test setG12PPS readCalibAdu5DataEFile

all: $(Targets)


$(Targets): % : %.o
	@echo "<**Linking**> $@ ..."
	@$(LD) $@.o $(LDFLAGS) $(ANITA_GPS_LIBS) -lm  -o $@
	@chmod 555 $@

clean: objclean
	@-rm -f $(Targets) 




















