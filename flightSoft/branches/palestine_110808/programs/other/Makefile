# 
#
#

include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk


#Targets = eventd fakeAcqd fakeCalibd fakePrioritizerd fakeGPSd
Targets = makeFakeTrigger getConfigValue setConfigValue fakeEvent getConfigString


all: $(Targets)


$(Targets): % : %.o
	@echo "<**Linking**> $@ ..."
	@$(LD) $@.o $(LDFLAGS) $(ANITA_LIBS)  -o $@
	@chmod 555 $@
	ln -sf $(shell pwd)/$@ ${ANITA_FLIGHT_SOFT_DIR}/bin

clean: objclean
	@-rm -f $(Targets) 




















