# 
#
#

include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk


#Targets = eventd fakeAcqd fakeCalibd fakePrioritizerd fakeGPSd
Targets = hkd hkd3 hkcommand

all: $(Targets)


$(Targets): % : %.o
	@echo "<**Linking**> $@ ..."
	$(LD) $@.o $(LDFLAGS) $(ANITA_LIBS) $(ANITA_HKD_LIBS)  -o $@
	@chmod 555 $@

clean: objclean
	@-rm -f $(Targets) 




















