# 
#
#

include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk


LIB_OBJS         =  compressLib.o pack12bit.o fibonacci.o compUtils.o mulaw.o
Library       = $(ANITA_LIB_DIR)/libCompress.a
DynLib = $(ANITA_LIB_DIR)/libCompress.${DllSuf}
TestProgs     =  ftest btest packtest unpacktest testmedian packwavetest unpackwavetest
all: $(Library) $(DynLib)
test: ${TestProgs}

$(Library): $(LIB_OBJS)
	@/bin/rm -f $(Library)	
#	@echo "Creating $(Library) ..."
	@ar -r $@ $^
#	@$(LD) $(LDFLAGS) $(LIBS) $(SOFLAGS) $(Objects) -o $(Library)
#	@chmod 555 $(Library)

$(DynLib): $(LIB_OBJS)
	@/bin/rm -f $(DynLib)	
	@echo "Creating $(DynLib) ..."
	@$(LD) $(LDFLAGS) $(LIBS) $(SOFLAGS) $(LIB_OBJS) -o $(DynLib)
	@chmod 555 $(DynLib)


$(TestProgs): % : %.o $(Library)
	@echo "<**Linking**> $@ ..."
	$(LD) $@.o $(LDFLAGS) $(ANITA_COMP_LIBS)  $(ANITA_LIBS) -o $@
	@chmod 555 $@

clean: objclean
	@/bin/rm -f $(Library) $(TestProgs) $(DynLib)





















