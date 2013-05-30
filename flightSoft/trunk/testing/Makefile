# 
#
#

include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk


Targets =  simpleLog sillyTest simpleLOSTest simplePacketReader unpackFilesForPrioritizerd ridiculousRunLog makePedestalFilesFromRun dumpCurrentPeds unpackFilesForArchived dumpEventTelemFile dumpLabPedFile testZipping slac2ice simpleSlowRateTest testWriting testDiskWritingWithCheck linkWatchTest testLogWatchd testGpsGeom  testRecursiveWatching sbsTempLog #bzTest #checksumTest

all: $(Targets)


$(Targets): % : %.o
	@echo "<**Linking**> $@ ..."
#	$(LD)  $@.o $(LDFLAGS) $(ANITA_COMP_LIBS) $(ANITA_LIBS) $(BZ_LIB) -lreadline -lcurses -o $@
	$(LD)  $@.o $(LDFLAGS) $(ANITA_COMP_LIBS) $(ANITA_LIBS) $(ANITA_MAP_LIBS) -lreadline  -lcurses -lAnitaFtp -lAnitaSbsTemp -o $@
	@chmod 555 $@

clean: objclean
	@-rm -f $(Targets) 




















