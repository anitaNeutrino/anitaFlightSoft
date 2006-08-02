# 
#
#

include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk


Targets =  sillyTest simpleLOSTest simplePacketReader unpackFilesForPrioritizerd ridiculousRunLog makePedestalFilesFromRun dumpCurrentPeds unpackFilesForArchived dumpEventTelemFile #checksumTest

all: $(Targets)


$(Targets): % : %.o
	@echo "<**Linking**> $@ ..."
	$(LD)  $@.o $(LDFLAGS) $(ANITA_COMP_LIBS) $(ANITA_LIBS)  -o $@
	@chmod 555 $@

clean: objclean
	@-rm -f $(Targets) 




















