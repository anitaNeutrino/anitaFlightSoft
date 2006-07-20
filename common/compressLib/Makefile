# 
#
#

include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk

#SOFLAGS       = -shared


LIB_OBJS         =  compressLib.o 
Library       = $(ANITA_LIB_DIR)/libCompress.a
TestProgs     =  ftest btest packtest unpacktest testmedian packwavetest unpackwavetest

all: $(Library)
test: ${TestProgs}

$(Library): $(LIB_OBJS)
	@/bin/rm -f $(Library)	
#	@echo "Creating $(Library) ..."
	@ar -r $@ $^
#	@$(LD) $(LDFLAGS) $(LIBS) $(SOFLAGS) $(Objects) -o $(Library)
#	@chmod 555 $(Library)


$(TestProgs): % : %.o
	@echo "<**Linking**> $@ ..."
	@$(LD) $@.o $(LDFLAGS) -pg $(ANITA_LIBS) $(ANITA_HKD_LIBS) $(ANITA_GPS_LIBS) $(ANITA_COMP_LIBS) -o $@
	@chmod 555 $@

clean: objclean
	@/bin/rm -f $(Library)





















